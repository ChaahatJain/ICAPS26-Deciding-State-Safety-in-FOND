//
// This file is part of the PlaJA code base.
// Copyright (c) (2019 - 2021) Marcel Vinzent.
// See README.md in the top-level directory for licensing information.
//

#include "state_values.h"
#include "../../search/fd_adaptions/state.h"

StateValues::StateValues(const State& state):
    StateBase(state.size()) {
    VariableIndex_type state_index = 0;
    // int
    const auto int_state_size = state.get_int_state_size();
    valuesInt.reserve(int_state_size);
    for (; state_index < int_state_size; ++state_index) { valuesInt.push_back(state.get_int(state_index)); }
    // floating
    const auto floating_state_size = state.get_floating_state_size();
    valuesFloating.reserve(floating_state_size);
    for (; state_index < int_state_size + floating_state_size; ++state_index) { valuesFloating.push_back(state.get_float(state_index)); }
}

StateValues::StateValues(std::vector<PLAJA::integer> values_int, std::vector<PLAJA::floating> values_floating):
    StateBase(values_int.size() + values_floating.size())
    , valuesInt(std::move(values_int))
    , valuesFloating(std::move(values_floating)) {
}

StateValues::StateValues(std::size_t int_state_size, std::size_t floating_state_size):
    StateBase(int_state_size + floating_state_size)
    , valuesInt(int_state_size)
    , valuesFloating(floating_state_size) {

}

//

PLAJA::integer StateValues::get_int(VariableIndex_type var) const {
    PLAJA_ASSERT(var < get_int_state_size())
    PLAJA_ASSERT(var < valuesInt.size())
    return valuesInt[var];
}

PLAJA::floating StateValues::get_float(VariableIndex_type var) const {
    PLAJA_ASSERT(get_int_state_size() <= var)
    PLAJA_ASSERT(var < get_int_state_size() + get_floating_state_size())
    auto var_offset = var - get_int_state_size();
    PLAJA_ASSERT(var_offset < valuesFloating.size())
    return valuesFloating[var_offset];
}

//

void StateValues::set_int(VariableIndex_type var, PLAJA::integer val) {
    PLAJA_ASSERT(var < get_int_state_size())
    PLAJA_ASSERT(var < valuesInt.size())
    // PLAJA_ASSERT(is_valid(var, val)) // Not possible while using StateValues for abstract states.
    valuesInt[var] = val;
}

void StateValues::set_float(VariableIndex_type var, PLAJA::floating val) {
    PLAJA_ASSERT(get_int_state_size() <= var)
    PLAJA_ASSERT(var < get_int_state_size() + get_floating_state_size())
    PLAJA_ASSERT(is_valid(var, val))
    auto var_offset = var - get_int_state_size();
    PLAJA_ASSERT(var_offset < valuesFloating.size())
    valuesFloating[var_offset] = val;
}

/* Debug. */

// #ifndef NDEBUG

#include "../../parser/ast/model.h"
#include "../../search/information/model_information.h"

/* Extern. */
namespace PLAJA_GLOBAL { extern const Model* currentModel; }

namespace STATE_VALUES {
    std::unique_ptr<StateRegistry> stateRegistry(nullptr);
    const Model* refModel = nullptr;
}

std::unique_ptr<const State> StateValues::to_state() const {
    if (!STATE_VALUES::stateRegistry || STATE_VALUES::refModel != PLAJA_GLOBAL::currentModel) {
        const ModelInformation& model_info = PLAJA_GLOBAL::currentModel->get_model_information();
        STATE_VALUES::stateRegistry = std::make_unique<StateRegistry>(model_info.get_ranges_int(), model_info.get_lower_bounds_int(), model_info.get_upper_bounds_int(), valuesFloating.size());
        STATE_VALUES::refModel = PLAJA_GLOBAL::currentModel;
    }

    return STATE_VALUES::stateRegistry->set_state(*this);
}

// #endif

