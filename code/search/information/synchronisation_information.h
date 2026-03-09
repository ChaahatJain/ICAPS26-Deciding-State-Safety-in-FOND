//
// This file is part of the PlaJA code base.
// Copyright (c) (2019 - 2020) Marcel Vinzent.
// See README.md in the top-level directory for licensing information.
//

#ifndef PLAJA_SYNCHRONISATION_INFORMATION_H
#define PLAJA_SYNCHRONISATION_INFORMATION_H

#include <vector>
#include "../../utils/structs_utils/automaton_sync_action.h"

// forward declaration
class Model;

struct SynchronisationInformation {
    // model's information cached:
    std::vector<ActionID_type> synchronisationResults;
    std::vector<std::vector<AutomatonSyncAction>> synchronisations; // automaton-action tuples
    // inferred:
    std::vector<SyncIndex_type> synchronisationsPerSilentAction; // synchs with a silent result action
    std::vector<std::vector<SyncIndex_type>> synchronisationsPerAction; // syncs per result action
    std::vector<std::vector<std::vector<SyncIndex_type>>> relevantSyncs; // syncs an edge of a specific action can participate in, sorted by automaton and action

    const std::size_t num_automata_instances; // cached
    const std::size_t num_syncs; // cached
    const std::size_t num_actions; // cached

    explicit SynchronisationInformation(const Model& model);
    ~SynchronisationInformation() = default;
};


#endif //PLAJA_SYNCHRONISATION_INFORMATION_H
