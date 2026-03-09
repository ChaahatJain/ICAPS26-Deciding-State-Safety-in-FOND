//
// This file is part of the PlaJA code base.
// Copyright (c) (2019 - 2022) Marcel Vinzent.
// See README.md in the top-level directory for licensing information.
//

#ifndef PLAJA_ABSTRACT_STATE_H
#define PLAJA_ABSTRACT_STATE_H

#include "../../../utils/utils.h"
#include "pa_state_base.h"
#include "state_registry_pa.h"

class AbstractState final: public PaStateBase {
    friend class StateRegistryPa;

private:
    StateRegistry::PackedStateBin buffer;
    StateRegistryPa* paRegistry;
    const IntPacker* paPacker;
    StateID_type id;

    const PredicateState* parent; // predicate state this registered state was constructed from, may be NULL

    AbstractState(StateRegistry::PackedStateBin buffer_v, StateRegistryPa& pa_registry, const IntPacker& pa_packer, StateID_type state_id, const PredicateState* parent);

    inline static std::unique_ptr<AbstractState> make_ptr(StateRegistry::PackedStateBin buffer_v, StateRegistryPa& pa_registry, const IntPacker& pa_packer, StateID_type state_id, const PredicateState* parent) { return std::unique_ptr<AbstractState>(new AbstractState(buffer_v, pa_registry, pa_packer, state_id, parent)); }

public:
    ~AbstractState() override;
    DELETE_CONSTRUCTOR(AbstractState)

    [[nodiscard]] inline StateID_type get_id_value() const { return id; }

    [[nodiscard]] inline std::unique_ptr<AbstractState> to_ptr() const { return make_ptr(buffer, *paRegistry, *paPacker, id, parent); }

    /******************************************************************************************************************/

    [[nodiscard]] PA::TruthValueType predicate_value(PredicateIndex_type pred_index) const final {
        PLAJA_ASSERT(pred_index < get_size_predicates())
        return paPacker->get(buffer.g_int(), pred_index + get_size_locs());
    }

    [[nodiscard]] bool is_set(PredicateIndex_type pred_index) const override { return not PA::is_unknown(predicate_value(pred_index)); }

    [[nodiscard]] PLAJA::integer location_value(VariableIndex_type loc_index) const final {
        PLAJA_ASSERT(loc_index < get_size_locs())
        return paPacker->get(buffer.g_int(), loc_index);
    }

    /******************************************************************************************************************/

    [[nodiscard]] bool has_entailment_information() const override;

    [[nodiscard]] bool is_entailed(PredicateIndex_type pred_index) const override;

    [[nodiscard]] PA::EntailmentValueType entailment_value(PredicateIndex_type pred_index) const override;

    FCT_IF_DEBUG(inline void drop_entailment_information() { parent = nullptr; })

    /******************************************************************************************************************/

    class LocationStateIterator final: public PaStateBase::LocationStateIterator {
        friend AbstractState;

    private:

        explicit LocationStateIterator(const AbstractState& abstract_state):
            PaStateBase::LocationStateIterator(abstract_state) {}

    public:
        ~LocationStateIterator() final = default;
        DELETE_CONSTRUCTOR(LocationStateIterator)

        [[nodiscard]] inline const AbstractState& operator()() const { return PLAJA_UTILS::cast_ref<AbstractState>(*paState); }
    };

    [[nodiscard]] inline LocationStateIterator init_loc_state_iterator() const { return LocationStateIterator(*this); }

    /******************************************************************************************************************/

    class PredicateStateIterator final: public PaStateBase::PredicateStateIterator {
        friend AbstractState;

    private:

        explicit PredicateStateIterator(const AbstractState& abstract_state, PredicateIndex_type start_index):
            PaStateBase::PredicateStateIterator(abstract_state, start_index) {}

    public:
        ~PredicateStateIterator() final = default;
        DELETE_CONSTRUCTOR(PredicateStateIterator)

        [[nodiscard]] inline const AbstractState& operator()() const { return PLAJA_UTILS::cast_ref<AbstractState>(*paState); }
    };

    [[nodiscard]] inline PredicateStateIterator init_pred_state_iterator() const { return PredicateStateIterator(*this, 0); }

};

#endif //PLAJA_ABSTRACT_STATE_H
