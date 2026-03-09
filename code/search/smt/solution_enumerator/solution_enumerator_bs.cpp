//
// This file is part of the PlaJA code base.
// Copyright (c) (2019 - 2020) Marcel Vinzent.
// See README.md in the top-level directory for licensing information.
//

#include "solution_enumerator_bs.h"
#include "../../../utils/floating_utils.h"
#include "../../information/model_information.h"
#include "../../states/state_values.h"
#include "../model/model_z3.h"
#include "../solver/smt_solver_z3.h"
#include "../solver/solution_z3.h"
#include "../context_z3.h"
#include "../vars_in_z3.h"

#ifndef NDEBUG

#include "../../../parser/ast/expression/expression.h"

#endif

#define SPLIT_SOLUTION
namespace SOLUTION_ENUMERATOR_BS {

    constexpr bool split_solution = true; // Essentially branch and bound.

    STMT_IF_DEBUG(extern const Expression* groundTruth;)
    STMT_IF_DEBUG(const Expression* groundTruth(nullptr);)

}

SolutionEnumeratorBS::SolutionEnumeratorBS(Z3_IN_PLAJA::SMTSolver& solver_ref, const ModelZ3& model_z3):
    SolutionEnumeratorBase(model_z3.get_model_info())
    , solver(&solver_ref)
    , modelZ3(&model_z3)
    , stateIndexes(model_z3.get_src_vars().cache_state_indexes(false)) {

}

SolutionEnumeratorBS::~SolutionEnumeratorBS() = default;

/**********************************************************************************************************************/

void SolutionEnumeratorBS::enumerate_next(VariableIndex_type state_index, StateValues& state_values) { // NOLINT(misc-no-recursion)
    const auto& model_info = modelZ3->get_model_info();

    const VariableIndex_type next_state_index = state_index + 1;
    if (next_state_index == model_info.get_state_size()) {
        PLAJA_ASSERT(not SOLUTION_ENUMERATOR_BS::groundTruth or SOLUTION_ENUMERATOR_BS::groundTruth->evaluate_integer(state_values))
        solutions.emplace_back(state_values);
    } else if (not stateIndexes->exists(next_state_index)) {
        enumerate_next(next_state_index, state_values); // skip unused state indexes
    } else {
        const auto& next_var_expr = stateIndexes->operator[](next_state_index);
        if (next_var_expr.is_bool()) { enumerate_bool(next_state_index, state_values); }
        else if (next_var_expr.is_int()) { enumerate_integer(next_state_index, model_info.get_lower_bound_int(next_state_index), model_info.get_upper_bound_int(next_state_index), state_values); }
        else {
            PLAJA_ASSERT(next_var_expr.is_real())
            enumerate_floating(next_state_index, model_info.get_lower_bound_float(next_state_index), model_info.get_upper_bound_float(next_state_index), state_values);
        }
    }
}

void SolutionEnumeratorBS::enumerate_bool(VariableIndex_type state_index, StateValues& state_values) { // NOLINT(misc-no-recursion)
    // TRUE
    solver->push();
    solver->add((*stateIndexes)[state_index]);
    if (solver->check()) {
        state_values.assign_int<false>(state_index, true);
        enumerate_next(state_index, state_values);
    }
    solver->pop();

    // FALSE
    solver->push();
    solver->add(!(*stateIndexes)[state_index]);
    if (solver->check()) {
        state_values.assign_int<false>(state_index, false);
        enumerate_next(state_index, state_values);
    }
    solver->pop();
}

void SolutionEnumeratorBS::enumerate_integer(VariableIndex_type state_index, PLAJA::integer lower_bound, PLAJA::integer upper_bound, StateValues& state_values) { // NOLINT(misc-no-recursion)
    PLAJA_ASSERT(upper_bound - lower_bound >= 0)

    const auto& state_indexes = *stateIndexes; // for convenience
    auto& context = modelZ3->get_context();

    if (upper_bound - lower_bound == 0) {
        solver->push();
        solver->add(state_indexes[state_index] == context.int_val(lower_bound));
        state_values.assign_int<false>(state_index, lower_bound);
        enumerate_next(state_index, state_values);
        solver->pop();
    } else {

        PLAJA::integer split, split_lower, split_upper; // NOLINT(*-init-variables)

        if constexpr (SOLUTION_ENUMERATOR_BS::split_solution) {
            if (not solver->check()) { return; }

            auto solution = solver->get_solution();
            split = solution->eval(state_indexes[state_index], true).get_numeral_int();

            if (split < upper_bound) {
                split_lower = split;
                split_upper = split + 1;
            } else {
                PLAJA_ASSERT(split > lower_bound)
                split_lower = split - 1;
                split_upper = split;
            }

        } else {
            // split values
            split = lower_bound + (upper_bound - lower_bound) / 2;
            split_lower = split;
            split_upper = split + 1;
        }

        // split expressions
        const z3::expr split_lower_expr = modelZ3->get_context().int_val(split_lower);
        const z3::expr split_upper_expr = modelZ3->get_context().int_val(split_upper);

        // [lower_bound, split_lower]
        solver->push();
        solver->add(state_indexes[state_index] <= split_lower_expr);
        if ((SOLUTION_ENUMERATOR_BS::split_solution and split_lower == split) or solver->check()) { enumerate_integer(state_index, lower_bound, split_lower, state_values); }
        solver->pop();

        // [split_upper, upper_bound]
        solver->push();
        solver->add(state_indexes[state_index] >= split_upper_expr);
        if ((SOLUTION_ENUMERATOR_BS::split_solution and split_upper == split) or solver->check()) { enumerate_integer(state_index, split_upper, upper_bound, state_values); }
        solver->pop();

    }

}

