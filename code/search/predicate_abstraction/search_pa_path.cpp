//
// This file is part of the PlaJA code base.
// Copyright (c) (2019 - 2021) Marcel Vinzent.
// See README.md in the top-level directory for licensing information.
//

#include <memory>
#include "search_pa_path.h"
#include "../../stats/stats_base.h"
#include "../factories/predicate_abstraction/search_engine_config_pa.h"
#include "base/expansion_pa_base.h"
#include "heuristic/state_queue.h"
#include "pa_states/abstract_path.h"
#include "search_space/search_space_pa_base.h"

SearchPAPath::SearchPAPath(const SearchEngineConfigPA& config, bool witness_interval):
    SearchPABase(config, true, false, witness_interval) {
}

SearchPAPath::~SearchPAPath() = default;

/**********************************************************************************************************************/

HeuristicValue_type SearchPAPath::search_path(StateID_type src_id) {

    frontier->clear();

    pathLength = -1;
    searchStatistics->set_attr_int(PLAJA::StatsInt::PATH_LENGTH, pathLength);

    searchSpace->reset();
    PLAJA_ASSERT(searchSpace->goal_path_frontier_empty())

    auto node = searchSpace->get_node(src_id);
    node.set_reached_init();
    frontier->push(node);

    while (not frontier->empty()) {

        const auto newly_reached = expansionPa->step(frontier->pop());

        PLAJA_ASSERT(goalPathSearch and not optimalSearch)
        if (/*goalPathSearch and not optimalSearch and*/ not searchSpace->goal_path_frontier_empty()) { break; }

        /* Update frontier. */
        for (const StateID_type& state_id: newly_reached) { frontier->push(searchSpace->get_node(state_id)); }

    }

    // Set.
    if (not searchSpace->goal_path_frontier_empty()) {
        searchSpace->compute_goal_path_states();
        return searchSpace->get_node_info(src_id).get_goal_distance();
    } else { // Set dead ends.
        /* while (not frontier->empty()) { searchSpace->compute_dead_ends_backwards(frontier->pop()); } */
        PLAJA_ABORT; // Do not expect that to happen on the considered benchmarks.
        return HEURISTIC::deadEnd;
    }

}

HeuristicValue_type SearchPAPath::get_goal_distance(const AbstractState& pa_state) {
    auto node = searchSpace->add_node(searchSpace->set_abstract_state(pa_state));
    if (node().known_goal_distance()) {
        statsHeuristic->inc_attr_unsigned(PLAJA::StatsUnsigned::HEURISTIC_CACHE_HITS);
        return node().get_goal_distance();
    } else { return search_path(node.get_id()); }
}