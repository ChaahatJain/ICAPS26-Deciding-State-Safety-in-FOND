//
// This file is part of the PlaJA code base.
// Copyright (c) (2019 - 2022) Marcel Vinzent.
// See README.md in the top-level directory for licensing information.
//

#include "abstract_state.h"
#include "predicate_state.h"

AbstractState::AbstractState(StateRegistry::PackedStateBin buffer_v, StateRegistryPa& pa_registry, const IntPacker& pa_packer, StateID_type state_id, const PredicateState* parent):
    PaStateBase(&pa_registry.get_predicates(), pa_packer.get_state_size() - pa_registry.num_locations(), pa_registry.num_locations())
    , buffer(buffer_v)
    , paRegistry(&pa_registry)
    , paPacker(&pa_packer)
    , id(state_id)
    , parent(parent) {

#ifndef NDEBUG

    if (parent) {

        PLAJA_ASSERT(parent->get_size_predicates() == get_size_predicates())

        PLAJA_ASSERT(parent->has_location_state())

        // locations:
        if (parent->has_location_state()) {

            const auto& loc_state = parent->get_location_state();

            PLAJA_ASSERT(loc_state.size() == get_size_locs())

            for (auto it_loc = init_loc_state_iterator(); !it_loc.end(); ++it_loc) {

                PLAJA_ASSERT(loc_state.get_int(it_loc.location_index()) == it_loc.location_value())

            }

        }

        // predicates:
        for (auto it_pred = parent->init_pred_state_iterator(); !it_pred.end(); ++it_pred) {

            PLAJA_ASSERT(it_pred.is_set() == is_set(it_pred.predicate_index()))

            PLAJA_ASSERT(not it_pred.is_set() or it_pred.predicate_value() == predicate_value(it_pred.predicate_index()))

        }

    }

#endif

}

AbstractState::~AbstractState() = default;

/**********************************************************************************************************************/

bool AbstractState::has_entailment_information() const { return parent; }

bool AbstractState::is_entailed(PredicateIndex_type pred_index) const {
    PLAJA_ASSERT(parent)
    return parent->is_entailed(pred_index);
}

PA::EntailmentValueType AbstractState::entailment_value(PredicateIndex_type pred_index) const {
    PLAJA_ASSERT(parent)
    return parent->entailment_value(pred_index);
}
