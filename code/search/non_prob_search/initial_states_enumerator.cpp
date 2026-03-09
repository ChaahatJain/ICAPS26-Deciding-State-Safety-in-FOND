//
// This file is part of the PlaJA code base.
// Copyright (c) (2019 - 2021) Marcel Vinzent.
// See README.md in the top-level directory for licensing information.
//

#include "initial_states_enumerator.h"
#include "../../include/factory_config_const.h"
#include "../../option_parser/enum_option_values_set.h"
#include "../../option_parser/option_parser.h"
#include "../../option_parser/plaja_options.h"
#include "../../parser/ast/expression/binary_op_expression.h"
#include "../../parser/ast/expression/constant_expression.h"
#include "../../parser/ast/expression/constant_value_expression.h"
#include "../../parser/ast/expression/expression.h"
#include "../../parser/ast/expression/non_standard/location_value_expression.h"
#include "../../parser/ast/expression/non_standard/state_condition_expression.h"
#include "../../parser/ast/expression/non_standard/states_values_expression.h"
#include "../../parser/ast/expression/special_cases/linear_expression.h"
#include "../../parser/ast/expression/variable_expression.h"
#include "../../parser/ast/model.h"
#include "../factories/configuration.h"
#include "../information/model_information.h"
#include "../information/property_information.h"
#include "../smt/bias_functions/avoid_counting.h"
#include "../smt/solution_enumerator/solution_enumerator_naive.h"

#include "../smt/solution_enumerator/state_values_enumerator.h"
#include "../states/state_values.h"
#include <vector>

#ifdef USE_Z3

#include "../smt/model/model_z3.h"
#include "../smt/solution_enumerator/solution_enumerator_bs.h"
#include "../smt/solution_enumerator/solution_enumerator_td.h"
#include "../smt/solution_enumerator/solution_enumerator_z3.h"
#include "../smt/solution_enumerator/solution_sampler.h"
#include "../smt/solution_enumerator/solution_sampler_optimizer.h"
#include "../smt/solver/smt_optimizer_z3.h"
#include "../smt/solver/smt_solver_z3.h"

#include "../smt/bias_functions/bias_to_z3.h"
#include "../smt/bias_functions/wp_bias.h"

#include "../../search/smt/visitor/to_z3_visitor.h"
#include "../../search/smt/vars_in_z3.h"

#endif

namespace SOLUTION_ENUMERATOR_BS { STMT_IF_DEBUG(extern const Expression* groundTruth;) }

/**********************************************************************************************************************/

InitialStatesEnumerator::InitialStatesEnumerator(const PLAJA::Configuration& config, const Expression& condition):
    modelSmt(nullptr)
    , solver(nullptr)
    , solutionEnumerator(nullptr)
    , intervals(config.has_value_option(PLAJA_OPTION::additional_initial_states) ? new std::vector<std::pair<int, int>>(config.share_option_parser()->get_option_intervals(PLAJA_OPTION::additional_initial_states)) : nullptr)
    CONSTRUCT_IF_DEBUG(startCondition(&condition)) {

    const auto* model = config.require_sharable_const<Model>(PLAJA::SharableKey::MODEL);

    const auto* initial_states_values = PLAJA_UTILS::cast_ref_if<StatesValuesExpression>(condition);
    if (initial_states_values) {
        solutionEnumerator = std::make_unique<StateValuesEnumerator>(model->get_model_information(), *initial_states_values);
    } else {
        const auto* initial_states_condition = PLAJA_GLOBAL::useZ3 and (not config.has_enum_option(PLAJA_OPTION::initial_state_enum, true) or config.get_enum_option(PLAJA_OPTION::initial_state_enum).get_value() != PLAJA_OPTION::EnumOptionValue::Naive) ? PLAJA_UTILS::cast_ref_if<StateConditionExpression>(condition) : nullptr;
        if (initial_states_condition) {
            prepare_smt_based_enumeration(config, *initial_states_condition);
        } else {
            PLAJA_LOG(PLAJA_UTILS::to_red_string("Warning: Initial states expression not supported by SMT-based enumeration!"))
            solutionEnumerator = std::make_unique<SolutionEnumeratorNaive>(model->get_model_information(), condition);
        }
    }

}

