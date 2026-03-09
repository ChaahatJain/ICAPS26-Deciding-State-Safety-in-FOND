#ifndef STATE_REGISTRY_H
#define STATE_REGISTRY_H

#include <memory>
#include <set>
#include <unordered_set>
#include "../../utils/fd_imports/segmented_vector.h"
#include "../../utils/fd_imports/hash_extension.h"
#include "../states/forward_states.h"
#include "../using_search.h"
#include "int_packer.h"

namespace STATE_REGISTRY { struct SetState; } // PLAJA technical internals

class StateRegistry {

public:

    struct PackedStateBin {
    private:
        IntPacker::Bin* intBuffer;
        PLAJA::floating* floatBuffer;

    public:
        explicit PackedStateBin(IntPacker::Bin* int_buffer, PLAJA::floating* float_buffer = nullptr):
            intBuffer(int_buffer)
            , floatBuffer(float_buffer) {}

        ~PackedStateBin() = default;

        DEFAULT_CONSTRUCTOR(PackedStateBin)

        [[nodiscard]] inline const IntPacker::Bin* g_int() const { return intBuffer; }

        [[nodiscard]] inline IntPacker::Bin* g_int() { return intBuffer; }

        [[nodiscard]] inline const PLAJA::floating* g_float() const { return floatBuffer; }

        [[nodiscard]] inline PLAJA::floating* g_float() { return floatBuffer; }

        inline void free() {
            delete[] intBuffer;
            delete[] floatBuffer;
        }
    };

private:

    struct StateIdSemanticHash {
        const StateRegistry* registry;

        explicit StateIdSemanticHash(const StateRegistry& registry_r):
            registry(&registry_r) {}

        std::size_t operator()(const StateID_type id) const {
            utils::HashState hash_state;
            // int
            const auto* data_int = registry->intDataPool[id];
            for (int i = 0; i < registry->intPacker.get_num_bins(); ++i) { hash_state.feed(data_int[i]); }
            // floating
            if (registry->floatDataPool) {
                const auto* data_float = (*registry->floatDataPool)[id];
                for (int i = 0; i < registry->sizeFloatingState; ++i) { utils::feed(hash_state, data_float[i]); }
            }
            //
            return hash_state.get_hash32();
        }
    };

    struct StateIdSemanticEqual {
        const StateRegistry* registry;

        explicit StateIdSemanticEqual(const StateRegistry& registry_r):
            registry(&registry_r) {}

        std::size_t operator()(const StateID_type lhs, const StateID_type rhs) const {
            // int
            const auto* lhs_data_int = registry->intDataPool[lhs];
            const auto* rhs_data_int = registry->intDataPool[rhs];
            if (not std::equal(lhs_data_int, lhs_data_int + registry->intPacker.get_num_bins(), rhs_data_int)) { return false; }
            // floating
            if (registry->floatDataPool) {
                const auto* lhs_data_float = (*registry->floatDataPool)[lhs];
                const auto* rhs_data_float = (*registry->floatDataPool)[rhs];
                if (not std::equal(lhs_data_float, lhs_data_float + registry->sizeFloatingState, rhs_data_float)) { return false; }
            }
            return true;
        }
    };

    /*
      Hash set of StateIDs used to detect states that are already registered in
      this registry and find their IDs. States are compared/hashed semantically,
      i.e. the actual state data is compared, not the memory location.
    */
    typedef std::unordered_set<StateID_type, StateIdSemanticHash, StateIdSemanticEqual> StateIdSet;

    const IntPacker intPacker; // NOLINT(*-avoid-const-or-ref-data-members)
    const std::size_t sizeFloatingState; // NOLINT(*-avoid-const-or-ref-data-members)
    PLAJA::floating* floatConstructor;
    //
    mutable segmented_vector::SegmentedArrayVector<IntPacker::Bin> intDataPool; // mutable to check membership without adding
    std::unique_ptr<segmented_vector::SegmentedArrayVector<PLAJA::floating>> floatDataPool; // may be unused
    //
    StateIdSet registeredStates;

    StateID_type insert_id_or_pop_state();

    friend StateIdSemanticHash; //
    friend StateIdSemanticEqual; //
    friend class State; //
    friend class StateRegistryPa;

    friend STATE_REGISTRY::SetState; // technical internals
    // to be called within State:
    PackedStateBin push_state_construct() {
        PLAJA_ASSERT(not floatDataPool or intDataPool.size() == floatDataPool->size())
        intDataPool.push_back(intPacker.get_bin_constructor());
        if (floatDataPool) {
            floatDataPool->push_back(floatConstructor);
            auto candidate_index = intDataPool.size() - 1;
            return PackedStateBin(intDataPool[candidate_index], (*floatDataPool)[candidate_index]);
        } else {
            return PackedStateBin(intDataPool[intDataPool.size() - 1]);
        }
    }

    //
    PackedStateBin push_state_construct(const PackedStateBin& buffer) {
        PLAJA_ASSERT(not floatDataPool or intDataPool.size() == floatDataPool->size())
        intDataPool.push_back(buffer.g_int());
        if (floatDataPool) {
            floatDataPool->push_back(buffer.g_float());
            auto candidate_index = intDataPool.size() - 1;
            return PackedStateBin(intDataPool[candidate_index], (*floatDataPool)[candidate_index]);
        } else {
            return PackedStateBin(intDataPool[intDataPool.size() - 1]);
        }
    }

    // lookup whether state(-prefix) (from other registry) is contained in this registry, e.g., abstract state extending abstract states stored in this registry
    StateID_type contains(const PackedStateBin& buffer) const;

    inline PackedStateBin get_state_buffer(StateID_type id) {
        PLAJA_ASSERT(has_state(id))
        return PackedStateBin(intDataPool[id], floatDataPool ? (*floatDataPool)[id] : nullptr);
    }

public:
    explicit StateRegistry(const std::vector<Range_type>& ranges, const std::vector<PLAJA::integer>& lower_bounds, const std::vector<PLAJA::integer>& upper_bounds, std::size_t num_floats = 0);
    ~StateRegistry();
    DELETE_CONSTRUCTOR(StateRegistry)

    inline const IntPacker& get_int_packer() const { return intPacker; }

    FCT_IF_DEBUG(bool has_state(const StateID_type id) const {
        PLAJA_ASSERT(not floatDataPool or intDataPool.size() == floatDataPool->size())
        PLAJA_ASSERT(id != STATE::none or id >= intDataPool.size())
        return id < intDataPool.size();
    })

    std::unique_ptr<State> get_state(StateID_type id);

    /*
      Returns the state that was registered at the given ID. The ID must refer
      to a state in this registry. Do not mix IDs from different registries.
    */
    State lookup_state(StateID_type id); // plaja-prob: Breaks the assumption that StateID instance is only constructed in insert_id_or_pop_state

    std::unique_ptr<State> set_state(const StateValues& values);

    State add_state(const StateValues& values); // set_state with no pointer return

    StateID_type add_state_id(const StateValues& values);

    void unregister_state_if_possible(StateID_type id);

    inline std::size_t get_int_state_size() const { return intPacker.get_state_size(); }

    inline std::size_t get_floating_state_size() const { return sizeFloatingState; }

    inline std::size_t get_state_size() const { return get_int_state_size() + get_floating_state_size(); }

    inline std::size_t get_num_bytes_per_state() const { return intPacker.get_num_bins() * IntPacker::get_bin_size_in_bytes(); }

    /** Returns the number of states registered so far. */
    inline std::size_t size() const { return registeredStates.size(); }

};

#endif
