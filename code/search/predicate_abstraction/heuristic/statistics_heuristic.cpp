//
// This file is part of the PlaJA code base.
// Copyright (c) (2019 - 2022) Marcel Vinzent.
// See README.md in the top-level directory for licensing information.
//

#include <string>
#include "statistics_heuristic.h"
#include "../../../stats/stats_base.h"

void ABSTRACT_HEURISTIC::add_stats(PLAJA::StatsBase& stats) {

    stats.add_unsigned_stats(
        std::list<PLAJA::StatsUnsigned> { PLAJA::StatsUnsigned::HEURISTIC_CALLS, PLAJA::StatsUnsigned::HEURISTIC_CACHE_HITS, PLAJA::StatsUnsigned::UPDATED_ESTIMATES },
        std::list<std::string> { "HeuristicCalls", "HeuristicCacheHits", "UpdatedEstimates" },
        0
    );

    stats.add_double_stats(PLAJA::StatsDouble::HEURISTIC_COMPUTATION_TIME, "HeuristicComputationTime", 0.0);

}