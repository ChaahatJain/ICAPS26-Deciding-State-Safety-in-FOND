//
// This file is part of the PlaJA code base.
// Copyright (c) (2019 - 2019) Marcel Vinzent.
// See README.md in the top-level directory for licensing information.
//

#include "breadth_first_search.h"
#include "../../option_parser/option_parser.h"
#include "../../option_parser/plaja_options.h"
#include "../../stats/stats_base.h"
#include "base/search_space_non_prob.h"

BFSearch::BFSearch(const PLAJA::Configuration& config):
    SearchEngineNonProb(config, true, false, true)
    , searchSpace(new SearchSpaceNonProb(*model)) {

    searchStatistics->add_int_stats(PLAJA::StatsInt::PATH_LENGTH, "PathLength", -1); // Set stats properly.

    SearchEngineNonProb::set_search_space(searchSpace.get());
}

BFSearch::~BFSearch() = default;

/* */

SearchEngine::SearchStatus BFSearch::finalize() {

    if (not searchSpace->goal_path_frontier_empty()) {

        PLAJA_LOG("Goal state reachable!")

        if (PLAJA_GLOBAL::has_value_option(PLAJA_OPTION::savePath)) {
            const auto path_len = PLAJA_UTILS::cast_numeric<int>(searchSpace->dump_path(searchSpace->goal_frontier_state(), model).size());
            searchStatistics->set_attr_int(PLAJA::StatsInt::PATH_LENGTH, path_len); // Set stats properly.
        } else {
            const auto path_len = PLAJA_UTILS::cast_numeric<int>(searchSpace->trace_path(searchSpace->goal_frontier_state()).size());
            searchStatistics->set_attr_int(PLAJA::StatsInt::PATH_LENGTH, path_len);
        }

    } else {
        searchStatistics->set_attr_int(PLAJA::StatsInt::PATH_LENGTH, 0);
    }

    return SearchEngine::FINISHED;
}