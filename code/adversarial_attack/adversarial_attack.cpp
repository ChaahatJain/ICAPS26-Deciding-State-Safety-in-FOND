//
// This file is part of the PlaJA code base.
// Copyright (c) (2019 - 2021) Marcel Vinzent.
// See README.md in the top-level directory for licensing information.
//

#include <cstdlib>
#include <pybind11/embed.h>
#include "adversarial_attack.h"
#include "../include/marabou_include/equation.h"
#include "../include/marabou_include/input_query.h"
#include "../include/ct_config_const.h"
#include "../include/pybind_config.h"
#include "../option_parser/option_parser.h"
#include "../search/information/jani2nnet/jani_2_nnet.h"
#include "../search/predicate_abstraction/nn/utils/state_indexes_to_marabou.h"
#include "../search/predicate_abstraction/nn/search_statistics_nn_sat.h"
#include "../search/states/state_values.h"

namespace py = pybind11;

// extern:
namespace PLAJA_GLOBAL { extern const PLAJA::OptionParser* optionParser; }
namespace PLAJA_OPTION {
    extern const std::string ignore_attack_constraints;
    extern const std::string attack_starts;
    extern const std::string pgd_steps;
    extern const std::string random_attack;
    extern const std::string scale_attack_step;
    extern const std::string constraint_delta_dir;
}
namespace PLAJA_UTILS {
    extern std::string extract_directory_path(const std::string& path_to_file);
    extern std::string extract_filename(const std::string& path_to_file, bool keep_extension);
}

#pragma GCC diagnostic push
#pragma ide diagnostic ignored "Simplify"
namespace ADVERSARIAL_ATTACK { struct Initializer { Initializer() { setenv("PYTHONOPTIMIZE", PLAJA_GLOBAL::debug ? "" : "TRUE", true); } }; }
#pragma GCC diagnostic pop

AdversarialAttack::AdversarialAttack(const Jani2NNet& jani_2_nnet, std::size_t num_additional_vars):
    sharedStatistics(nullptr)
    , initializer(new ADVERSARIAL_ATTACK::Initializer())
    , interpreter(new py::scoped_interpreter())
    , inputSize(jani_2_nnet.get_num_input_features())
    , constraintVarsOffset(jani_2_nnet.number_of_marabou_encoding_vars() - inputSize)
    , constraintsOffset(jani_2_nnet.count_linear_constraints()) {

    // disable constraints on request
    if (PLAJA_GLOBAL::optionParser->is_flag_set(PLAJA_OPTION::ignore_attack_constraints)) { constraintsOffset = std::numeric_limits<std::size_t>::max(); }

    // make site-packages of local environment accessible
    py::module sys = py::module::import("sys");
    sys.attr("path").attr("insert")(1, CUSTOM_SYS_PATH);
    sys.attr("path").attr("insert")(1, ATTACK_MODULE);

    /*
    auto TorchModule = std::make_unique<py::object>(py::module_::import("torch"));
    TorchModule->attr("manual_seed")(0);
    TorchModule->attr("set_num_threads")(1);
    TorchModule->attr("set_num_interop_threads")(1);
    */

    // only afterwards we construct the module
    auto AdversarialAttackModule = std::make_unique<py::object>(py::module_::import("adversarial_attacks").attr("AdversarialAttacks"));
    auto QueryModule = std::make_unique<py::object>(py::module_::import("query").attr("Query"));
    ConstraintModule = std::make_unique<py::object>(py::module_::import("query").attr("Constraint"));

    unsigned int input_size = jani_2_nnet.get_num_input_features();
    unsigned int output_size = jani_2_nnet.get_num_output_features();
    py::list hidden_layer_sizes_py;
    for (auto layer_size: jani_2_nnet.get_hidden_layer_sizes()) { hidden_layer_sizes_py.attr("append")(layer_size); }

    attackInstance = std::make_unique<py::object>((*AdversarialAttackModule)(jani_2_nnet.get_torch_file(), input_size, hidden_layer_sizes_py, output_size,
                                                                             PLAJA_GLOBAL::optionParser->get_option_value(PLAJA_OPTION::attack_starts, 1),
                                                                             PLAJA_GLOBAL::optionParser->get_option_value(PLAJA_OPTION::pgd_steps, 25),
                                                                             PLAJA_GLOBAL::optionParser->get_option_value(PLAJA_OPTION::random_attack, 0),
                                                                             PLAJA_GLOBAL::optionParser->get_option_value(PLAJA_OPTION::scale_attack_step, 0),
                                                                             PLAJA_GLOBAL::optionParser->is_flag_set(PLAJA_OPTION::constraint_delta_dir)));
    queryInstance = std::make_unique<py::object>((*QueryModule)(input_size, num_additional_vars));
}

AdversarialAttack::~AdversarialAttack() = default;

//


inline unsigned int AdversarialAttack::marabou_var_to_py(MarabouVarIndex_type var) const {
    PLAJA_ASSERT(var < inputSize || inputSize + constraintVarsOffset <= var)
    if (var >= inputSize) { var -= constraintVarsOffset; }
    return var;
}

inline MarabouVarIndex_type AdversarialAttack::py_to_marabou_var(unsigned int var) const {
    if (var >= inputSize) { var += constraintVarsOffset; }
    return var;
}

