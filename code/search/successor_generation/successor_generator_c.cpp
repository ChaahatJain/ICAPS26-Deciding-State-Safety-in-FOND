//
// This file is part of the PlaJA code base.
// Copyright (c) (2019 - 2020) Marcel Vinzent.
// See README.md in the top-level directory for licensing information.
//

#include "successor_generator_c.h"
#include "../../parser/ast/automaton.h"
#include "../../parser/ast/edge.h"
#include "../../parser/ast/expression/expression.h"
#include "../../parser/ast/model.h"
#include "../fd_adaptions/state.h"
#include "../information/model_information.h"
#include "base/action_op_construction.h"
#include "base/enabled_sync_computation.h"
#include "action_op.h"

using ActionOpConstruction = SUCCESSOR_GENERATION::ActionOpConstruction<Edge, ActionOp>;

SuccessorGeneratorC::SuccessorGeneratorC(const PLAJA::Configuration& config, const Model& model_ref):
    SuccessorGeneratorBase(config, model_ref)
    , cached_exploration_state(nullptr) {
    // initialise operator/exploration structures
    syncOps.resize(get_sync_information().num_syncs);
    SUCCESSOR_GENERATION::initialize_enablement_structures(get_sync_information(), enabledEdges, enabledSynchronisations);
    initialize_action_ops();
    // allocate exploration structures
    labeledOps.resize(get_sync_information().num_actions);
}

SuccessorGeneratorC::~SuccessorGeneratorC() = default;

/** helper ************************************************************************************************************/

void SuccessorGeneratorC::compute_enabled_synchronisations() {
    SUCCESSOR_GENERATION::compute_enabled_synchronisations(get_sync_information(), enabledEdges, enabledSynchronisations);
}

void SuccessorGeneratorC::compute_sync_ops(ActionOpStructure& action_op_structure, SyncIndex_type sync_index, std::size_t condition_index) { // NOLINT(misc-no-recursion)
    const auto& condition = get_sync_information().synchronisations[sync_index];
    if (condition_index == condition.size()) {
        const ActionOpIndex_type action_op_index = get_model_information().compute_action_op_index(action_op_structure);
        auto& sync_ops = syncOps[sync_index];
        SUCCESSOR_GENERATION::resize_action_op_vector(sync_ops, action_op_index + 1);
        sync_ops[action_op_index] = ActionOp::construct_operator(action_op_structure, get_model());
    } else {
        const auto& condition_loc = condition[condition_index];
        const auto& edges_loc = enabledEdges[condition_loc.automatonIndex][condition_loc.actionID];
        for (const EdgeID_type edge_id: edges_loc) {
            action_op_structure.participatingEdges.emplace_back(condition_loc.automatonIndex, edge_id);
            compute_sync_ops(action_op_structure, sync_index, condition_index + 1);
            action_op_structure.participatingEdges.pop_back();
        }
    }
}

void SuccessorGeneratorC::reset_exploration_structures() {
    reset_base();
    /* Operators. */
    for (auto& sync_flag: enabledSynchronisations) { sync_flag = 0; } // synchronisation data
    silentOps.clear();
    for (auto& ops_per_label: labeledOps) { ops_per_label.clear(); }
    /* Edges. */
    for (auto& enabled_per_automaton: enabledEdges) { for (auto& enabled_per_action: enabled_per_automaton) { enabled_per_action.clear(); } }
}

void SuccessorGeneratorC::compute_enabled_sync_ops(SyncIndex_type sync_index, const std::vector<AutomatonSyncAction>& condition, ActionOpIndex_type action_op_index, std::size_t condition_index) { // NOLINT(misc-no-recursion)
    if (condition_index == condition.size()) {
        auto* enabled_op = syncOps[sync_index][action_op_index].get();
        PLAJA_ASSERT(enabled_op)
        enabled_op->initialise(cached_exploration_state);
        const ActionLabel_type result_action = enabled_op->_action_label();
        PLAJA_ASSERT(result_action == get_sync_information().synchronisationResults[sync_index])
        if (result_action == ACTION::silentAction) { silentOps.push_back(enabled_op); }
        else { labeledOps[result_action].push_back(enabled_op); }
    } else {
        const auto& condition_loc = condition[condition_index];
        const auto& enabled_edges_loc = enabledEdges[condition_loc.automatonIndex][condition_loc.actionID];
        for (const EdgeID_type edge_id: enabled_edges_loc) {
            compute_enabled_sync_ops(sync_index, condition, action_op_index + get_model_information().compute_action_op_index_sync_additive(sync_index, condition[condition_index].automatonIndex, edge_id), condition_index + 1);
        }
    }
}

/**********************************************************************************************************************/

