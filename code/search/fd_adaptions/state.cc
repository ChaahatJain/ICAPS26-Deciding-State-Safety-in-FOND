
#include "state.h"

// static

// std::size_t State::get_state_size(const StateBase* state) { return static_cast<const State*>(state)->get_state_size(); } // NOLINT(cppcoreguidelines-pro-type-static-cast-downcast)

// integer State::get(const StateBase* state, VariableIndex_type index) { return static_cast<const State*>(state)->get(index); } // NOLINT(cppcoreguidelines-pro-type-static-cast-downcast)

// void State::set(StateBase* state, VariableIndex_type var, integer val) { static_cast<State*>(state)->set(var, val); } // NOLINT(cppcoreguidelines-pro-type-static-cast-downcast)

//

State::State(StateRegistry::PackedStateBin bin_, StateRegistry* registry, StateID_type id):
    StateBase(registry->get_state_size())
    , bin(bin_)
    , stateRegistry(registry)
    , intPacker(&registry->get_int_packer())
    , id(id) {
}

State::~State() {

    if (not stateRegistry) { // free buffer for unregistered states

        PLAJA_ASSERT(id == STATE::none)

        bin.free();

    }

}

/*
State::State(PackedStateBin* buffer, const IntPacker* state_packer):
        StateBase(state_packer->get_state_size(), &State::get, &State::set, &State::get_state_size),
        buffer(buffer),
        stateRegistry(nullptr), statePacker(state_packer),
        id(StateID::no_state) {
}
 */

PLAJA::integer State::get_int(VariableIndex_type var) const {
    PLAJA_ASSERT(var < get_int_state_size())
    return intPacker->get(bin.g_int(), var);
}

PLAJA::floating State::get_float(VariableIndex_type var) const {
    PLAJA_ASSERT(get_int_state_size() <= var)
    PLAJA_ASSERT(var < get_int_state_size() + get_floating_state_size())
    auto var_offset = var - get_int_state_size();
    PLAJA_ASSERT(var_offset < get_floating_state_size())
    return bin.g_float()[var_offset];
}

//

void State::set_int(VariableIndex_type var, PLAJA::integer val) {
    PLAJA_ASSERT(id == STATE::none)
    PLAJA_ASSERT(var < get_int_state_size())
    PLAJA_ASSERT(is_valid(var, val))
    intPacker->set(bin.g_int(), var, val);
}

void State::set_float(VariableIndex_type var, PLAJA::floating val) {
    PLAJA_ASSERT(id == STATE::none)
    PLAJA_ASSERT(get_int_state_size() <= var)
    PLAJA_ASSERT(var < get_int_state_size() + get_floating_state_size())
    PLAJA_ASSERT(is_valid(var, val))
    auto var_offset = var - get_int_state_size();
    PLAJA_ASSERT(var_offset < get_floating_state_size())
    bin.g_float()[var_offset] = val;
}
