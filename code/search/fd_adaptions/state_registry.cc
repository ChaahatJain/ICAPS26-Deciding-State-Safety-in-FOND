
#include "state.h"
#include "state_registry.h"

namespace STATE_REGISTRY {

    template<typename Bin_t>
    inline Bin_t* generate_bin_constructor(std::size_t bin_size) {
        auto* bin_constructor = new Bin_t[bin_size];
        for (std::size_t i = 0; i < bin_size; ++i) { bin_constructor[i] = 0; } // set to 0 to properly maintain state set, i.e., hash and detect duplicates
        return bin_constructor;
    }

}

/**********************************************************************************************************************/

StateRegistry::StateRegistry(const std::vector<Range_type>& ranges, const std::vector<PLAJA::integer>& lower_bounds, const std::vector<PLAJA::integer>& upper_bounds, std::size_t num_floats):
    intPacker(ranges, lower_bounds, upper_bounds)
    , sizeFloatingState(num_floats)
    , floatConstructor(sizeFloatingState > 0 ? STATE_REGISTRY::generate_bin_constructor<PLAJA::floating>(sizeFloatingState) : nullptr)
    , intDataPool(intPacker.get_num_bins())
    , floatDataPool(sizeFloatingState > 0 ? new segmented_vector::SegmentedArrayVector<PLAJA::floating>(sizeFloatingState) : nullptr)
    , registeredStates(0, StateIdSemanticHash(*this),
                       StateIdSemanticEqual(*this)) {

    // model_info_concrete.get_ranges(), model_info_concrete.get_lowerBounds(), model_info_concrete.get_upperBounds()
}

StateRegistry::~StateRegistry() { delete[] floatConstructor; }

StateID_type StateRegistry::insert_id_or_pop_state() {
    PLAJA_ASSERT(not floatDataPool or intDataPool.size() == floatDataPool->size())
    /*
      Attempt to insert a StateID for the last state of state_data_pool
      if none is present yet. If this fails (another entry for this state
      is present), we have to remove the duplicate entry from the
      state data pool.
    */
    const auto id = PLAJA_UTILS::cast_numeric<StateID_type>(intDataPool.size() - 1);
    const std::pair<StateIdSet::iterator, bool> result = registeredStates.insert(id);
    const bool is_new_entry = result.second;
    if (not is_new_entry) {
        intDataPool.pop_back();
        if (floatDataPool) { floatDataPool->pop_back(); }
    }
    assert(registeredStates.size() == intDataPool.size());
    return *result.first;
}

std::unique_ptr<State> StateRegistry::get_state(StateID_type id) {
    PLAJA_ASSERT(id != STATE::none)
    return std::unique_ptr<State>(new State(PackedStateBin(intDataPool[id], floatDataPool ? (*floatDataPool)[id] : nullptr), this, id));
}

State StateRegistry::lookup_state(StateID_type id) {
    PLAJA_ASSERT(id != STATE::none)
    return { PackedStateBin(intDataPool[id], floatDataPool ? (*floatDataPool)[id] : nullptr), this, id };
}

StateID_type StateRegistry::contains(const PackedStateBin& buffer) const {
    PLAJA_ASSERT(not floatDataPool or intDataPool.size() == floatDataPool->size())
    // int
    intDataPool.push_back(intPacker.get_bin_constructor()); // to properly maintain state set
    const StateID_type id = intDataPool.size() - 1; // NOLINT(cppcoreguidelines-narrowing-conversions)
    auto* buffer_int = intDataPool[id];
    for (auto i = 0; i < get_int_state_size(); ++i) { intPacker.set(buffer_int, i, intPacker.get(buffer.g_int(), i)); }
    // floating
    if (floatDataPool) {
        floatDataPool->push_back(floatConstructor); // to properly maintain state set
        auto* buffer_float = (*floatDataPool)[id];
        for (auto i = 0; i < get_floating_state_size(); ++i) { buffer_float[i] = buffer.g_float()[i]; }
    }
    //
    auto result = registeredStates.find(id);
    intDataPool.pop_back();
    if (floatDataPool) { floatDataPool->pop_back(); }
    if (result == registeredStates.end()) { return STATE::none; }
    else { return *result; }
}

/**********************************************************************************************************************/

namespace STATE_REGISTRY {

    struct SetState {

        template<typename Output_t>
        inline static Output_t set_state(StateRegistry* state_registry, const StateValues& values) {

            /* Int. */
            auto& int_data_pool = state_registry->intDataPool;
            int_data_pool.push_back(state_registry->intPacker.get_bin_constructor());
            auto* buffer_int = int_data_pool[int_data_pool.size() - 1];
            auto int_state_size = values.get_int_state_size();
            PLAJA_ASSERT(int_state_size == state_registry->get_int_state_size())
            for (auto i = 0; i < int_state_size; ++i) { state_registry->intPacker.set(buffer_int, i, values.get_int(i)); }

            /* Floating. */
            if (state_registry->floatDataPool) {
                auto& float_data_pool = *state_registry->floatDataPool;
                float_data_pool.push_back(state_registry->floatConstructor);
                auto* buffer_float = float_data_pool[float_data_pool.size() - 1];
                auto float_state_size = values.get_floating_state_size();
                PLAJA_ASSERT(float_state_size == state_registry->get_floating_state_size())
                for (auto i = 0; i < float_state_size; ++i) { buffer_float[i] = values.get_float(i + int_state_size); }
            }

            const StateID_type id = state_registry->insert_id_or_pop_state();

            if constexpr (PLAJA_UTILS::is_static_type<Output_t, std::unique_ptr<State>>()) {
                return std::unique_ptr<State>(new State(StateRegistry::PackedStateBin(int_data_pool[id], state_registry->floatDataPool ? (*state_registry->floatDataPool)[id] : nullptr), state_registry, id));
            } else if constexpr (PLAJA_UTILS::is_static_type<Output_t, State>()) {
                return State(StateRegistry::PackedStateBin(int_data_pool[id], state_registry->floatDataPool ? (*state_registry->floatDataPool)[id] : nullptr), state_registry, id); // NOLINT(modernize-return-braced-init-list)
            } else {
                return id;
            }

        }

    };

}

/**********************************************************************************************************************/

std::unique_ptr<State> StateRegistry::set_state(const StateValues& values) { return STATE_REGISTRY::SetState::set_state<std::unique_ptr<State>>(this, values); }

State StateRegistry::add_state(const StateValues& values) { return STATE_REGISTRY::SetState::set_state<State>(this, values); }

StateID_type StateRegistry::add_state_id(const StateValues& values) { return STATE_REGISTRY::SetState::set_state<StateID_type>(this, values); }

void StateRegistry::unregister_state_if_possible(StateID_type id) {

    PLAJA_ASSERT(has_state(id))

    if (intDataPool.size() - 1 != id) { return; }

    registeredStates.erase(id);

    intDataPool.pop_back();

    if (floatDataPool) { floatDataPool->pop_back(); }

}
