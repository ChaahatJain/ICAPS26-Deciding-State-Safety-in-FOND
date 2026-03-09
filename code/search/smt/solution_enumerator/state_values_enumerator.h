//
// This file is part of the PlaJA code base.
// Copyright (c) (2019 - 2024) Marcel Vinzent.
// See README.md in the top-level directory for licensing information.
//

#ifndef PLAJA_STATE_VALUES_ENUMERATOR_H
#define PLAJA_STATE_VALUES_ENUMERATOR_H

#include "solution_enumerator_base.h"
#include "../../information/forward_information.h"

class StatesValuesExpression;

class StateValuesEnumerator final: public SolutionEnumeratorBase {

private:
    const StatesValuesExpression* stateValues;
    std::size_t index;

public:
    StateValuesEnumerator(const ModelInformation& model_info, const StatesValuesExpression& states_values);
    ~StateValuesEnumerator() override;
    DELETE_CONSTRUCTOR(StateValuesEnumerator)

    void initialize() override;

    [[nodiscard]] std::unique_ptr<StateValues> retrieve_solution() override;

    [[nodiscard]] std::list<StateValues> enumerate_solutions() override;

    [[nodiscard]] static std::list<StateValues> compute_solutions(const ModelInformation& model_info, const StatesValuesExpression& states_values);

};

#endif //PLAJA_STATE_VALUES_ENUMERATOR_H
