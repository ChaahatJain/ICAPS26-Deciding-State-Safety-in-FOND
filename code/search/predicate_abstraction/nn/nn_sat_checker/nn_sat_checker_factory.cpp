//
// This file is part of the PlaJA code base.
// Copyright (c) (2019 - 2022) Marcel Vinzent.
// See README.md in the top-level directory for licensing information.
//

#include <memory>
#include "nn_sat_checker_factory.h"
#include "../../../../exception/constructor_exception.h"
#include "../../../../include/ct_config_const.h"
#include "../../../../option_parser/option_parser_aux.h"
#include "../../../../option_parser/option_parser.h"
#include "../../../../parser/ast/expression/non_standard/predicates_expression.h"
#include "../../../../parser/visitor/extern/to_linear_expression.h"
#include "../../../../parser/visitor/linear_constraints_checker.h"
#include "../../../factories/configuration.h"
#include "../nn_sat_in_z3/marabou_in_z3_checker.h"
#include "../nn_sat_in_z3/nn_sat_checker_z3.h"
#include "../smt/model_marabou_pa.h"

// extern
namespace PLAJA_OPTION { extern const std::string marabou_mode; }

namespace PLAJA_OPTION_DEFAULT {

    const std::string nn_sat("marabou"); // NOLINT(cert-err58-cpp)
    const std::string per_src_tightening("none"); // NOLINT(cert-err58-cpp)
    const std::string per_src_label_tightening("none"); // NOLINT(cert-err58-cpp)

}

namespace PLAJA_OPTION {

    const std::string nn_sat("nn-sat"); // NOLINT(cert-err58-cpp)
    const std::string per_src_tightening("per-src-tightening"); // NOLINT(cert-err58-cpp)
    const std::string per_src_label_tightening("per-src-label-tightening"); // NOLINT(cert-err58-cpp)
    const std::string relaxed_for_exact("relaxed-for-exact"); // NOLINT(cert-err58-cpp)
    const std::string stalling_detection("stalling-detection"); // NOLINT(cert-err58-cpp)
    const std::string adversarial_attack("adversarial-attack"); // NOLINT(cert-err58-cpp)
    const std::string ignore_attack_constraints("ignore_attack_constraints"); // NOLINT(cert-err58-cpp)
    const std::string attack_solution_guess("attack-solution-guess"); // NOLINT(cert-err58-cpp)
    const std::string attack_starts("attack-starts"); // NOLINT(cert-err58-cpp)
    const std::string pgd_steps("pgd-steps"); // NOLINT(cert-err58-cpp)
    const std::string random_attack("random-attack"); // NOLINT(cert-err58-cpp)
    const std::string scale_attack_step("scale_attack_step"); // NOLINT(cert-err58-cpp)
    const std::string constraint_delta_dir("constraint_delta_dir"); // NOLINT(cert-err58-cpp)
    const std::string query_cache("query-cache"); // NOLINT(cert-err58-cpp)

    namespace SMT_SOLVER_MARABOU { extern std::string print_supported_bound_tightening(); }

    namespace NN_SAT_CHECKER_PA {

        void add_options(PLAJA::OptionParser& option_parser) {
            OPTION_PARSER::add_option(option_parser, PLAJA_OPTION::nn_sat, PLAJA_OPTION_DEFAULT::nn_sat);
            OPTION_PARSER::add_flag(option_parser, PLAJA_OPTION::relaxed_for_exact);
            OPTION_PARSER::add_option(option_parser, PLAJA_OPTION::per_src_tightening, PLAJA_OPTION_DEFAULT::per_src_tightening);
            OPTION_PARSER::add_option(option_parser, PLAJA_OPTION::per_src_label_tightening, PLAJA_OPTION_DEFAULT::per_src_label_tightening);

            if constexpr (PLAJA_GLOBAL::enableStallingDetection) { OPTION_PARSER::add_flag(option_parser, PLAJA_OPTION::stalling_detection); }

            if constexpr (PLAJA_GLOBAL::useAdversarialAttack) {
                OPTION_PARSER::add_flag(option_parser, PLAJA_OPTION::adversarial_attack);
                OPTION_PARSER::add_flag(option_parser, PLAJA_OPTION::ignore_attack_constraints);
                OPTION_PARSER::add_flag(option_parser, PLAJA_OPTION::attack_solution_guess);
                OPTION_PARSER::add_value_option(option_parser, PLAJA_OPTION::attack_starts);
                OPTION_PARSER::add_value_option(option_parser, PLAJA_OPTION::pgd_steps);
                OPTION_PARSER::add_value_option(option_parser, PLAJA_OPTION::random_attack);
                OPTION_PARSER::add_value_option(option_parser, PLAJA_OPTION::scale_attack_step);
                OPTION_PARSER::add_flag(option_parser, PLAJA_OPTION::constraint_delta_dir);
            }

            if constexpr (PLAJA_GLOBAL::enableQueryCaching) { OPTION_PARSER::add_value_option(option_parser, PLAJA_OPTION::query_cache); }
        }

