//
// This file is part of the PlaJA code base.
// Copyright (c) (2019 - 2024) Marcel Vinzent.
// See README.md in the top-level directory for licensing information.
//

#include "solution_sampler.h"
#include "../../../utils/floating_utils.h"
#include "../../../utils/rng.h"
#include "../../information/model_information.h"
#include "../../states/state_values.h"
#include "../model/model_z3.h"
#include "../solver/smt_solver_z3.h"
#include "../solver/solution_z3.h"
#include "../vars_in_z3.h"

#ifndef NDEBUG

#include "../../../parser/ast/expression/expression.h"

#endif

namespace SOLUTION_ENUMERATOR_BS { STMT_IF_DEBUG(extern const Expression* groundTruth;) }

namespace SOLUTION_SAMPLER {

    template<typename T>
    struct CallStack {
        T lb;
        T ub;
        z3::expr boundZ3;

        CallStack(T lb, T ub, z3::expr bound_z3):
            lb(lb)
            , ub(ub)
            , boundZ3(bound_z3) {
            PLAJA_ASSERT(bool (PLAJA_UTILS::is_static_type<T, PLAJA::integer>()) or bool(PLAJA_UTILS::is_static_type<T, PLAJA::floating>()))
        }

        ~CallStack() = default;
        DELETE_CONSTRUCTOR(CallStack)
    };

}

/**********************************************************************************************************************/

SolutionSampler::SolutionSampler(Z3_IN_PLAJA::SMTSolver& solver_ref, const ModelZ3& model_z3):
    SolutionEnumeratorBase(model_z3.get_model_info())
    , solver(&solver_ref)
    , modelZ3(&model_z3)
    , stateIndexes(model_z3.get_src_vars().cache_state_indexes(false))
    , iterationOrder()
    , sampled(false) {
    iterationOrder.reserve(stateIndexes->size());
    for (auto it = stateIndexes->iterator(); !it.end(); ++it) { iterationOrder.push_back(it.state_index()); }
}

SolutionSampler::~SolutionSampler() = default;

/**********************************************************************************************************************/

void SolutionSampler::enumerate_next(IterationIndexType it_index, StateValues& state_values) {
    const auto& model_info = modelZ3->get_model_info();

    const IterationIndexType next_it_index = it_index + 1;

    if (next_it_index == iterationOrder.size()) {
        PLAJA_ASSERT(solver->check())
        PLAJA_ASSERT(not SOLUTION_ENUMERATOR_BS::groundTruth or SOLUTION_ENUMERATOR_BS::groundTruth->evaluate_integer(state_values))
        PLAJA_ASSERT(not sampled)
        sampled = true;
        return;
    }

    PLAJA_ASSERT(next_it_index < iterationOrder.size())
    const auto next_state_index = iterationOrder[next_it_index];
    PLAJA_ASSERT(stateIndexes->exists(next_state_index))

    const auto& next_var_expr = (*stateIndexes)[next_state_index];

    if (next_var_expr.is_bool()) { enumerate_bool(next_it_index, state_values); }
    else if (next_var_expr.is_int()) { enumerate_integer(next_it_index, model_info.get_lower_bound_int(next_state_index), model_info.get_upper_bound_int(next_state_index), state_values); }
    else {
        PLAJA_ASSERT(next_var_expr.is_real())
        enumerate_floating(next_it_index, model_info.get_lower_bound_float(next_state_index), model_info.get_upper_bound_float(next_state_index), state_values);
    }

}

void SolutionSampler::enumerate_bool(SolutionSampler::IterationIndexType it_index, StateValues& state_values) {

    const auto state_index = iterationOrder[it_index];

    std::list<std::pair<z3::expr, bool>> call_stacks;
    if (get_rng().flag()) {
        call_stacks.emplace_back((*stateIndexes)[state_index], true);
        call_stacks.emplace_back(!(*stateIndexes)[state_index], false);
    } else {
        call_stacks.emplace_back(!(*stateIndexes)[state_index], false);
        call_stacks.emplace_back((*stateIndexes)[state_index], true);
    }

    for (const auto& call_stack: call_stacks) {
        solver->push();
        solver->add(call_stack.first);
        state_values.assign_int<false>(state_index, call_stack.second);
        if (solver->check()) { enumerate_next(it_index, state_values); }
        solver->pop();
        if (sampled) { break; }
    }

}

