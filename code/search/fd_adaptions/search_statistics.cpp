//
// This file is part of the PlaJA code base.
// Copyright (c) (2019 - 2021) Marcel Vinzent.
// See README.md in the top-level directory for licensing information.
//

#include "search_statistics.h"
#include "../../stats/stats_base.h"

#if 0
const std::list<PLAJA::StatsDouble> SearchStatistics::timeAttrs { PLAJA::StatsDouble::PREPROCESSING_TIME, PLAJA::StatsDouble::CONSTRUCTION_TIME, PLAJA::StatsDouble::INITIALIZATION_TIME, PLAJA::StatsDouble::SEARCH_TIME, PLAJA::StatsDouble::FINALIZATION_TIME };
const std::list<std::string> SearchStatistics::timeAttrsStr { "PreprocessingTime", "ConstructionTime", "InitializationTime", "SearchTime", "FinalizationTime" };

SearchStatistics::SearchStatistics() { init_aux(timeAttrs, -1.0); }

SearchStatistics::~SearchStatistics() = default;

void SearchStatistics::reset_specific() { init_aux(timeAttrs, -1.0); }

/* */

void SearchStatistics::print_statistics_specific() const { print_aux(timeAttrs, timeAttrsStr, false); }

void SearchStatistics::stats_to_csv_specific(std::ofstream& file) const { stats_to_csv_aux(file, timeAttrs); }

void SearchStatistics::stat_names_to_csv_specific(std::ofstream& file) const { stat_names_to_csv_aux(file, timeAttrsStr); }
#endif

/* */

void SEARCH_STATISTICS::add_stats(PLAJA::StatsBase& stats) {

    stats.add_int_stats(PLAJA::StatsInt::STATUS, "Status", -1);

    stats.add_double_stats(
        std::list<PLAJA::StatsDouble> { PLAJA::StatsDouble::PREPROCESSING_TIME, PLAJA::StatsDouble::CONSTRUCTION_TIME, PLAJA::StatsDouble::INITIALIZATION_TIME, PLAJA::StatsDouble::SEARCH_TIME, PLAJA::StatsDouble::FINALIZATION_TIME },
        std::list<std::string> { "PreprocessingTime", "ConstructionTime", "InitializationTime", "SearchTime", "FinalizationTime" },
        0
    );

}

void SEARCH_STATISTICS::add_basic_stats(PLAJA::StatsBase& stats) {

    add_stats(stats);

    stats.add_unsigned_stats(
        std::list<PLAJA::StatsUnsigned> { PLAJA::StatsUnsigned::EXPANDED_STATES, PLAJA::StatsUnsigned::GENERATED_STATES,
                                          PLAJA::StatsUnsigned::ACTION_LABELS, PLAJA::StatsUnsigned::ACTION_OPS, PLAJA::StatsUnsigned::TRANSITIONS,
                                          PLAJA::StatsUnsigned::TERMINAL_STATES, PLAJA::StatsUnsigned::POLICY_TERMINAL_STATES,
                                          PLAJA::StatsUnsigned::START_STATES, PLAJA::StatsUnsigned::REACHABLE_STATES, PLAJA::StatsUnsigned::GOAL_STATES, PLAJA::StatsUnsigned::DEAD_END_STATES },
        std::list<std::string> { "ExpandedStates", "GeneratedStates", "ActionLabels", "ActionOps", "Transitions", "TerminalStates", "PolicyTerminalStates", "StartStates", "ReachableStates", "GoalStates", "DeadEndStates" },
        0
    );

}
