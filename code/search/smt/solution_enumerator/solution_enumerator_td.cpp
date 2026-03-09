//
// This file is part of the PlaJA code base.
// Copyright (c) (2019 - 2020) Marcel Vinzent.
// See README.md in the top-level directory for licensing information.
//

#include "solution_enumerator_td.h"
#include "../../states/state_values.h"
#include "../model/model_z3.h"
#include "../solver/smt_solver_z3.h"
#include "../solver/solution_z3.h"
#include "../vars_in_z3.h"

SolutionEnumeratorTD::SolutionEnumeratorTD(Z3_IN_PLAJA::SMTSolver& solver_ref, const ModelZ3& model_z3):
    SolutionEnumeratorBase(model_z3.get_model_info())
    , solver(&solver_ref)
    , stateIndexes(model_z3.get_src_vars().cache_state_indexes(false)) {
    PLAJA_ASSERT(static_cast<const z3::context&>(solver->get_context_z3()) == model_z3.get_context_z3())
}

SolutionEnumeratorTD::~SolutionEnumeratorTD() = default;

/* */

void SolutionEnumeratorTD::enumerate(StateValues& state_values) { // NOLINT(misc-no-recursion)
    if (solver->check()) {
        auto solution = solver->get_solution();
        /* Extract solution. */
        state_values.refresh_written_to();
        stateIndexes->extract_solution(*solution, state_values);
        /* Add solution. */
        solutions.emplace_back(state_values);
        /* Proceed excluding solution (i.e., distribute or-not-disjunction over recursion tree). */
        for (auto it = stateIndexes->iterator(); !it.end(); ++it) {
            solver->push();
            solver->add(it.state_index_z3() != solution->eval(it.state_index_z3(), true));
            enumerate(state_values);
            solver->pop();
        }
    }
}

/* */

void SolutionEnumeratorTD::initialize() {
    solutions.clear();
    auto constructor = get_constructor();
    enumerate(constructor);
    set_initialized();
}

/* */

std::list<StateValues> SolutionEnumeratorTD::compute_solutions(Z3_IN_PLAJA::SMTSolver& solver, const ModelZ3& model_z3, StateValues&& constructor) {
    SolutionEnumeratorTD solution_enumerator(solver, model_z3);
    solution_enumerator.set_constructor(std::make_unique<StateValues>(std::move(constructor)));
    return solution_enumerator.enumerate_solutions();
}
