//
// This file is part of the PlaJA code base.
// Copyright (c) (2019 - 2019) Marcel Vinzent.
// See README.md in the top-level directory for licensing information.
//

#include "successor_generator.h"
#include "../../parser/ast/automaton.h"
#include "../../parser/ast/edge.h"
#include "../../parser/ast/expression/expression.h"
#include "../../parser/ast/location.h"
#include "../../parser/ast/model.h"
#include "../information/model_information.h"
#include "../states/state_base.h"
#include "base/enabled_sync_computation.h"

SuccessorGenerator::SuccessorGenerator(const PLAJA::Configuration& config, const Model& model):
    SuccessorGeneratorBase(config, model) {
    // initialise exploration structures
    operatorList.resize(get_sync_information().num_syncs + 1);
    labeledOps.resize(get_sync_information().num_actions);
    SUCCESSOR_GENERATION::initialize_enablement_structures(get_sync_information(), enabledEdges, enabledSynchronisations);
}

SuccessorGenerator::~SuccessorGenerator() = default;

/**helper**************************************************************************************************************/

void SuccessorGenerator::reset_exploration_structures() {

    reset_base();

    /* Operators. */
    for (auto& operators: operatorList) { operators.clear(); }
    /* */
    silentOps.clear();
    for (auto& labeled_ops: labeledOps) { labeled_ops.clear(); }

    /* Synchronisation data. */
    for (auto& sync_flag: enabledSynchronisations) { sync_flag = 0; }

    /* Edges. */
    for (auto& enabled_per_automaton: enabledEdges) { for (auto& enabled_per_action: enabled_per_automaton) { enabled_per_action.clear(); } }

}

void SuccessorGenerator::compute_enabled_synchronisations() {
    SUCCESSOR_GENERATION::compute_enabled_synchronisations(get_sync_information(), enabledEdges, enabledSynchronisations);
}

/**
 * Adaption of calculateTransitions method from https://github.com/prismmodelchecker/prism/blob/8394df8e1a0058cec02f47b0437b185e3ae667d7/prism/src/simulator/Updater.java (PRISM, March 20, 2019).
*/
void SuccessorGenerator::process_local_op(SyncIndex_type sync_index, std::vector<ActionOpInit>& sync_ops, std::list<PreOpInit>& enabled_edges_loc, const StateBase& state) {
    PLAJA_ASSERT(!enabled_edges_loc.empty())  // should have skipped sync otherwise

    // initialise operators if necessary:
    if (not enabled_edges_loc.front().is_initialised()) {
        for (auto& pre_op: enabled_edges_loc) { pre_op.initialise(state); }
    }

    // case where there is only 1 edge for this automaton
    if (enabled_edges_loc.size() == 1) {
        const PreOpInit& pre_op = enabled_edges_loc.front();

        // case where this is the first operator created
        if (sync_ops.empty()) {
            sync_ops.emplace_back(pre_op, get_model_information().compute_action_op_id_additive(ModelInformation::sync_index_to_id(sync_index)), sync_index);
        } else { // case where there are existing operators
            // product with existing operators
            for (auto& sync_op: sync_ops) { sync_op.product_with(pre_op); }
        }

    } else { // case where there are multiple edges (i.e. local nondeterminism)

        // case where there are no existing operator
        if (sync_ops.empty()) {
            for (const auto& pre_op: enabled_edges_loc) {
                sync_ops.emplace_back(pre_op, get_model_information().compute_action_op_id_additive(ModelInformation::sync_index_to_id(sync_index)), sync_index); // depending on the model type, operator size check (e.g. for emptiness) becomes necessary
            }
        } else { // case where there are existing operators
            // duplicate (count-1 copies of) current operator list
            std::size_t op_list_index;
            const std::size_t num_duplicates = enabled_edges_loc.size() - 1;
            const std::size_t old_number_of_ops = sync_ops.size();
            for (std::size_t i = 0; i < num_duplicates; ++i) {
                for (op_list_index = 0; op_list_index < old_number_of_ops; ++op_list_index) {
                    sync_ops.emplace_back(sync_ops[op_list_index]);
                }
            }
            // products with existing operators
            std::size_t edge_counter = 0;
            for (const auto& pre_op: enabled_edges_loc) {
                for (op_list_index = 0; op_list_index < old_number_of_ops; ++op_list_index) {
                    sync_ops[edge_counter * old_number_of_ops + op_list_index].product_with(pre_op);
                }
                ++edge_counter;
            }

        }
    }
}

/**********************************************************************************************************************/

void SuccessorGenerator::explore(const StateBase& state) {
    /* Reset exploration structures. */
    reset_exploration_structures();

    if constexpr (PLAJA_GLOBAL::enableTerminalStateSupport) { if (is_terminal(state)) { return; } }

    auto& non_sync_ops = operatorList[SYNCHRONISATION::nonSyncID];

    // explore (create non-sync operators and compute enabled edges for synchronisation operators):
    for (AutomatonIndex_type automaton_index = 0; automaton_index < get_sync_information().num_automata_instances; ++automaton_index) { // for each automaton
        PLAJA_ASSERT(state.get_int(automaton_index) >= 0)
        const std::vector<const Edge*>& localEdges = get_edges_per_auto_per_loc()[automaton_index][state.get_int(automaton_index)]; // note: automaton_index is also the location id
        for (const Edge* edge: localEdges) { // for each edge starting in the current location
            const Expression* guard = edge->get_guardExpression();
            if (not guard/*trivial true*/ or guard->evaluateInteger(&state)) { // guard satisfied
                const auto* action_ptr = edge->get_action();
                if (action_ptr) { // Sync.
                    enabledEdges[automaton_index][action_ptr->get_id()].emplace_back(*edge, automaton_index, get_model()); // Add to enabled Edges of automaton and action.
                } else { // Silent.
                    non_sync_ops.emplace_back(*edge, state, get_model_information().compute_action_op_id_non_sync(automaton_index, edge->get_id()), get_model());
                }
            }
        }
    }

    access_num_of_enabled_ops() = non_sync_ops.size(); // update #enabled operators
    if (!non_sync_ops.empty()) { silentOps.push_back(SYNCHRONISATION::nonSyncID); } // maintain per action information

    // compute whether a synchronisation is enabled or not
    compute_enabled_synchronisations();
    // compute synchronisation operators:
    for (SyncIndex_type sync_index = 0; sync_index < get_sync_information().num_syncs; ++sync_index) {
        if (not enabledSynchronisations[sync_index]) { continue; } // skip
        // else
        const SyncID_type sync_id = ModelInformation::sync_index_to_id(sync_index);
        auto& sync_ops = operatorList[sync_id];
        sync_ops.reserve(enabledSynchronisations[sync_index]); // reserve
        for (const auto& condition: get_sync_information().synchronisations[sync_index]) {
            process_local_op(sync_index, sync_ops, enabledEdges[condition.automatonIndex][condition.actionID], state);
        }
        access_num_of_enabled_ops() += sync_ops.size(); // update #enabled operators

        // maintain per action information
        if (not sync_ops.empty()) {
            const ActionLabel_type action_label = ModelInformation::action_id_to_label(get_model_information().sync_index_to_action(sync_index));
            if (action_label == ACTION::silentLabel) { silentOps.push_back(sync_id); }
            else { labeledOps[action_label].push_back(sync_id); }
        }

    }

    // return operatorList;
}






