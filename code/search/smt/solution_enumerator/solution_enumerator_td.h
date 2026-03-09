//
// This file is part of the PlaJA code base.
// Copyright (c) (2019 - 2020) Marcel Vinzent.
// See README.md in the top-level directory for licensing information.
//

#ifndef PLAJA_SOLUTION_ENUMERATOR_TD_H
#define PLAJA_SOLUTION_ENUMERATOR_TD_H

#include "../forward_smt_z3.h"
#include "../using_smt.h"
#include "solution_enumerator_base.h"

/** Solution enumerator Tree Disjunction */
class SolutionEnumeratorTD final: public SolutionEnumeratorBase { // TODO untested

private:
    Z3_IN_PLAJA::SMTSolver* solver;

    /* Variable (indexes) of interest */
    std::unique_ptr<Z3_IN_PLAJA::StateIndexes> stateIndexes;

    /* Recursive enumeration routines. */
    void enumerate(StateValues& constructor);

public:
    SolutionEnumeratorTD(Z3_IN_PLAJA::SMTSolver& solver, const ModelZ3& model_z3);
    ~SolutionEnumeratorTD() override;
    DELETE_CONSTRUCTOR(SolutionEnumeratorTD)

    void initialize() override;

    /** Compute solutions given the ground truth of the solver (variable bounds must have been added). */
    static std::list<StateValues> compute_solutions(Z3_IN_PLAJA::SMTSolver& solver, const ModelZ3& model_z3, StateValues&& constructor);

};


#endif //PLAJA_SOLUTION_ENUMERATOR_TD_H