        void print_options() {
            OPTION_PARSER::print_option(PLAJA_OPTION::nn_sat, OPTION_PARSER::valueStr, PLAJA_OPTION_DEFAULT::nn_sat, "Set mode for NN-satisfiability check to compute (relaxed) PPA):", true);
            OPTION_PARSER::print_additional_specification(NN_SAT_CHECKER_PA::print_supported_nn_sat_modes());

            if constexpr (PLAJA_GLOBAL::supportGurobi) {
                OPTION_PARSER::print_option(PLAJA_OPTION::per_src_tightening, OPTION_PARSER::valueStr, PLAJA_OPTION_DEFAULT::per_src_tightening, "Per abstract source expansion tightening:", true);
                OPTION_PARSER::print_additional_specification(PLAJA_OPTION::SMT_SOLVER_MARABOU::print_supported_bound_tightening());
                OPTION_PARSER::print_option(PLAJA_OPTION::per_src_label_tightening, OPTION_PARSER::valueStr, PLAJA_OPTION_DEFAULT::per_src_label_tightening, "Per abstract source expansion and label tightening.");
            }

            OPTION_PARSER::print_flag(PLAJA_OPTION::relaxed_for_exact, "First query relaxed in Marabou, then exact in Z3.");

            if constexpr (PLAJA_GLOBAL::enableStallingDetection) { OPTION_PARSER::print_flag(PLAJA_OPTION::stalling_detection, "Activate stalling detection."); }

            if constexpr (PLAJA_GLOBAL::useAdversarialAttack) {
                OPTION_PARSER::print_flag(PLAJA_OPTION::adversarial_attack, "Activate adversarial attack methods.");
                OPTION_PARSER::print_flag(PLAJA_OPTION::ignore_attack_constraints, "Ignore constraints in adversarial attacks.");
                OPTION_PARSER::print_flag(PLAJA_OPTION::attack_solution_guess, "Provide a solution guess to the attack (usually a constraint satisfying source state).");
                OPTION_PARSER::print_option(PLAJA_OPTION::attack_starts, OPTION_PARSER::valueStr, "Number of vectors to start an attack from.");
                OPTION_PARSER::print_option(PLAJA_OPTION::pgd_steps, OPTION_PARSER::valueStr, "Maximal number of pgd_steps.");
                OPTION_PARSER::print_option(PLAJA_OPTION::random_attack, OPTION_PARSER::valueStr, "Random attack steps in +- " + OPTION_PARSER::valueStr + PLAJA_UTILS::dotString);
                OPTION_PARSER::print_option(PLAJA_OPTION::scale_attack_step, OPTION_PARSER::valueStr, "Scale absolut step size for at least one attack up to" + OPTION_PARSER::valueStr + PLAJA_UTILS::dotString);
                OPTION_PARSER::print_flag(PLAJA_OPTION::constraint_delta_dir, "Constraint attack step direction during projection step.");
            }

            if constexpr (PLAJA_GLOBAL::enableQueryCaching) { OPTION_PARSER::print_option(PLAJA_OPTION::query_cache, OPTION_PARSER::valueStr, "Cache NN-SAT queries."); }
        }

        const std::unordered_map<std::string, NN_SAT_CHECKER::NNSatMode> stringToNNSatMode { // NOLINT(cert-err58-cpp)
            { "marabou",    NN_SAT_CHECKER::NNSatMode::MARABOU },
            { "z3",         NN_SAT_CHECKER::NNSatMode::Z3 },
            { "z3_marabou", NN_SAT_CHECKER::NNSatMode::Z3_MARABOU } };

