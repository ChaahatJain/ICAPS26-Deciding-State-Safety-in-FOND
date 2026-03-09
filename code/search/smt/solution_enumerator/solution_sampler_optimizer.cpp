//
// Created by solution_sampler_optimizer on 2024.
//

#include "solution_sampler_optimizer.h"
#include "../../states/state_values.h"
#include "../model/model_z3.h"
#include "../solver/smt_solver_z3.h"
#include "../solver/solution_z3.h"
#include "../vars_in_z3.h"

SolutionSamplerOptimizer::SolutionSamplerOptimizer(Z3_IN_PLAJA::SMTSolver& solver_ref, const ModelZ3& model_z3):
    SolutionEnumeratorBase(model_z3.get_model_info()),
    solver(&solver_ref), modelZ3(&model_z3),
    stateIndexes(model_z3.get_src_vars().cache_state_indexes(false)),
    stateVars(nullptr) {// only for customized state indexes
    z3::set_param("smt.phase_selection", 5);
    z3::set_param("sat.phase", "random");
}

SolutionSamplerOptimizer::~SolutionSamplerOptimizer() = default;

/**
 *  @brief Samples a solution from the solver,
 *  and re-adds its negation as a constraint to solver
 *  to make sure the next solution is different.
 */
std::unique_ptr<StateValues> SolutionSamplerOptimizer::sample() {
    if (not solver->check()) { return nullptr; }// Problem is unsat.
    const auto solution = solver->get_solution();

    // New solution found, extract and constraint for next solution
    auto solution_state = std::make_unique<StateValues>(get_constructor());
    stateIndexes->extract_solution(*solution, *solution_state);
    // solver->add(!stateIndexes->encode_solution(*solution));
    // ensures we generate new solution next sample.
    z3::set_param("smt.random_seed", seed++);
    z3::set_param("sls.random_seed", seed++);
    z3::set_param("sat.random_seed", seed++);

    return solution_state;
}