namespace ADVERSARIAL_ATTACK {

    int eq_op_to_py_op(Equation::EquationType op) {
        switch (op) {
            case Equation::LE: { return 0; }
            case Equation::EQ: { return 1; }
            case Equation::GE: { return 2; }
        }
        PLAJA_ABORT
    }

}

void AdversarialAttack::set_bounds(MarabouVarIndex_type var, int lower_bound, int upper_bound) {
    queryInstance->attr("set_bounds")(marabou_var_to_py(var), lower_bound, upper_bound);
}

void AdversarialAttack::add_constraint(const Equation& eq) {
    pybind11::object constraint = (*ConstraintModule)(ADVERSARIAL_ATTACK::eq_op_to_py_op(eq._type), eq._scalar);
    for (const auto& addend: eq._addends) {
        auto var = addend._variable;
        if (var >= inputSize) { var -= constraintVarsOffset; }
        constraint.attr("add_addend")(var, addend._coefficient);
    }
    queryInstance->attr("add_constraint")(constraint);
}

void AdversarialAttack::set_label(OutputIndex_type output_index) { queryInstance->attr("set_label")(output_index); }

void AdversarialAttack::set_solution_guess(const StateValues& solution_guess, const InputQuery& input_query, const StateIndexesToMarabou& state_indexes_to_marabou) {
    // TODO here we do not set target variables properly; however handled by projection step and on the long run we might substitute them anyway
    queryInstance->attr("init_solution_guess")();
    for (const auto& input_var: input_query._variableToInputIndex) {
        MarabouVarIndex_type var = input_var.first;
        queryInstance->attr("set_solution_guess")(marabou_var_to_py(var), solution_guess[state_indexes_to_marabou._source_reverse(var)]);
    }
}

bool AdversarialAttack::check() {
    if (sharedStatistics) { sharedStatistics->inc_attr_unsigned(PLAJA::StatsUnsigned::AD_ATTACKS); }
    bool rlt = attackInstance->attr("check_query")(*queryInstance).cast<bool>();
    if (sharedStatistics) {
        if (attackInstance->attr("is_query_solved_at_start")().cast<bool>()) { sharedStatistics->inc_attr_unsigned(PLAJA::StatsUnsigned::AD_ATTACKS_SOLVED_AT_START); }
        if (rlt) {
            sharedStatistics->inc_attr_unsigned(PLAJA::StatsUnsigned::AD_ATTACKS_SUCCESSFUL);
            sharedStatistics->inc_attr_unsigned(PLAJA::StatsUnsigned::AD_ATTACKS_STEPS_SUC, attackInstance->attr("get_num_attack_steps")().cast<int>());
            sharedStatistics->inc_attr_unsigned(PLAJA::StatsUnsigned::AD_ATTACKS_PROJECTIONS_SUC, attackInstance->attr("get_num_projections")().cast<int>());
        } else {
            sharedStatistics->inc_attr_unsigned(PLAJA::StatsUnsigned::AD_ATTACKS_STEPS_NON_SUC, attackInstance->attr("get_num_attack_steps")().cast<int>());
            sharedStatistics->inc_attr_unsigned(PLAJA::StatsUnsigned::AD_ATTACKS_PROJECTIONS_NON_SUC, attackInstance->attr("get_num_projections")().cast<int>());
        }
    }
    return rlt;
}

bool AdversarialAttack::check(const InputQuery& input_query, OutputIndex_type output_index, const StateValues* solution_guess, const StateIndexesToMarabou* state_indexes_to_marabou) {
    queryInstance->attr("reset")();
    // variables
    for (MarabouVarIndex_type marabou_index = 0; marabou_index < inputSize; ++marabou_index) {
        set_bounds(marabou_index, static_cast<int>(std::floor(input_query.getLowerBound(marabou_index))), static_cast<int>(std::ceil(input_query.getUpperBound(marabou_index))));
    }
    for (MarabouVarIndex_type marabou_index = inputSize + constraintVarsOffset; marabou_index < input_query.getNumberOfVariables(); ++marabou_index) {
        set_bounds(marabou_index, static_cast<int>(std::floor(input_query.getLowerBound(marabou_index))), static_cast<int>(std::ceil(input_query.getUpperBound(marabou_index))));
    }
    // constraints
    std::size_t index = 0;
    for (const auto& equation: input_query.getEquations()) { if (index++ >= constraintsOffset) { add_constraint(equation); } }
    // label
    set_label(output_index);
    // solution guess
    if (solution_guess) { set_solution_guess(*solution_guess, input_query, *state_indexes_to_marabou); }
    return check();
}

void AdversarialAttack::extract_solution(StateValues& solution, const InputQuery& input_query, const StateIndexesToMarabou& state_indexes_to_marabou) {
    PLAJA_ASSERT(queryInstance->attr("has_solution")().cast<bool>())
    for (const auto& input_var: input_query._variableToInputIndex) {
        MarabouVarIndex_type var = input_var.first;
        solution.assign(state_indexes_to_marabou._source_reverse(var), queryInstance->attr("extract_solution")(marabou_var_to_py(var)).cast<integer>());
    }
}