void SolutionSampler::enumerate_integer(IterationIndexType it_index, PLAJA::integer lower_bound, PLAJA::integer upper_bound, StateValues& state_values) {
    PLAJA_ASSERT(upper_bound - lower_bound >= 0)

    const auto& state_indexes = *stateIndexes; // For convenience.
    auto& context = modelZ3->get_context();

    const auto state_index = iterationOrder[it_index];

    if (upper_bound - lower_bound == 0) {

        solver->push();
        solver->add(state_indexes[state_index] == context.int_val(lower_bound));
        state_values.assign_int<false>(state_index, lower_bound);
        enumerate_next(it_index, state_values);
        solver->pop();

    } else {

        /* Split values. */
        const PLAJA::integer split = lower_bound + (upper_bound - lower_bound) / 2;
        const PLAJA::integer split_lower = split;
        const PLAJA::integer split_upper = split + 1;

        PLAJA_ASSERT(lower_bound <= split_lower)
        PLAJA_ASSERT(split_lower <= split_upper)
        PLAJA_ASSERT(split_upper <= upper_bound)

        std::list<SOLUTION_SAMPLER::CallStack<PLAJA::integer>> call_stacks;
        if (get_rng().flag()) {
            call_stacks.emplace_back(lower_bound, split_lower, state_indexes[state_index] <= context.int_val(split_lower));
            call_stacks.emplace_back(split_upper, upper_bound, state_indexes[state_index] >= context.int_val(split_upper));
        } else {
            call_stacks.emplace_back(split_upper, upper_bound, state_indexes[state_index] >= context.int_val(split_upper));
            call_stacks.emplace_back(lower_bound, split_lower, state_indexes[state_index] <= context.int_val(split_lower));
        }

        for (const auto& call_stack: call_stacks) {
            solver->push();
            solver->add(call_stack.boundZ3);
            if (solver->check()) { enumerate_integer(it_index, call_stack.lb, call_stack.ub, state_values); }
            solver->pop();
            if (sampled) { break; }
        }

    }
}

void SolutionSampler::enumerate_floating(SolutionSampler::IterationIndexType it_index, PLAJA::floating lower_bound, PLAJA::floating upper_bound, StateValues& state_values) {
    PLAJA_ASSERT(lower_bound <= upper_bound)

    const auto& state_indexes = *stateIndexes; // For convenience.
    auto& context = modelZ3->get_context();

    const auto state_index = iterationOrder[it_index];

    if (PLAJA_FLOATS::equal(lower_bound, upper_bound, Z3_IN_PLAJA::enumerationPrecision)) {

        solver->push();
        PLAJA_ASSERT(solver->check())
        auto rlt = solver->get_solution()->eval(state_indexes[state_index], true);
        solver->add(state_indexes[state_index] == rlt);
        state_values.assign_float<false>(state_index, rlt.as_double());
        enumerate_next(it_index, state_values);
        solver->pop();

    } else if (PLAJA_FLOATS::equal(lower_bound, upper_bound, Z3_IN_PLAJA::samplePrecision)) {

        PLAJA_ASSERT(not sampled)
        PLAJA_ASSERT(solver->check())

        auto backup_sample = solver->get_solution()->eval(state_indexes[state_index], true);

        /* Special case: Unique solution. */
        const bool unique_sol = solver->check_push_pop(state_indexes[state_index] != backup_sample);

        auto trials = Z3_IN_PLAJA::sampleTrials;
        while (not unique_sol and trials-- > 0 and not sampled) {

            solver->push();
            const auto sample_value = get_rng().sample_float(lower_bound, upper_bound);
            solver->add(state_indexes[state_index] == context.real_val(sample_value, Z3_IN_PLAJA::floatingPrecision));
            if (solver->check()) {
                state_values.assign_float<false>(state_index, sample_value);
                enumerate_next(it_index, state_values);
            }
            solver->pop();

        }

        /* Fall-back. */
        if (not sampled) {
            solver->push();
            solver->add(state_indexes[state_index] == backup_sample);
            PLAJA_ASSERT(solver->check())
            state_values.assign_float<false>(state_index, backup_sample.as_double());
            enumerate_next(it_index, state_values);
            solver->pop();
        }

    } else {

        PLAJA::floating split = (lower_bound + upper_bound) / 2;

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

        std::list<SOLUTION_SAMPLER::CallStack<PLAJA::floating>> call_stacks;
        if (get_rng().flag()) {
            call_stacks.emplace_back(lower_bound, split, split_lower_expr);
            call_stacks.emplace_back(split, upper_bound, split_upper_expr);
        } else {
            call_stacks.emplace_back(split, upper_bound, split_upper_expr);
            call_stacks.emplace_back(lower_bound, split, split_lower_expr);
        }

        for (const auto& call_stack: call_stacks) {
            solver->push();
            solver->add(call_stack.boundZ3);
            if (solver->check()) { enumerate_floating(it_index, call_stack.lb, call_stack.ub, state_values); }
            solver->pop();
            if (sampled) { break; }
        }

    }
}

std::unique_ptr<StateValues> SolutionSampler::sample() {
    get_rng().shuffle(iterationOrder);
    auto sample = get_constructor().to_ptr();
    IterationIndexType it_index(-1);
    sampled = false;
    enumerate_next(it_index, *sample);
    PLAJA_ASSERT(sampled)
    if (sampled) { return sample; } else { return nullptr; }
}


