//
// This file is part of the PlaJA code base.
// Copyright (c) (2019 - 2022) Marcel Vinzent.
// See README.md in the top-level directory for licensing information.
//

#ifndef PLAJA_STATISTICS_HEURISTIC_H
#define PLAJA_STATISTICS_HEURISTIC_H

#if 0
#include "../../../stats/stats_base.h"

class StatisticsHeuristic final: public PLAJA::StatsBase {
public:
    StatisticsHeuristic();
    ~StatisticsHeuristic() override;

    StatisticsHeuristic(const StatisticsHeuristic& other) = delete;
    StatisticsHeuristic(StatisticsHeuristic&& other) = default;
    StatisticsHeuristic& operator=(const StatisticsHeuristic& other) = delete;
    StatisticsHeuristic& operator=(StatisticsHeuristic&& other) = default;
};
#endif

namespace PLAJA { class StatsBase; }
namespace ABSTRACT_HEURISTIC { extern void add_stats(PLAJA::StatsBase& stats); }

#endif //PLAJA_STATISTICS_HEURISTIC_H
