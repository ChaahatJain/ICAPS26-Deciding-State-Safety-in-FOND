//
// This file is part of the PlaJA code base.
// Copyright (c) (2023 - 2024) Chaahat Jain.
//

#include "vars_in_veritas.h"
#include "../information/model_information.h"
#include "../states/state_values.h"
#include "solver/solution_veritas.h"
#include "constraints_in_veritas.h"
#include "veritas_context.h"
#include <algorithm>
#include <cmath>

StateIndexesInVeritas::StateIndexesInVeritas(const ModelInformation& model_info, VERITAS_IN_PLAJA::Context& c):
    modelInfo(&model_info)
    , context(&c) {}

StateIndexesInVeritas::~StateIndexesInVeritas() = default;

VeritasVarIndex_type StateIndexesInVeritas::add(VariableIndex_type state_index, bool mark_integer) {
    PLAJA_ASSERT(not exists(state_index))
    auto veritas_var = context->add_var(modelInfo->get_lower_bound(state_index), modelInfo->get_upper_bound(state_index), mark_integer and modelInfo->is_integer(state_index));
    stateIndexToVeritas.emplace(state_index, veritas_var);
    veritasToStateIndex.emplace(veritas_var, state_index);
    return veritas_var;
}

void StateIndexesInVeritas::set(VariableIndex_type state_index, VeritasVarIndex_type veritas_var) {
    // PLAJA_ASSERT(not context->is_marked_output(veritas_var))
    // delete old mapping:
    auto it_state = stateIndexToVeritas.find(state_index);
    if (it_state != stateIndexToVeritas.end()) {
        PLAJA_ASSERT(exists_reverse(it_state->second))
        veritasToStateIndex.erase(it_state->second);
    }
    auto it_veritas = veritasToStateIndex.find(veritas_var);
    if (it_veritas != veritasToStateIndex.end()) {
        PLAJA_ASSERT(exists(it_veritas->second))
        stateIndexToVeritas.erase(it_veritas->second);
    }
    //
    PLAJA_ASSERT(not exists(state_index) or not exists_reverse(to_veritas(state_index)))
    PLAJA_ASSERT(not exists_reverse(veritas_var) or not exists(to_state_index(state_index)))
    //
    stateIndexToVeritas[state_index] = veritas_var;
    veritasToStateIndex[veritas_var] = state_index;
}

#ifndef NDEBUG

StateIndexesInVeritas::StateIndexesInVeritas(VERITAS_IN_PLAJA::Context& c):
    modelInfo(nullptr)
    , context(&c) {}

VeritasVarIndex_type StateIndexesInVeritas::add(VariableIndex_type state_index, VeritasFloating_type lb, VeritasFloating_type ub, bool is_integer) {
    auto veritas_var = context->add_var(lb, ub, is_integer);
    stateIndexToVeritas.emplace(state_index, veritas_var);
    veritasToStateIndex.emplace(veritas_var, state_index);
    return veritas_var;
}

#endif

void StateIndexesInVeritas::extract_solution(const VERITAS_IN_PLAJA::Solution& solution, StateValues& solution_state, VariableIndex_type state_index, VeritasVarIndex_type veritas_var) const {

    const auto val = solution.get_value(veritas_var);

    if (modelInfo->is_integer(state_index)) {

        PLAJA_ASSERT(context->is_marked_integer(veritas_var))
        solution_state.assign_int<true>(state_index, static_cast<PLAJA::integer>(std::round(val)));

    } else {

        PLAJA_ASSERT(modelInfo->is_floating(state_index))
        PLAJA_ASSERT(not context->is_marked_integer(veritas_var))
        solution_state.assign_float<true>(state_index, val);

    }

}

void StateIndexesInVeritas::extract_solution(const VERITAS_IN_PLAJA::Solution& solution, StateValues& solution_state, VariableIndex_type state_index) const {
    extract_solution(solution, solution_state, state_index, to_veritas(state_index));
}

void StateIndexesInVeritas::extract_solution(const VERITAS_IN_PLAJA::Solution& solution, StateValues& solution_state, bool include_locs) const {

    for (auto& [state_index, veritas_var]: stateIndexToVeritas) {
        // if (is_fixed(modelInfo.get_id(state_index))) { continue; }
        if (not include_locs and modelInfo->is_location(state_index)) { continue; }
        extract_solution(solution, solution_state, state_index, veritas_var);
    }

}

std::unique_ptr<VeritasConstraint> StateIndexesInVeritas::encode_solution(const VERITAS_IN_PLAJA::Solution& solution) const {

    std::unique_ptr<ConjunctionInVeritas> solution_constraint(new ConjunctionInVeritas(*context));

    for (auto& [state_index, veritas_var]: stateIndexToVeritas) {
        // if (is_fixed(modelInfo.get_id(state_index))) { continue; }
        //
        const auto val = solution.get_value(veritas_var);
        //
        if (modelInfo->is_integer(state_index)) {
            PLAJA_ASSERT(context->is_marked_integer(veritas_var))
            solution_constraint->add_equality_constraint(veritas_var, static_cast<PLAJA::integer>(std::round(val)));
        } else {
            PLAJA_ASSERT(modelInfo->is_floating(state_index))
            PLAJA_ASSERT(not context->is_marked_integer(veritas_var))
            solution_constraint->add_equality_constraint(veritas_var, solution.get_value(veritas_var));
        }
    }

    return solution_constraint;
}

std::unique_ptr<VeritasConstraint> StateIndexesInVeritas::encode_state(const StateValues& state) const {

    std::unique_ptr<ConjunctionInVeritas> solution_constraint(new ConjunctionInVeritas(*context));

    for (auto& [state_index, veritas_var]: stateIndexToVeritas) {
        // if (is_fixed(modelInfo.get_id(state_index))) { continue; }
        //
        if (modelInfo->is_integer(state_index)) {
            PLAJA_ASSERT(context->is_marked_integer(veritas_var))
            solution_constraint->add_equality_constraint(veritas_var, state.get_int(state_index));
        } else {
            PLAJA_ASSERT(modelInfo->is_floating(state_index))
            PLAJA_ASSERT(not context->is_marked_integer(veritas_var))
            solution_constraint->add_equality_constraint(veritas_var, state.get_float(state_index));
        }
    }

    return solution_constraint;

}

void StateIndexesInVeritas::compute_substitution(std::unordered_map<VeritasVarIndex_type, VeritasVarIndex_type>& substitution, const StateIndexesInVeritas& other) const {
    for (auto it = iterator(); !it.end(); ++it) {
        PLAJA_ASSERT(not substitution.count(it.veritas_var()))
        substitution[it.veritas_var()] = other.to_veritas(it.state_index());
    }
}

std::unordered_map<VeritasVarIndex_type, VeritasVarIndex_type> StateIndexesInVeritas::compute_substitution(const StateIndexesInVeritas& other) const {
    std::unordered_map<VeritasVarIndex_type, VeritasVarIndex_type> substitution;
    compute_substitution(substitution, other);
    return substitution;
}
