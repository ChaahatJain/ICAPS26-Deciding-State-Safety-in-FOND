//
// This file is part of the PlaJA code base.
// Copyright (c) (2019 - 2020) Marcel Vinzent.
// See README.md in the top-level directory for licensing information.
//

#ifndef PLAJA_SOLUTION_ENUMERATOR_BS_H
#define PLAJA_SOLUTION_ENUMERATOR_BS_H

#include "../../../parser/using_parser.h"
#include "../forward_smt_z3.h"
#include "../using_smt.h"
#include "solution_enumerator_base.h"

/** Solution enumerator Binary Search */
class SolutionEnumeratorBS final: public SolutionEnumeratorBase {
private:
    Z3_IN_PLAJA::SMTSolver* solver;
    const ModelZ3* modelZ3;

    /* Variable (indexes) of interest */
    std::unique_ptr<Z3_IN_PLAJA::StateIndexes> stateIndexes;

    /* Recursive enumeration routines. */
    void enumerate_next(VariableIndex_type state_index, StateValues& state_values);
    void enumerate_bool(VariableIndex_type state_index, StateValues& state_values);
    void enumerate_integer(VariableIndex_type state_index, PLAJA::integer lower_bound, PLAJA::integer upper_bound, StateValues& state_values);
    void enumerate_floating(VariableIndex_type state_index, PLAJA::floating lower_bound, PLAJA::floating upper_bound, StateValues& state_values);

public:
    SolutionEnumeratorBS(Z3_IN_PLAJA::SMTSolver& solver, const ModelZ3& model_z3);
    ~SolutionEnumeratorBS() override;
    DELETE_CONSTRUCTOR(SolutionEnumeratorBS)

    void initialize() override;

    /** Compute solutions given the ground truth of the solver (variable bounds must have been added). */
    static std::list<StateValues> compute_solutions(Z3_IN_PLAJA::SMTSolver& solver, const ModelZ3& model_z3, StateValues&& constructor);

};

#endif //PLAJA_SOLUTION_ENUMERATOR_BS_H
