//
// This file is part of the PlaJA code base.
// Copyright (c) (2019 - 2020) Marcel Vinzent.
// See README.md in the top-level directory for licensing information.
//

#ifndef PLAJA_SOLUTION_ITERATOR_Z3_H
#define PLAJA_SOLUTION_ITERATOR_Z3_H

#include "../../../parser/using_parser.h"
#include "../../../utils/default_constructors.h"
#include "../forward_smt_z3.h"
#include "../using_smt.h"
#include "solution_enumerator_base.h"

class SolutionEnumeratorZ3 final: public SolutionEnumeratorBase {

private:
    Z3_IN_PLAJA::SMTSolver* solver;
    const ModelZ3* modelZ3;

    /* Variables of interest. */
    std::unique_ptr<Z3_IN_PLAJA::StateIndexes> stateIndexes;
    std::unique_ptr<Z3_IN_PLAJA::StateVarsInZ3> stateVars;

public:
    SolutionEnumeratorZ3(Z3_IN_PLAJA::SMTSolver& solver, const ModelZ3& model_z3);
    ~SolutionEnumeratorZ3() override;
    DELETE_CONSTRUCTOR(SolutionEnumeratorZ3)

    void set_variables_of_interest(const std::vector<VariableID_type>& var_ids, bool use_target_vars);

    void initialize() override;

    [[nodiscard]] std::unique_ptr<StateValues> retrieve_solution() override;

    /** * @return true if a new solution was found, false otherwise. */
    [[nodiscard]] bool retrieve_solution(StateValues& solution);

    [[nodiscard]] std::list<StateValues> enumerate_solutions() override;

    /** Compute solutions given the ground truth of the solver (variable bounds must have been added). */
    [[nodiscard]] static std::list<StateValues> compute_solutions(Z3_IN_PLAJA::SMTSolver& solver, const ModelZ3& model_z3, StateValues&& constructor);

};

#endif //PLAJA_SOLUTION_ITERATOR_Z3_H