void SuccessorGeneratorC::initialize_action_ops() {
    // clear structures in case of re-usage
    reset_exploration_structures();
    // operators:
    nonSyncOps.clear();
    for (auto& ops_per_sync: syncOps) { ops_per_sync.clear(); }

    // construction structure
    ActionOpConstruction action_op_construction(get_model());

    // iterate over edges and assign to proper structure
    std::size_t num_edges, j;
    for (AutomatonIndex_type automaton_index = 0; automaton_index < get_sync_information().num_automata_instances; ++automaton_index) { // for each automaton
        const Automaton* automaton = get_model().get_automatonInstance(automaton_index);
        num_edges = automaton->get_number_edges();
        for (j = 0; j < num_edges; ++j) { // for each ...
            const Edge* edge = automaton->get_edge(j);
            const auto* action_ptr = edge->get_action();
            if (action_ptr) { // Sync.
                const auto action_id = action_ptr->get_id();
                action_op_construction.add_enabled_edge(automaton_index, action_id, edge);
                enabledEdges[automaton_index][action_id].push_back(edge->get_id()); // have to push value also to exploration time structure to make *compute_enabled_synchronisations* work properly
            } else { // Immediately construct operator.
                const ActionOpIndex_type action_op_index = get_model_information().compute_action_op_index_non_sync(automaton_index, edge->get_id());
                SUCCESSOR_GENERATION::resize_action_op_vector(nonSyncOps, action_op_index + 1);
                nonSyncOps[action_op_index].reset(new ActionOp(edge, get_model_information().compute_action_op_id_non_sync(automaton_index, edge->get_id()), get_model()));
            }
        }
    }

    compute_enabled_synchronisations(); // with proper modelling this should be redundant, i.e., for any sync it should at least syntactically be possible to enable it

    for (SyncIndex_type sync_index = 0; sync_index < get_sync_information().num_syncs; ++sync_index) { // for each sync ...
        if (not enabledSynchronisations[sync_index]) { continue; }
        // construct
        /* alternative *************************************************************/
        // syncOps[sync_index].reserve(enabledSynchronisations[sync_index]);
        // ActionOpStructure action_op_structure;
        // action_op_structure.syncID = ModelInformation::sync_index_to_id(sync_index);
        // compute_sync_ops(action_op_structure, sync_index, 0);
        /*++++++++++++*************************************************************/
        action_op_construction.reserve(enabledSynchronisations[sync_index]);
        for (const auto& condition: get_sync_information().synchronisations[sync_index]) { // construct op stepwise including edges from each participating automaton
            action_op_construction.process_local_sync_edges(sync_index, condition);
        }
        // finalize and add to global list
        action_op_construction.move_to(syncOps[sync_index]);
        action_op_construction.clear();
    }

}

/**********************************************************************************************************************/

void SuccessorGeneratorC::explore(const StateBase& state) {
    /* Reset exploration structures. */
    reset_exploration_structures();

    if constexpr (PLAJA_GLOBAL::enableTerminalStateSupport) { if (is_terminal(state)) { return; } }

    cached_exploration_state = &state;

    // explore (create non-sync operators and compute enabled edges for synchronisation operators):
    for (AutomatonIndex_type automaton_index = 0; automaton_index < get_sync_information().num_automata_instances; ++automaton_index) { // for each automaton
        PLAJA_ASSERT((*cached_exploration_state).get_int(automaton_index) >= 0)
        const std::vector<const Edge*>& local_edges = get_edges_per_auto_per_loc()[automaton_index][(*cached_exploration_state).get_int(automaton_index)]; // note: automaton_index is also the location id
        for (const Edge* edge: local_edges) { // for each edge starting in the current location
            const Expression* guard = edge->get_guardExpression();
            if (not guard/*trivial true*/ or guard->evaluateInteger(cached_exploration_state)) { // guard satisfied
                const auto* action_ptr = edge->get_action();
                if (action_ptr) { // Synchronisation.
                    enabledEdges[automaton_index][action_ptr->get_id()].push_back(edge->get_id()); // Add to enable Edges of automaton and action.
                } else {
                    ActionOp* enabled_op = nonSyncOps[get_model_information().compute_action_op_index_non_sync(automaton_index, edge->get_id())].get();
                    enabled_op->initialise(cached_exploration_state);
                    silentOps.push_back(enabled_op);
                    PLAJA_ASSERT(enabled_op)
                }
            }
        }
    }

    // compute whether a synchronisation is enabled or not
    compute_enabled_synchronisations();
    // compute synchronisation operators:
    for (SyncIndex_type sync_index = 0; sync_index < get_sync_information().num_syncs; ++sync_index) {
        if (not enabledSynchronisations[sync_index]) { continue; }
        compute_enabled_sync_ops(sync_index, get_sync_information().synchronisations[sync_index], 0, 0);
    }

    // set #enabled operators
    access_num_of_enabled_ops() = silentOps.size();
    for (const auto& ops_per_action: labeledOps) { access_num_of_enabled_ops() += ops_per_action.size(); }

    cached_exploration_state = nullptr;
}