        std::string print_supported_nn_sat_modes() { return PLAJA::OptionParser::print_supported_options(stringToNNSatMode); }

        NN_SAT_CHECKER::NNSatMode get_nn_sat_mode(const PLAJA::Configuration& config) {
            if (not config.has_option(PLAJA_OPTION::nn_sat)) { return NN_SAT_CHECKER::NNSatMode::NONE; }
            return config.get_option(PLAJA_OPTION::NN_SAT_CHECKER_PA::stringToNNSatMode, PLAJA_OPTION::nn_sat);
        }

        bool is_relaxed_nn_sat_mode(const PLAJA::Configuration& config) {
            if (get_nn_sat_mode(config) == NN_SAT_CHECKER::NNSatMode::MARABOU) {
                return config.get_option(PLAJA_OPTION::marabou_mode) == "relaxed";
            }

            return false;
        }

        NN_SAT_CHECKER::NNSatMode get_nn_sat_mode(const std::string& mode) {
            auto it = stringToNNSatMode.find(mode);
            if (it == stringToNNSatMode.end()) { return NN_SAT_CHECKER::NNSatMode::NONE; }
            else { return it->second; }
        }

        std::string get_nn_sat_mode(NN_SAT_CHECKER::NNSatMode mode) {
            switch (mode) {
                case NN_SAT_CHECKER::NNSatMode::MARABOU: { return "marabou"; }
                case NN_SAT_CHECKER::NNSatMode::Z3: { return "z3"; }
                case NN_SAT_CHECKER::NNSatMode::Z3_MARABOU: { return "z3_marabou"; }
                default: { PLAJA_ABORT }
            }
            PLAJA_ABORT
        }

        void set_nn_sat_mode(PLAJA::Configuration& config, NN_SAT_CHECKER::NNSatMode mode) {
            if (mode == NN_SAT_CHECKER::NNSatMode::NONE) { config.disable_value_option(PLAJA_OPTION::nn_sat); }
            else { config.set_value_option(PLAJA_OPTION::nn_sat, get_nn_sat_mode(mode)); }
        }

    }

}

/**********************************************************************************************************************/

std::unique_ptr<NNSatChecker> NN_SAT_CHECKER::construct_checker(const Jani2NNet& jani_2_nnet, const PLAJA::Configuration& config) {
    PLAJA_LOG("Constructing NN-SAT checker ...")

    // Check predicates
    PLAJA_ASSERT(config.has_sharable(PLAJA::SharableKey::MODEL_MARABOU))
    PLAJA_ASSERT(config.get_sharable<ModelMarabou>(PLAJA::SharableKey::MODEL_MARABOU)->_predicates())
    for (auto pred_it = config.get_sharable<ModelMarabou>(PLAJA::SharableKey::MODEL_MARABOU)->_predicates()->predicatesIterator(); !pred_it.end(); ++pred_it) {
        if (not TO_LINEAR_EXP::is_linear_constraint(*pred_it) and not LinearConstraintsChecker::is_linear(*pred_it, LINEAR_CONSTRAINTS_CHECKER::Specification::as_predicate(true))) { throw ConstructorException(pred_it->to_string(), PLAJA_EXCEPTION::nonLinear); }
    }

    switch (config.get_option(PLAJA_OPTION::NN_SAT_CHECKER_PA::stringToNNSatMode, PLAJA_OPTION::nn_sat)) {
        case NN_SAT_CHECKER::NNSatMode::MARABOU: { return std::make_unique<NNSatChecker>(jani_2_nnet, config); }
        case NN_SAT_CHECKER::NNSatMode::Z3: { return std::unique_ptr<NNSatChecker>(new NNSatCheckerZ3(jani_2_nnet, config)); }
        case NN_SAT_CHECKER::NNSatMode::Z3_MARABOU: { return std::unique_ptr<NNSatChecker>(new MarabouInZ3Checker(jani_2_nnet, config)); }
        default: { PLAJA_ABORT }
    }
    PLAJA_ABORT
}
