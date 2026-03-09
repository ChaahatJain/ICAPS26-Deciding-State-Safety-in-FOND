//
// This file is part of the PlaJA code base.
// Copyright (c) (2019 - 2020) Marcel Vinzent.
// See README.md in the top-level directory for licensing information.
//

#include <z3++.h>
#include "solution_enumerator_z3.h"
#include "../../states/state_values.h"
#include "../model/model_z3.h"
#include "../solver/smt_solver_z3.h"
#include "../solver/solution_z3.h"
#include "../vars_in_z3.h"

SolutionEnumeratorZ3::SolutionEnumeratorZ3(Z3_IN_PLAJA::SMTSolver& solver_ref, const ModelZ3& model_z3):
    SolutionEnumeratorBase(model_z3.get_model_info())
    , solver(&solver_ref)
    , modelZ3(&model_z3)
    , stateIndexes(model_z3.get_src_vars().cache_state_indexes(false))
    , stateVars(nullptr) { // only for customized state indexes
}

SolutionEnumeratorZ3::~SolutionEnumeratorZ3() = default;

/* */

void SolutionEnumeratorZ3::set_variables_of_interest(const std::vector<VariableID_type>& var_ids, bool use_target_vars) {
    /* Compute id to expr mapping. */
    stateVars = std::make_unique<Z3_IN_PLAJA::StateVarsInZ3>(*modelZ3);
    const auto& state_vars = use_target_vars ? modelZ3->get_target_vars() : modelZ3->get_src_vars();
    for (const VariableID_type var_id: var_ids) { stateVars->set(var_id, state_vars.to_z3(var_id)); }
    /* */
    stateIndexes = stateVars->cache_state_indexes(false);
}

/* */

void SolutionEnumeratorZ3::initialize() { set_initialized(); }

bool SolutionEnumeratorZ3::retrieve_solution(StateValues& solution_state) {
    if (not solver->check()) { return false; } // Problem is unsat.
    auto solution = solver->get_solution();
    /* New solution found, extract and constraint for next solution ... */
    stateIndexes->extract_solution(*solution, solution_state);
    solver->add(!stateIndexes->encode_solution(*solution));
    return true;
}

std::unique_ptr<StateValues> SolutionEnumeratorZ3::retrieve_solution() {
    if (not solver->check()) { return nullptr; } // Problem is unsat.
    auto solution = solver->get_solution();
    /* New solution found, extract and constraint for next solution ... */
    auto solution_state = std::make_unique<StateValues>(get_constructor());
    stateIndexes->extract_solution(*solution, *solution_state);
    solver->add(!stateIndexes->encode_solution(*solution));
    return solution_state;
}

std::list<StateValues> SolutionEnumeratorZ3::enumerate_solutions() {
    std::list<StateValues> solutions;
    solutions.push_back(get_constructor());
    while (retrieve_solution(solutions.back())) { solutions.push_back(get_constructor()); }
    solutions.pop_back(); // Pop last state which does not correspond to a new state.
    return solutions;
}

/* */

std::list<StateValues> SolutionEnumeratorZ3::compute_solutions(Z3_IN_PLAJA::SMTSolver& solver, const ModelZ3& model_z3, StateValues&& constructor) {
    SolutionEnumeratorZ3 solution_enumerator(solver, model_z3);
    solution_enumerator.set_constructor(std::make_unique<StateValues>(std::move(constructor)));
    return solution_enumerator.enumerate_solutions();
}