bool SuccessorGeneratorC::is_applicable(ActionLabel_type action_label) const {
    if (action_label == ACTION::silentAction) {
        return !silentOps.empty();
    } else {
        PLAJA_ASSERT(0 <= action_label && action_label <= labeledOps.size())
        return !labeledOps[action_label].empty();
    }
}

/**********************************************************************************************************************/

inline const ActionOp& SuccessorGeneratorC::get_action_op(SyncID_type sync_id, ActionOpIndex_type action_op_index) const {
    if (sync_id == SYNCHRONISATION::nonSyncID) {
        PLAJA_ASSERT(action_op_index < nonSyncOps.size())
        return *nonSyncOps[action_op_index];
    } else {
        const SyncIndex_type sync_index = ModelInformation::sync_id_to_index(sync_id);
        PLAJA_ASSERT(sync_index < syncOps.size())
        PLAJA_ASSERT(action_op_index < syncOps[sync_index].size())
        return *syncOps[sync_index][action_op_index];
    }
}

const ActionOp& SuccessorGeneratorC::get_action_op(const ActionOpStructure& action_op_structure) const {
    return get_action_op(action_op_structure.syncID, get_model_information().compute_action_op_index(action_op_structure));
}

const ActionOp& SuccessorGeneratorC::get_action_op(ActionOpID_type action_op_id) const {
    const ActionOpIDStructure action_op_id_structure = get_model_information().compute_action_op_id_structure(action_op_id);
    return get_action_op(action_op_id_structure.syncID, action_op_id_structure.actionOpIndex);
}

std::list<const ActionOp*> SuccessorGeneratorC::extract_ops_per_label(ActionLabel_type action_label) const {
    PLAJA_ASSERT(action_label <= PLAJA_UTILS::cast_numeric<ActionLabel_type>(get_sync_information().num_actions))

    std::list<const ActionOp*> ops_per_label;

    if (action_label == ACTION::silentAction) { // SILENT
        // non sync
        for (auto& non_sync_op: nonSyncOps) {
            ops_per_label.push_back(non_sync_op.get());
        }
        // silent synchs
        for (const SyncIndex_type sync_index: get_sync_information().synchronisationsPerSilentAction) {
            for (auto& sync_op: syncOps[sync_index]) {
                ops_per_label.push_back(sync_op.get());
            }
        }
    } else { // NON-SILENT
        for (const SyncIndex_type sync_index: get_sync_information().synchronisationsPerAction[action_label]) {
            for (auto& sync_op: syncOps[sync_index]) {
                ops_per_label.push_back(sync_op.get());
            }
        }
    }

    return ops_per_label;
}

ActionLabel_type SuccessorGeneratorC::get_action_label(const ActionOpStructure& action_op_structure) const { return get_action_op(action_op_structure)._action_label(); }

ActionLabel_type SuccessorGeneratorC::get_action_label(ActionOpID_type action_op_id) const { return get_action_op(action_op_id)._action_label(); }

//

inline const ActionOp& SuccessorGeneratorC::explore_action_op(const StateBase& state, SyncID_type sync_id, ActionOpIndex_type action_op_index) const {
    if (sync_id == SYNCHRONISATION::nonSyncID) {
        ActionOp& non_sync_op = *nonSyncOps[action_op_index];
        non_sync_op.initialise(&state);
        return non_sync_op;
    } else {
        const SyncIndex_type sync_index = ModelInformation::sync_id_to_index(sync_id);
        ActionOp& sync_op = *syncOps[sync_index][action_op_index];
        sync_op.initialise(&state);
        return sync_op;
    }
}

const ActionOp& SuccessorGeneratorC::explore_action_op(const StateBase& state, const ActionOpStructure& action_op_structure) {
    return explore_action_op(state, action_op_structure.syncID, get_model_information().compute_action_op_index(action_op_structure));
}

const ActionOp& SuccessorGeneratorC::explore_action_op(const StateBase& state, ActionOpID_type action_op_id) {
    const ActionOpIDStructure action_op_id_structure = get_model_information().compute_action_op_id_structure(action_op_id);
    return explore_action_op(state, action_op_id_structure.syncID, action_op_id_structure.actionOpIndex);
}

/* */

bool SuccessorGeneratorC::is_applicable(const StateBase& state, const ActionOp& action_op) const {
    PLAJA_ASSERT_EXPECT(not is_terminal(state))
    return action_op.is_enabled(state);
}

