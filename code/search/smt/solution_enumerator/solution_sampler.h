//
// This file is part of the PlaJA code base.
// Copyright (c) (2019 - 2024) Marcel Vinzent.
// See README.md in the top-level directory for licensing information.
//

#ifndef PLAJA_SOLUTION_SAMPLER_H
#define PLAJA_SOLUTION_SAMPLER_H

#include <vector>
#include "../../../parser/using_parser.h"
#include "../../states/forward_states.h"
#include "../forward_smt_z3.h"
#include "../using_smt.h"
#include "solution_enumerator_base.h"

class SolutionSampler final: public SolutionEnumeratorBase {

private:

    Z3_IN_PLAJA::SMTSolver* solver;
    const ModelZ3* modelZ3;

    /* Variable (indexes) of interest */
    std::unique_ptr<Z3_IN_PLAJA::StateIndexes> stateIndexes;

    std::vector<VariableIndex_type> iterationOrder;
    using IterationIndexType = std::size_t;
    bool sampled;

    /* Recursive enumeration routines. */
    void enumerate_next(IterationIndexType it_index, StateValues& state_values);
    void enumerate_bool(IterationIndexType it_index, StateValues& state_values);
    void enumerate_integer(IterationIndexType it_index, PLAJA::integer lower_bound, PLAJA::integer upper_bound, StateValues& state_values);
    void enumerate_floating(IterationIndexType it_index, PLAJA::floating lower_bound, PLAJA::floating upper_bound, StateValues& state_values);

public:
    SolutionSampler(Z3_IN_PLAJA::SMTSolver& solver, const ModelZ3& model_z3);
    ~SolutionSampler() override;
    DELETE_CONSTRUCTOR(SolutionSampler)

    [[nodiscard]] std::unique_ptr<StateValues> sample() override;

};

#endif //PLAJA_SOLUTION_SAMPLER_H
