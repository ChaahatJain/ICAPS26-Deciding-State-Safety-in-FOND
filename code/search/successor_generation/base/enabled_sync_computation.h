//
// This file is part of the PlaJA code base.
// Copyright (c) (2019 - 2021) Marcel Vinzent.
// See README.md in the top-level directory for licensing information.
//

#ifndef PLAJA_ENABLED_SYNC_COMPUTATION_H
#define PLAJA_ENABLED_SYNC_COMPUTATION_H

namespace SUCCESSOR_GENERATION {

    template<typename Edge_t>
    inline void initialize_enablement_structures(const SynchronisationInformation& sync_info, std::vector<std::vector<std::list<Edge_t>>>& enabled_edges, std::vector<unsigned int>& enabled_synchronisations) {
        enabled_synchronisations.resize(sync_info.num_syncs, 0);
        enabled_edges.resize(sync_info.num_automata_instances);
        for (auto& enabled_per_automaton: enabled_edges) { enabled_per_automaton.resize(sync_info.num_actions); }
    }

    template<typename Edge_t>
    inline void compute_enabled_synchronisations(const SynchronisationInformation& sync_info, const std::vector<std::vector<std::list<Edge_t>>>& enabled_edges, std::vector<unsigned int>& enabled_synchronisations) {
        std::size_t num_ops_loc;
        for (SyncIndex_type sync_index = 0; sync_index < sync_info.num_syncs; ++sync_index) { // for each synchronisation
            num_ops_loc = 1; // number of ops enabled
            for (const auto& condition: sync_info.synchronisations[sync_index]) { // check whether for each participating automaton there is at least one respectively labeled edge
                num_ops_loc *= enabled_edges[condition.automatonIndex][condition.actionID].size();
                if (!num_ops_loc) { // num_ops_loc = 0 iff enabled_edges[automaton][action].size() = 0
                    break;
                }
            }
            enabled_synchronisations[sync_index] = num_ops_loc;
        }
    }

}

#endif //PLAJA_ENABLED_SYNC_COMPUTATION_H
