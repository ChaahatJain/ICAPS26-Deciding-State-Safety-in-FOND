//
// This file is part of the PlaJA code base.
// Copyright (c) (2019 - 2020) Marcel Vinzent.
// See README.md in the top-level directory for licensing information.
//

#include "synchronisation_information.h"
#include "../../parser/ast/model.h"
#include "../../parser/ast/composition.h"
#include "../../parser/ast/synchronisation.h"

SynchronisationInformation::SynchronisationInformation(const Model& model):
    num_automata_instances(model.get_number_automataInstances()),
    num_syncs(model.get_system()->get_number_syncs()),
    num_actions(model.get_number_actions()) {

    // storage maintenance
    synchronisationResults.reserve(num_syncs);
    synchronisations.resize(num_syncs);
    synchronisationsPerAction.resize(num_actions);
    relevantSyncs.resize(num_automata_instances, std::vector<std::vector<SyncIndex_type>>(num_actions));

    // initialise
    const Composition* system = model.get_system();
    for (SyncIndex_type sync_index = 0; sync_index < num_syncs; ++sync_index) {
        const Synchronisation* sync = system->get_sync(sync_index);
        ActionID_type result_action_id = sync->get_resultID();
        synchronisationResults.push_back(result_action_id); // set results
        if (result_action_id == ACTION::silentAction) {
            synchronisationsPerSilentAction.push_back(sync_index);
        } else {
            synchronisationsPerAction[result_action_id].push_back(sync_index);
        }
        // participating automata:
        PLAJA_ASSERT(sync->get_size_synchronise() == num_automata_instances)
        for (AutomatonIndex_type automaton_index = 0; automaton_index < num_automata_instances; ++automaton_index) {
            ActionID_type action = sync->get_syncActionID(automaton_index);
            if (action == ACTION::nullAction) continue; // not participating
            PLAJA_ASSERT(action >= 0)
            synchronisations[sync_index].emplace_back(automaton_index, sync->get_syncActionID(automaton_index)); // from synchronisation to automaton
            relevantSyncs[automaton_index][sync->get_syncActionID(automaton_index)].push_back(sync_index); // from automaton to synchronisation
        }
    }

}
