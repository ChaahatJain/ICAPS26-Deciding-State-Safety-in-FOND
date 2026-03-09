#ifndef STATE_H
#define STATE_H

#include <memory>
#include "../states/state_values.h"
#include "state_registry.h"

namespace STATE_REGISTRY { struct SetState; } // PLAJA technical internals

class State final: public StateBase {
    friend class StateRegistry;

    friend STATE_REGISTRY::SetState;

private:
    // static interface:
    // static std::size_t get_state_size(const StateBase* state);
    // static integer get(const StateBase* state, VariableIndex_type index);
    // static void set(StateBase* state, VariableIndex_type var, integer val);

    StateRegistry::PackedStateBin bin;
    StateRegistry* stateRegistry;
    const IntPacker* intPacker;
    StateID_type id;

    [[nodiscard]] inline const StateRegistry::PackedStateBin& get_bin() const { return bin; }

    [[nodiscard]] inline const StateRegistry* get_registry() const { return stateRegistry; }

    State(StateRegistry::PackedStateBin bin, StateRegistry* registry, StateID_type id);
public:
    ~State() final;
    DELETE_CONSTRUCTOR(State)

    [[nodiscard]] inline StateID_type get_id() const { return id; }

    [[nodiscard]] inline StateID_type get_id_value() const { return get_id(); }

    // interface:

    [[nodiscard]] std::size_t get_int_state_size() const override { return stateRegistry->get_int_state_size(); }

    [[nodiscard]] std::size_t get_floating_state_size() const override { return stateRegistry->get_floating_state_size(); }

    [[nodiscard]] PLAJA::integer get_int(VariableIndex_type var) const override;

    [[nodiscard]] PLAJA::floating get_float(VariableIndex_type var) const override;

    void set_int(VariableIndex_type var, PLAJA::integer val) override;

    void set_float(VariableIndex_type var, PLAJA::floating val) override;

    // for state construction

    [[nodiscard]] inline static State to_construct(StateRegistry& state_registry) { return { state_registry.push_state_construct(), &state_registry, STATE::none }; }

    [[nodiscard]] inline std::unique_ptr<State> to_construct() const { return std::unique_ptr<State>(new State(stateRegistry->push_state_construct(bin), stateRegistry, STATE::none)); }

    inline void finalize() {
        PLAJA_ASSERT(id == STATE::none)
        PLAJA_ASSERT(bin.g_int() == stateRegistry->intDataPool[stateRegistry->intDataPool.size() - 1])
        PLAJA_ASSERT(not bin.g_float() or bin.g_float() == (*stateRegistry->floatDataPool)[stateRegistry->floatDataPool->size() - 1])
        id = stateRegistry->insert_id_or_pop_state();
        bin = StateRegistry::PackedStateBin(stateRegistry->intDataPool[get_id_value()], stateRegistry->floatDataPool ? (*stateRegistry->floatDataPool)[get_id_value()] : nullptr);
    }

    [[nodiscard]] inline std::unique_ptr<State> to_state(const StateValues& state_values) const { return stateRegistry->set_state(state_values); }

    [[nodiscard]] inline State to_state() const { return { bin, stateRegistry, id }; }

    [[nodiscard]] inline std::unique_ptr<State> to_ptr() const { return std::unique_ptr<State>(new State(bin, stateRegistry, id)); }

    [[nodiscard]] StateValues to_state_values() const override { return StateValues(*this); }

    [[nodiscard]] inline StateID_type move_to_reg(StateRegistry& different_reg) const {
        PLAJA_ASSERT(stateRegistry->get_state_size() == different_reg.get_state_size())
        different_reg.push_state_construct(bin);
        return different_reg.insert_id_or_pop_state();
    }

    /* Look up whether (sub-)state is contained in registry. */
    [[nodiscard]] inline StateID_type contained_in(const StateRegistry& state_registry) const {
        PLAJA_ASSERT(state_registry.get_int_state_size() <= get_int_state_size())
        PLAJA_ASSERT(stateRegistry->get_floating_state_size() <= get_floating_state_size())
        return state_registry.contains(bin);
    }

};

/** hashing for states ************************************************************************************************/

namespace utils {

    void feed(HashState& hash_state, const State& state) = delete; // hashing state object directly is unintended

    inline void feed(HashState& hash_state, const State* state) { hash_state.feed(state->get_id()); }

    inline void feed(HashState& hash_state, const std::unique_ptr<State>& state) { hash_state.feed(state->get_id()); }

    inline void feed(HashState& hash_state, const std::unique_ptr<const State>& state) { hash_state.feed(state->get_id()); }

}

#endif //STATE_H