void SolutionEnumeratorBS::enumerate_floating(VariableIndex_type state_index, PLAJA::floating lower_bound, PLAJA::floating upper_bound, StateValues& state_values) { // NOLINT(misc-no-recursion)
    PLAJA_ASSERT(lower_bound <= upper_bound)

    const auto& state_indexes = *stateIndexes; // for convenience
    auto& context = modelZ3->get_context();

    if (PLAJA_FLOATS::is_zero(upper_bound - lower_bound, Z3_IN_PLAJA::enumerationPrecision)) {
        solver->push();
        PLAJA_ASSERT(solver->check())
        auto solution = solver->get_solution();
        auto rlt = solution->eval(state_indexes[state_index], true);
        solver->add(state_indexes[state_index] == rlt);
        state_values.assign_float<false>(state_index, rlt.as_double());
        enumerate_next(state_index, state_values);
        solver->pop();
    } else {

        PLAJA::floating split; // NOLINT(*-init-variables)

        if constexpr (SOLUTION_ENUMERATOR_BS::split_solution) {

            if (not solver->check()) { return; }

            auto solution = solver->get_solution();
            split = solution->eval(state_indexes[state_index], true).as_double();

        } else {
            split = lower_bound + (upper_bound - lower_bound) / 2; // split point
        }

        /* Split expressions. */
        z3::expr split_lower_expr(context());
        z3::expr split_upper_expr(context());

        /* Handle oob due to "as_double" rounding. */
        split = split < lower_bound ? lower_bound : (upper_bound < split ? upper_bound : split);
        PLAJA_ASSERT(upper_bound - split >= 0)
        PLAJA_ASSERT(split - lower_bound >= 0)

        if (upper_bound - split < split - lower_bound) { // Split point is closer to upper bound.
            split_lower_expr = state_indexes[state_index] <= context.real_val(split - Z3_IN_PLAJA::enumerationPrecision, Z3_IN_PLAJA::floatingPrecision);
            split_upper_expr = state_indexes[state_index] >= context.real_val(split, Z3_IN_PLAJA::floatingPrecision);
        } else {
            split_lower_expr = state_indexes[state_index] <= context.real_val(split, Z3_IN_PLAJA::floatingPrecision);
            split_upper_expr = state_indexes[state_index] >= context.real_val(split + Z3_IN_PLAJA::enumerationPrecision, Z3_IN_PLAJA::floatingPrecision);
        }

        /* [lower_bound, split_lower] */
        solver->push();
        solver->add(split_lower_expr);
        if (solver->check()) { enumerate_floating(state_index, lower_bound, split, state_values); }
        solver->pop();

        /* [split_upper, upper_bound] */
        solver->push();
        solver->add(split_upper_expr);
        if (solver->check()) { enumerate_floating(state_index, split, upper_bound, state_values); }
        solver->pop();

    }
}

/**********************************************************************************************************************/

void SolutionEnumeratorBS::initialize() {
    solutions.clear();
    const VariableIndex_type state_index = 0; // solution_enumerator.modelInfo.get_automata_offset();
    auto constructor = get_constructor();
    enumerate_next(state_index - 1, constructor);
    set_initialized();
}

/**********************************************************************************************************************/

std::list<StateValues> SolutionEnumeratorBS::compute_solutions(Z3_IN_PLAJA::SMTSolver& solver, const ModelZ3& model_z3, StateValues&& constructor) {
    SolutionEnumeratorBS solution_enumerator(solver, model_z3);
    solution_enumerator.set_constructor(std::make_unique<StateValues>(std::move(constructor)));
    return solution_enumerator.enumerate_solutions();
}