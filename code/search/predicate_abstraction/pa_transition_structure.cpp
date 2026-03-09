//
// This file is part of the PlaJA code base.
// Copyright (c) (2019 - 2022) Marcel Vinzent.
// See README.md in the top-level directory for licensing information.
//

#include "pa_transition_structure.h"
#include "pa_states/abstract_state.h"
#include "successor_generation/action_op/action_op_pa.h"

PATransitionStructure::PATransitionStructure():
    source(nullptr)
    , target(nullptr)
    , actionLabel(ACTION::noneLabel)
    , actionOpId(ACTION::noneOp)
    , updateIndex(ACTION::noneUpdate)
    , updateOpId(ACTION::noneUpdateOp)
    , actionOp(nullptr)
    , update(nullptr) {

}

PATransitionStructure::PATransitionStructure(const PATransitionStructure& other):
    source(other.source ? other.source->to_ptr() : nullptr)
    , target(other.target ? other.target->to_ptr() : nullptr)
    , actionLabel(other.actionLabel)
    , actionOpId(other.actionOpId)
    , updateIndex(other.updateIndex)
    , updateOpId(other.updateOpId)
    , actionOp(other.actionOp)
    , update(other.update) {

}

PATransitionStructure::~PATransitionStructure() = default;

PATransitionStructure& PATransitionStructure::operator=(const PATransitionStructure& other) {
    if (&other == this) { return *this; }
    source = other.source ? other.source->to_ptr() : nullptr;
    target = other.target ? other.target->to_ptr() : nullptr;
    actionLabel = other.actionLabel;
    actionOpId = other.actionOpId;
    updateIndex = other.updateIndex;
    actionOp = other.actionOp;
    update = other.update;
    return *this;
}

/* Setter. */

void PATransitionStructure::set_source(std::unique_ptr<AbstractState>&& pa_src) { source = std::move(pa_src); }

void PATransitionStructure::unset_source() { source = nullptr; }

void PATransitionStructure::set_target(std::unique_ptr<AbstractState>&& pa_target) { target = std::move(pa_target); }

void PATransitionStructure::unset_target() { target = nullptr; }

/* */

void PATransitionStructure::set_action_op(const ActionOpPA& action_op) {
    PLAJA_ASSERT(not has_action_label() or action_op._action_label() == actionLabel)
    PLAJA_ASSERT(not has_action_op() or action_op._op_id() == actionOpId)
    actionLabel = action_op._action_label();
    actionOpId = action_op._op_id();
    actionOp = &action_op;
}

void PATransitionStructure::set_update(UpdateIndex_type update_index, const UpdatePA& update_r) {
    PLAJA_ASSERT(has_action_op() and _action_op())
    PLAJA_ASSERT(not has_update())
    set_update(update_index);
    PLAJA_ASSERT(&_action_op()->get_update_pa(updateIndex) == &update_r)
    update = &update_r;
}

/* Getter. */

StateID_type PATransitionStructure::_source_id() const { return source ? source->get_id_value() : STATE::none; }

StateID_type PATransitionStructure::_target_id() const { return target ? target->get_id_value() : STATE::none; }