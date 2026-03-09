//
// This file is part of the PlaJA code base.
// Copyright (c) (2019 - 2022) Marcel Vinzent.
// See README.md in the top-level directory for licensing information.
//

#ifndef PLAJA_SOLUTION_ENUMERATOR_NAIVE_H
#define PLAJA_SOLUTION_ENUMERATOR_NAIVE_H

#include "../../../parser/using_parser.h"
#include "solution_enumerator_base.h"

class Expression;

class SolutionEnumeratorNaive final: public SolutionEnumeratorBase {
private:
    const Expression* condition;

    void compute_solutions_rec(VariableIndex_type state_index, StateValues& state_values);

public:
    SolutionEnumeratorNaive(const ModelInformation& model_info, const Expression& condition);
    ~SolutionEnumeratorNaive() override;
    DELETE_CONSTRUCTOR(SolutionEnumeratorNaive)

    void initialize() override;

    static std::list<StateValues> compute_solutions(const Expression& condition, const ModelInformation& model_info);

};


#endif //PLAJA_SOLUTION_ENUMERATOR_NAIVE_H