InitialStatesEnumerator::~InitialStatesEnumerator() = default;

/* */

void InitialStatesEnumerator::prepare_smt_based_enumeration(const PLAJA::Configuration& config, const StateConditionExpression& initial_state_condition) {

#ifdef USE_Z3

    /* Prepare z3 encoding. */
    if (not config.has_sharable(PLAJA::SharableKey::MODEL_Z3)) { config.set_sharable(PLAJA::SharableKey::MODEL_Z3, std::make_shared<ModelZ3>(config)); } // This loads the entire model in z3, which is relatively quit an overhead, but usually negligible in absolut terms.
    modelSmt = config.get_sharable_as_const<ModelZ3>(PLAJA::SharableKey::MODEL_Z3);
    solver = std::make_unique<Z3_IN_PLAJA::SMTSolver>(modelSmt->share_context());
    modelSmt->add_src_bounds(*solver, true);
    solver->push();
    const auto* start_constraint = initial_state_condition.get_constraint();
    if (start_constraint) { modelSmt->add_expression(*solver, *start_constraint, 0); }

    /* Prepare constructor. */
    StateValues constructor(modelSmt->get_model_info().get_initial_values());
    for (auto loc_it = initial_state_condition.init_loc_value_iterator(); !loc_it.end(); ++loc_it) {
        constructor.assign_int<true>(loc_it->get_location_index(), loc_it->get_location_value());
    }

    const auto initial_state_enum_config = config.get_enum_option(PLAJA_OPTION::initial_state_enum).get_value();

    if (initial_state_enum_config == PLAJA_OPTION::EnumOptionValue::Inc) {
                std::cout << "Inc" << std::endl;

        /* Naive solution enumeration (but still SMT-based). */
        solutionEnumerator = std::make_unique<SolutionEnumeratorZ3>(*solver, *modelSmt);
        solver->push(); // To suppress scoped_timer thread in z3, which is started when using tactic2solver.
        // solver->pop();
    } else if (initial_state_enum_config == PLAJA_OPTION::EnumOptionValue::Bs) {
        std::cout << "Bs" << std::endl;
        solutionEnumerator = std::make_unique<SolutionEnumeratorBS>(*solver, *modelSmt);

    } else if (initial_state_enum_config == PLAJA_OPTION::EnumOptionValue::Td) {
        std::cout << "Td" << std::endl;
        solutionEnumerator = std::make_unique<SolutionEnumeratorTD>(*solver, *modelSmt);

    } else if (initial_state_enum_config == PLAJA_OPTION::EnumOptionValue::Biased) {
        std::cout << "BiasedSample" << std::endl;

        const auto avoid = config.require_sharable_const<PropertyInformation>(PLAJA::SharableKey::PROP_INFO)->get_reach();
        // auto objective = BiasToZ3::distance_to_condition(avoid->deepCopy_Exp().get(), modelSmt->share_context());
        auto objective = BiasToZ3::distance_to_expression(avoid, modelSmt);
        auto optimizer = std::make_unique<SMTOptimizer>(modelSmt->share_context(), objective);
        modelSmt->add_src_bounds(*optimizer, true);
        optimizer->minimize();
        optimizer->push();

        if (start_constraint) {
            modelSmt->add_expression(*optimizer, *start_constraint, 0);
            // workaround for getting not phi_U instead:
            // auto not_unsafe_start = BiasToZ3::get_not_unsafe_start(start_constraint, modelSmt);
            // solver->add(not_unsafe_start);
        }
        optimizer->dump_solver();
        solver = std::move(optimizer);
        solutionEnumerator = std::make_unique<SolutionSamplerOptimizer>(*solver, *modelSmt);
    } else if (initial_state_enum_config == PLAJA_OPTION::EnumOptionValue::WP) {
        std::cout << "Biased WP" << std::endl;

        const auto avoid = config.require_sharable_const<PropertyInformation>(PLAJA::SharableKey::PROP_INFO)->get_reach();
        auto objective = WeakestPreconditionBias::get_wp_bias(avoid->deepCopy_Exp().get(), modelSmt->share_context(), modelSmt);
        auto optimizer = std::make_unique<SMTOptimizer>(modelSmt->share_context(), objective);
        modelSmt->add_src_bounds(*optimizer, true);

        if (start_constraint) {
            modelSmt->add_expression(*optimizer, *start_constraint, 0);
        }
        optimizer->minimize();
        solver = std::move(optimizer);
        solutionEnumerator = std::make_unique<SolutionSamplerOptimizer>(*solver, *modelSmt);
    } else if (initial_state_enum_config == PLAJA_OPTION::EnumOptionValue::AvoidCount) {
        std::cout << "Avoid Count" << std::endl;

        const auto avoid = config.require_sharable_const<PropertyInformation>(PLAJA::SharableKey::PROP_INFO)->get_reach();
        auto objective = AvoidCounting::get_avoid_count_bias(avoid->deepCopy_Exp().get(), modelSmt->share_context(), modelSmt);
        auto optimizer = std::make_unique<SMTOptimizer>(modelSmt->share_context(), objective);
        modelSmt->add_src_bounds(*optimizer, true);

        if (start_constraint) {
            modelSmt->add_expression(*optimizer, *start_constraint, 0);
        }
        optimizer->maximize();
        optimizer->dump_solver();
        solver = std::move(optimizer);
        solutionEnumerator = std::make_unique<SolutionSamplerOptimizer>(*solver, *modelSmt);
    } else {
        PLAJA_ASSERT(initial_state_enum_config == PLAJA_OPTION::EnumOptionValue::Sample)
        std::cout << "Sample" << std::endl;
        solutionEnumerator = std::make_unique<SolutionSampler>(*solver, *modelSmt);
    }

    solutionEnumerator->set_constructor(std::make_unique<StateValues>(std::move(constructor)));

#else
    MAYBE_UNUSED(config)
    MAYBE_UNUSED(model)
    MAYBE_UNUSED(initial_state_condition)
#endif

}