std::unique_ptr<State> SuccessorGeneratorC::compute_successor(const State& source, const ActionOp& action_op, UpdateIndex_type update_index) const { return compute_successor(action_op.get_update(update_index), source); }

std::unique_ptr<State> SuccessorGeneratorC::compute_successor(const State& source, ActionOpID_type action_op_id, UpdateIndex_type update_index) const { return compute_successor(source, get_action_op(action_op_id), update_index); }

std::unique_ptr<State> SuccessorGeneratorC::compute_successor(const State& source, UpdateOpID_type update_op_id) const {
    const UpdateOpStructure update_op_structure = get_model_information().compute_update_op_structure(update_op_id);
    return compute_successor(source, update_op_structure.actionOpID, update_op_structure.updateIndex);
}

/* */

bool SuccessorGeneratorC::is_applicable(const StateBase& state, ActionLabel_type action_label) const {
    PLAJA_ASSERT_EXPECT(not is_terminal(state))

    if (action_label == ACTION::silentAction) { // SILENT
        // non sync
        for (const auto& non_sync_op: nonSyncOps) {
            if (non_sync_op->is_enabled(state)) { return true; }
        }
        // silent synchs
        for (const SyncIndex_type sync_index: get_sync_information().synchronisationsPerSilentAction) {
            for (const auto& silent_sync_op: syncOps[sync_index]) {
                if (silent_sync_op->is_enabled(state)) { return true; }
            }
        }
    } else { // NON-SILENT
        PLAJA_ASSERT(0 <= action_label && action_label <= labeledOps.size())
        for (const SyncIndex_type sync_index: get_sync_information().synchronisationsPerAction[action_label]) {
            for (const auto& sync_op: syncOps[sync_index]) {
                if (sync_op->is_enabled(state)) { return true; }
            }
        }
    }

    return false;
}

std::list<const ActionOp*> SuccessorGeneratorC::get_action(const StateBase& state, ActionLabel_type action_label) const {

    std::list<const ActionOp*> applicable_ops;

    if (action_label == ACTION::silentAction) { // SILENT
        // non sync
        for (auto& non_sync_op: nonSyncOps) {
            if (non_sync_op->is_enabled(state)) {
                applicable_ops.push_back(non_sync_op.get());
            }
        }
        // silent synchs
        for (const SyncIndex_type sync_index: get_sync_information().synchronisationsPerSilentAction) {
            for (auto& silent_sync_op: syncOps[sync_index]) {
                if (silent_sync_op->is_enabled(state)) {
                    applicable_ops.push_back(silent_sync_op.get());
                }
            }
        }
    } else { // NON-SILENT
        PLAJA_ASSERT(0 <= action_label && action_label <= labeledOps.size())
        for (const SyncIndex_type sync_index: get_sync_information().synchronisationsPerAction[action_label]) {
            for (const auto& sync_op: syncOps[sync_index]) {
                if (sync_op->is_enabled(state)) {
                    applicable_ops.push_back(sync_op.get());
                }
            }
        }
    }

    return applicable_ops;
}

std::list<const ActionOp*> SuccessorGeneratorC::explore_action(const StateBase& state, ActionLabel_type action_label) {

    std::list<const ActionOp*> applicable_ops;

    if constexpr (PLAJA_GLOBAL::enableTerminalStateSupport) { if (is_terminal(state)) { return applicable_ops; } }

    if (action_label == ACTION::silentAction) { // SILENT
        // non sync
        for (auto& non_sync_op: nonSyncOps) {
            if (non_sync_op->is_enabled(state)) {
                non_sync_op->initialise(&state);
                applicable_ops.push_back(non_sync_op.get());
            }
        }
        // silent synchs
        for (const SyncIndex_type sync_index: get_sync_information().synchronisationsPerSilentAction) {
            for (auto& silent_sync_op: syncOps[sync_index]) {
                if (silent_sync_op->is_enabled(state)) {
                    silent_sync_op->initialise(&state);
                    applicable_ops.push_back(silent_sync_op.get());
                }
            }
        }
    } else { // NON-SILENT
        PLAJA_ASSERT(0 <= action_label && action_label <= labeledOps.size())
        for (const SyncIndex_type sync_index: get_sync_information().synchronisationsPerAction[action_label]) {
            for (const auto& sync_op: syncOps[sync_index]) {
                if (sync_op->is_enabled(state)) {
                    sync_op->initialise(&state);
                    applicable_ops.push_back(sync_op.get());
                }
            }
        }
    }

    return applicable_ops;
}

/**********************************************************************************************************************/

namespace SUCCESSOR_GENERATOR_C {
    [[nodiscard]] std::list<const ActionOp*> extract_ops_per_label(const SuccessorGeneratorC& successor_generator, ActionLabel_type action_label) { return successor_generator.extract_ops_per_label(action_label); }
}
