//
// This file is part of the PlaJA code base.
// Copyright (c) (2019 - 2022) Marcel Vinzent.
// See README.md in the top-level directory for licensing information.
//

#include "abstract_path.h"
#include "../../successor_generation/action_op.h"
#include "../search_space/search_space_pa_base.h"
#include "abstract_state.h"

namespace PLAJA_GLOBAL { extern const Model* currentModel; }

AbstractPath::AbstractPath(const PredicatesExpression& predicates, const SearchSpacePABase& ss_abstract):
    predicates(&predicates)
    , searchSpacePa(&ss_abstract) {}

AbstractPath::~AbstractPath() = default;

/**********************************************************************************************************************/

void AbstractPath::set_op_path(const std::list<UpdateOpStructure>& op_path) {
    opPath = std::vector<UpdateOpStructure>(op_path.cbegin(), op_path.cend());
}

void AbstractPath::set_witness_path(const std::list<StateID_type>& witness_path) {
    witnessPath = std::vector<StateID_type>(witness_path.cbegin(), witness_path.cend());
}

void AbstractPath::set_pa_path(const std::list<StateID_type>& pa_state_path) {
    paStatePath.reserve(pa_state_path.size());
    for (auto pa_state_id: pa_state_path) { append_abstract_state(pa_state_id); }
}

void AbstractPath::append(AbstractPath&& path) {
    PLAJA_ASSERT(predicates == path.predicates)
    PLAJA_ASSERT(searchSpacePa == path.searchSpacePa)
    PLAJA_ASSERT(get_pa_target_id() == path.get_pa_source_id())

    opPath.reserve(opPath.size() + path.opPath.size());
    witnessPath.reserve(witnessPath.size() + path.witnessPath.size());
    paStatePath.reserve(paStatePath.size() + path.paStatePath.size());

    for (const auto& op: path.opPath) { append_op(op.actionOpID, op.updateIndex); }

    if (path.has_witnesses()) { for (const auto witness: path.witnessPath) { append_witness(witness); } }

    if (path.has_pa_state_path()) {
        auto it = path.paStatePath.begin();
        const auto end = path.paStatePath.cend();
        for (++it /*skip first state*/; it != end; ++it) { paStatePath.push_back(std::move(*it)); }
    }

    PLAJA_ASSERT(not has_witnesses() or witnessPath.size() == opPath.size())
    PLAJA_ASSERT(not has_pa_state_path() or paStatePath.size() == opPath.size() + 1)
}

void AbstractPath::append_abstract_state(StateID_type id) {
    PLAJA_ASSERT(searchSpacePa)
    PLAJA_ASSERT(id != STATE::none)
    paStatePath.emplace_back(searchSpacePa->get_abstract_state(id));
}

/**********************************************************************************************************************/

const ActionOp& AbstractPath::retrieve_op(ActionOpID_type op_id) const {
    PLAJA_ASSERT(searchSpacePa)
    return searchSpacePa->retrieve_action_op(op_id);
}

ActionLabel_type AbstractPath::retrieve_label(ActionOpID_type op_id) const { return retrieve_op(op_id)._action_label(); }

bool AbstractPath::is_non_deterministic() const {
    return std::any_of(opPath.cbegin(), opPath.cend(), [this](const auto& op_update) { return retrieve_op(op_update.actionOpID).get_update(op_update.updateIndex).is_non_deterministic(); });
}

StateID_type AbstractPath::get_witness_id(StepIndex_type step) const {
    PLAJA_ASSERT(has_witnesses())
    PLAJA_ASSERT(step < witnessPath.size())
    return witnessPath[step];
}

std::unique_ptr<State> AbstractPath::get_witness(StepIndex_type step) const {
    const auto witness_id = get_witness_id(step);
    return witness_id == STATE::none ? nullptr : searchSpacePa->get_witness(get_witness_id(step));
}
#ifdef USE_VERITAS
std::vector<veritas::Interval> AbstractPath::get_box_witness(StepIndex_type step) const {
    const auto witness_id = get_witness_id(step);
    return witness_id == STATE::none ? std::vector<veritas::Interval>{} : searchSpacePa->get_box_witness(witness_id);
}
std::vector<veritas::Interval> AbstractPath::PathIterator::get_box_witness() const { return path->get_box_witness(step); }
#endif

std::unique_ptr<State> AbstractPath::last_witness() const { return get_witness(last_witness_id()); }

const AbstractState& AbstractPath::get_pa_state(StepIndex_type step) const {
    PLAJA_ASSERT(has_pa_state_path())
    PLAJA_ASSERT(step < paStatePath.size())
    PLAJA_ASSERT(paStatePath[step])
    return *paStatePath[step];
}

StateID_type AbstractPath::get_pa_state_id(StepIndex_type step) const { return get_pa_state(step).get_id_value(); }

/* Output. */

void AbstractPath::dump() const {
    for (auto path_it = init_path_iterator(); !path_it.end(); ++path_it) {
        path_it.get_pa_state().dump();
        path_it.retrieve_op().dump(PLAJA_GLOBAL::currentModel, true);
        PLAJA_LOG(ActionOp::update_to_str(path_it.get_update_index()));
    }
    get_pa_target_state().dump();
}

/**********************************************************************************************************************/

AbstractPath::PathIterator::PathIterator(const AbstractPath& path):
    path(&path)
    , step(0) {}

std::unique_ptr<State> AbstractPath::PathIterator::get_witness() const { return path->get_witness(step); }


AbstractPath::PathIteratorForward::PathIteratorForward(const AbstractPath& path, StepIndex_type target_step):
    PathIterator(path)
    , targetStep(target_step) {
    PLAJA_ASSERT(targetStep <= path.get_target_step())
}

AbstractPath::PathIteratorBackwards::PathIteratorBackwards(const AbstractPath& path, StepIndex_type target_step):
    PathIterator(path)
    , step(PLAJA_UTILS::cast_numeric<int>(target_step) - 1) {

    PLAJA_ASSERT(path.get_size() > 0)

    PLAJA_ASSERT(step < PLAJA_UTILS::cast_numeric<int>(path.get_target_step()))

}