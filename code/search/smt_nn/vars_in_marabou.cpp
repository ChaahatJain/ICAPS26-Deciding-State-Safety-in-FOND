//
// This file is part of the PlaJA code base.
// Copyright (c) (2019 - 2022) Marcel Vinzent.
// See README.md in the top-level directory for licensing information.
//

#include "vars_in_marabou.h"
#include "../information/model_information.h"
#include "../states/state_values.h"
#include "solver/solution_marabou.h"
#include "constraints_in_marabou.h"
#include "marabou_context.h"

StateIndexesInMarabou::StateIndexesInMarabou(const ModelInformation& model_info, MARABOU_IN_PLAJA::Context& c):
    modelInfo(&model_info)
    , context(&c) {}

StateIndexesInMarabou::~StateIndexesInMarabou() = default;

MarabouVarIndex_type StateIndexesInMarabou::add(VariableIndex_type state_index, bool mark_integer, bool is_input) {
    PLAJA_ASSERT(not exists(state_index))
    auto marabou_var = context->add_var(modelInfo->get_lower_bound(state_index), modelInfo->get_upper_bound(state_index), mark_integer and modelInfo->is_integer(state_index), is_input, false);
    stateIndexToMarabou.emplace(state_index, marabou_var);
    marabouToStateIndex.emplace(marabou_var, state_index);
    return marabou_var;
}

void StateIndexesInMarabou::set(VariableIndex_type state_index, MarabouVarIndex_type marabou_var) {
    PLAJA_ASSERT(not context->is_marked_output(marabou_var))
    // delete old mapping:
    auto it_state = stateIndexToMarabou.find(state_index);
    if (it_state != stateIndexToMarabou.end()) {
        PLAJA_ASSERT(exists_reverse(it_state->second))
        marabouToStateIndex.erase(it_state->second);
    }
    auto it_marabou = marabouToStateIndex.find(marabou_var);
    if (it_marabou != marabouToStateIndex.end()) {
        PLAJA_ASSERT(exists(it_marabou->second))
        stateIndexToMarabou.erase(it_marabou->second);
    }
    //
    PLAJA_ASSERT(not exists(state_index) or not exists_reverse(to_marabou(state_index)))
    PLAJA_ASSERT(not exists_reverse(marabou_var) or not exists(to_state_index(state_index)))
    //
    stateIndexToMarabou[state_index] = marabou_var;
    marabouToStateIndex[marabou_var] = state_index;
}

#ifndef NDEBUG

StateIndexesInMarabou::StateIndexesInMarabou(MARABOU_IN_PLAJA::Context& c):
    modelInfo(nullptr)
    , context(&c) {}

MarabouVarIndex_type StateIndexesInMarabou::add(VariableIndex_type state_index, MarabouFloating_type lb, MarabouFloating_type ub, bool is_integer, bool is_input, bool is_output) {
    auto marabou_var = context->add_var(lb, ub, is_integer, is_input, is_output);
    stateIndexToMarabou.emplace(state_index, marabou_var);
    marabouToStateIndex.emplace(marabou_var, state_index);
    return marabou_var;
}

#endif

void StateIndexesInMarabou::extract_solution(const MARABOU_IN_PLAJA::Solution& solution, StateValues& solution_state, VariableIndex_type state_index, MarabouVarIndex_type marabou_var) const {

    const auto val = solution.get_value(marabou_var);

    if (modelInfo->is_integer(state_index)) {

        PLAJA_ASSERT(context->is_marked_integer(marabou_var))
        solution_state.assign_int<true>(state_index, static_cast<PLAJA::integer>(std::round(val)));

    } else {

        PLAJA_ASSERT(modelInfo->is_floating(state_index))
        PLAJA_ASSERT(not context->is_marked_integer(marabou_var))
        solution_state.assign_float<true>(state_index, val);

    }

    PLAJA_ASSERT(solution_state.is_valid(state_index))

}

void StateIndexesInMarabou::extract_solution(const MARABOU_IN_PLAJA::Solution& solution, StateValues& solution_state, VariableIndex_type state_index) const {
    extract_solution(solution, solution_state, state_index, to_marabou(state_index));
}

void StateIndexesInMarabou::extract_solution(const MARABOU_IN_PLAJA::Solution& solution, StateValues& solution_state, bool include_locs) const {

    for (auto& [state_index, marabou_var]: stateIndexToMarabou) {
        // if (is_fixed(modelInfo.get_id(state_index))) { continue; }
        if (not include_locs and modelInfo->is_location(state_index)) { continue; }
        extract_solution(solution, solution_state, state_index, marabou_var);
    }

}

std::unique_ptr<MarabouConstraint> StateIndexesInMarabou::encode_solution(const MARABOU_IN_PLAJA::Solution& solution) const {

    std::unique_ptr<ConjunctionInMarabou> solution_constraint(new ConjunctionInMarabou(*context));

    for (auto& [state_index, marabou_var]: stateIndexToMarabou) {
        // if (is_fixed(modelInfo.get_id(state_index))) { continue; }
        //
        const auto val = solution.get_value(marabou_var);
        //
        if (modelInfo->is_integer(state_index)) {
            PLAJA_ASSERT(context->is_marked_integer(marabou_var))
            solution_constraint->add_equality_constraint(marabou_var, static_cast<PLAJA::integer>(std::round(val)));
        } else {
            PLAJA_ASSERT(modelInfo->is_floating(state_index))
            PLAJA_ASSERT(not context->is_marked_integer(marabou_var))
            solution_constraint->add_equality_constraint(marabou_var, solution.get_value(marabou_var));
        }
    }

    return solution_constraint;
}

std::unique_ptr<MarabouConstraint> StateIndexesInMarabou::encode_state(const StateValues& state) const {

    std::unique_ptr<ConjunctionInMarabou> solution_constraint(new ConjunctionInMarabou(*context));

    for (auto& [state_index, marabou_var]: stateIndexToMarabou) {
        // if (is_fixed(modelInfo.get_id(state_index))) { continue; }
        //
        if (modelInfo->is_integer(state_index)) {
            PLAJA_ASSERT(context->is_marked_integer(marabou_var))
            solution_constraint->add_equality_constraint(marabou_var, state.get_int(state_index));
        } else {
            PLAJA_ASSERT(modelInfo->is_floating(state_index))
            PLAJA_ASSERT(not context->is_marked_integer(marabou_var))
            solution_constraint->add_equality_constraint(marabou_var, state.get_float(state_index));
        }
    }

    return solution_constraint;

}

void StateIndexesInMarabou::compute_substitution(std::unordered_map<MarabouVarIndex_type, MarabouVarIndex_type>& substitution, const StateIndexesInMarabou& other) const {
    for (auto it = iterator(); !it.end(); ++it) {
        PLAJA_ASSERT(not substitution.count(it.marabou_var()))
        substitution[it.marabou_var()] = other.to_marabou(it.state_index());
    }
}

std::unordered_map<MarabouVarIndex_type, MarabouVarIndex_type> StateIndexesInMarabou::compute_substitution(const StateIndexesInMarabou& other) const {
    std::unordered_map<MarabouVarIndex_type, MarabouVarIndex_type> substitution;
    compute_substitution(substitution, other);
    return substitution;
}

/**********************************************************************************************************************/

#ifndef NDEBUG

void StateIndexesInMarabou::init(bool do_locs, bool mark_input) {

    for (auto it = modelInfo->stateIndexIterator(true); !it.end(); ++it) {
        if (it.is_location() and not do_locs) { continue; }
        add(it.state_index(), it.is_location() or it.is_integer(), mark_input);
    }

}

#endif
