//
// Created by solution_sampler_optimizer on 2024.
//

#ifndef SOLUTION_SAMPLER_OPTIMIZER_H
#define SOLUTION_SAMPLER_OPTIMIZER_H

#include "../../../utils/rng.h"
#include "../forward_smt_z3.h"
#include "../solver/smt_optimizer_z3.h"
#include "solution_enumerator_base.h"
#include "../../../globals.h"

class SolutionSamplerOptimizer: public SolutionEnumeratorBase {

    Z3_IN_PLAJA::SMTSolver* solver;
    const ModelZ3* modelZ3;

    /* Variables of interest. */
    std::unique_ptr<Z3_IN_PLAJA::StateIndexes> stateIndexes;
    std::unique_ptr<Z3_IN_PLAJA::StateVarsInZ3> stateVars;

    // seed used for generating random solutions.
    int seed = PLAJA_GLOBAL::rng->sample_signed(0,100000);

public:
    SolutionSamplerOptimizer(Z3_IN_PLAJA::SMTSolver& solver, const ModelZ3& model_z3);
    ~SolutionSamplerOptimizer() override;
    std::unique_ptr<StateValues> sample() override;
};

#endif//SOLUTION_SAMPLER_OPTIMIZER_H
