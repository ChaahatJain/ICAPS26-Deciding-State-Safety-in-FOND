//
// This file is part of the PlaJA code base.
// Copyright (c) (2019 - 2021) Marcel Vinzent.
// See README.md in the top-level directory for licensing information.
//

#include <fstream>
#include <sstream>
#include "learning_statistics.h"
#include "../search/fd_adaptions/search_statistics.h"
#include "../stats/stats_base.h"

LearningStatistics::LearningStatistics():
    totalEpsilonGreedySelections(0) {}

LearningStatistics::~LearningStatistics() = default;

void LearningStatistics::add_basic_stats(PLAJA::StatsBase& stats) {

    SEARCH_STATISTICS::add_basic_stats(stats);

    stats.add_unsigned_stats(
        std::list<PLAJA::StatsUnsigned> {
            PLAJA::StatsUnsigned::EPISODES, PLAJA::StatsUnsigned::POLICY_ACTS, PLAJA::StatsUnsigned::SUB_RANK_ACTS, PLAJA::StatsUnsigned::EPSILON_GREEDY_ACTS,
            PLAJA::StatsUnsigned::TeacherActs, PLAJA::StatsUnsigned::TeacherActsUnsuccessful,
            PLAJA::StatsUnsigned::DONE_GOAL, PLAJA::StatsUnsigned::DONE_AVOID, PLAJA::StatsUnsigned::DONE_STALLING, PLAJA::StatsUnsigned::UNDONE,
            PLAJA::StatsUnsigned::DETECTED_CYCLES, PLAJA::StatsUnsigned::UNSAFE_START_STATES, PLAJA::StatsUnsigned::RESAMPLES, PLAJA::StatsUnsigned::TIE_BREAKS,
        },
        std::list<std::string> {
            "Episodes", "PolicyActs", "SubRankActs", "EpsilonGreedyActs", "TeacherActs", "TeacherActsUnsuccessful",
            "DoneGoal", "DoneAvoid", "DoneStalling", "Undone", "DetectedCycles", "UnsafeStartStates","Resamples", "TieBreaks"
        },
        0
    );

    stats.add_double_stats(
        std::list<PLAJA::StatsDouble> { PLAJA::StatsDouble::BEST_SCORE, PLAJA::StatsDouble::AVERAGE_LENGTH_BEST_SCORE, PLAJA::StatsDouble::LAST_SCORE, PLAJA::StatsDouble::LAST_AVERAGE_LENGTH, PLAJA::StatsDouble::EPSILON, PLAJA::StatsDouble::TimeForTeacher, PLAJA::StatsDouble::AVG_SAMPLED_POLICY_LENGTH, PLAJA::StatsDouble::DISCRIMINATION_RATE },
        std::list<std::string> { "BestScore", "AverageLengthBestScore", "LastScore", "LastAverageLength", "Epsilon", "TimeForTeacher", "AvgSampledPolicyLength", "DiscriminationRate" },
        -1
    );

}

//

void LearningStatistics::reset() {
    for (auto& action_count: epsilonGreedySelectionsPerAction) { action_count.second = 0; }
    totalEpsilonGreedySelections = 0;
}

void LearningStatistics::print_statistics() const {
    std::stringstream ss;
    ss << "TotalEpsilonGreedySelections:";
    for (const auto& [action_label, count]: epsilonGreedySelectionsPerAction) { ss << PLAJA_UTILS::spaceString << "(" << action_label << PLAJA_UTILS::commaString << PLAJA_UTILS::spaceString << (static_cast<double>(count) / totalEpsilonGreedySelections) << ")"; }
    ss << std::endl;
    PLAJA_LOG(ss.str());
}

void LearningStatistics::stats_to_csv(std::ofstream& file) const {
    file << PLAJA_UTILS::commaString << totalEpsilonGreedySelections;
    for (const auto& action_label_count: epsilonGreedySelectionsPerAction) { file << PLAJA_UTILS::commaString << action_label_count.second; }
}

void LearningStatistics::stat_names_to_csv(std::ofstream& file) const {
    file << PLAJA_UTILS::commaString << "TotalEpsilonGreedySelections";
    for (const auto& action_label_count: epsilonGreedySelectionsPerAction) { file << PLAJA_UTILS::commaString << action_label_count.first; }

}