void InitialStatesEnumerator::initialize() {
    STMT_IF_DEBUG(SOLUTION_ENUMERATOR_BS::groundTruth = startCondition;)
    solutionEnumerator->initialize();
    STMT_IF_DEBUG(SOLUTION_ENUMERATOR_BS::groundTruth = nullptr;)
}

std::unique_ptr<StateValues> InitialStatesEnumerator::retrieve_state() {
    PLAJA_ASSERT(not PLAJA_UTILS::is_derived_type_or_null<SolutionSampler>(solutionEnumerator.get()))
    return solutionEnumerator->retrieve_solution();
}

std::list<StateValues> InitialStatesEnumerator::enumerate_states() {
    PLAJA_ASSERT(not samples())

    auto initial_states = solutionEnumerator->enumerate_solutions();

    if (intervals) {

        /* Add initial state if within interval. */
        std::size_t index = 0;
        std::list<StateValues> restricted_initial_states;
        for (auto& initial_state: initial_states) {
            if (std::any_of(intervals->cbegin(), intervals->cend(), [index](const std::pair<int, int>& interval) { return interval.first <= index && index <= interval.second; })) {
                restricted_initial_states.push_back(std::move(initial_state));
            }
            ++index;
        }

        return restricted_initial_states;

    } else {

        return initial_states;

    }

}

std::list<StateValues> InitialStatesEnumerator::enumerate_states(const PLAJA::Configuration& config, const Expression& condition) {
    InitialStatesEnumerator initial_states_enumerator(config, condition);
    initial_states_enumerator.initialize();
    return initial_states_enumerator.enumerate_states();
}

/* */

bool InitialStatesEnumerator::samples() const { return PLAJA_UTILS::is_derived_ptr_type<SolutionSampler>(solutionEnumerator.get()) || PLAJA_UTILS::is_derived_ptr_type<SolutionSamplerOptimizer>(solutionEnumerator.get()); }

std::unique_ptr<StateValues> InitialStatesEnumerator::sample_state() {
    PLAJA_ASSERT_EXPECT(samples())
    return solutionEnumerator->sample();
}

void InitialStatesEnumerator::update_start_condition(const Expression& new_start) {
    auto state_vars = modelSmt->get_state_vars();
    auto start_z3_cond = ToZ3Visitor::to_z3_condition(new_start, state_vars);
    solver->reset(); // remove old start condition
    solver->add(start_z3_cond);

}
