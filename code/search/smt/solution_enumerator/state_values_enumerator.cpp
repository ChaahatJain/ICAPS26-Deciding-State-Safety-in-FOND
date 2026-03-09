//
// This file is part of the PlaJA code base.
// Copyright (c) (2019 - 2024) Marcel Vinzent.
// See README.md in the top-level directory for licensing information.
//

#include "state_values_enumerator.h"
#include "../../../parser/ast/expression/non_standard/state_values_expression.h"
#include "../../../parser/ast/expression/non_standard/states_values_expression.h"
#include "../../../search/states/state_values.h"

StateValuesEnumerator::StateValuesEnumerator(const ModelInformation& model_info, const StatesValuesExpression& states_values):
    SolutionEnumeratorBase(model_info)
    , stateValues(&states_values)
    , index(0) {
}

StateValuesEnumerator::~StateValuesEnumerator() = default;

/* */

void StateValuesEnumerator::initialize() {
    set_initialized();
    index = 0;
}

std::unique_ptr<StateValues> StateValuesEnumerator::retrieve_solution() {
    PLAJA_ASSERT(is_initialized())
    if (index >= stateValues->get_size_states_values()) { return nullptr; }
    return std::make_unique<StateValues>(stateValues->get_state_values(index)->construct_state_values(get_constructor()));
}

std::list<StateValues> StateValuesEnumerator::enumerate_solutions() {
    std::list<StateValues> solutions;
    for (auto it = stateValues->init_state_values_iterator(); !it.end(); ++it) { solutions.push_back(it->construct_state_values(get_constructor())); }
    return solutions;
}

/* */

std::list<StateValues> StateValuesEnumerator::compute_solutions(const ModelInformation& model_info, const StatesValuesExpression& states_values) {
    StateValuesEnumerator enumerator(model_info, states_values);
    return enumerator.enumerate_solutions();
}