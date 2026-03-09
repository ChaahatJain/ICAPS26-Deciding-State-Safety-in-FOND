#include "reduction_statistics.h"
#include "../search/fd_adaptions/search_statistics.h"
#include "../stats/stats_base.h"

ReductionStatistics::ReductionStatistics():
    totalReducedNeurons(0) {}

ReductionStatistics::~ReductionStatistics() = default;

void ReductionStatistics::add_basic_stats(PLAJA::StatsBase& stats) {
    SEARCH_STATISTICS::add_basic_stats(stats);
    stats.add_unsigned_stats(
        std::list<PLAJA::StatsUnsigned> {
            PLAJA::StatsUnsigned::EPISODES, PLAJA::StatsUnsigned::POLICY_ACTS, PLAJA::StatsUnsigned::SUB_RANK_ACTS, PLAJA::StatsUnsigned::DONE_GOAL,
            PLAJA::StatsUnsigned::DONE_AVOID, PLAJA::StatsUnsigned::DONE_STALLING, PLAJA::StatsUnsigned::UNDONE, PLAJA::StatsUnsigned::DETECTED_CYCLES,
            PLAJA::StatsUnsigned::UNSAFE_START_STATES
        },
        std::list<std::string> {
            "Episodes", "PolicyActs", "SubRankActs", "DoneGoal", "DoneAvoid", "DoneStalling", "Undone", "DetectedCycles", "UnsafeStartStates"
        },
        0
    );
    stats.add_double_stats(
        std::list<PLAJA::StatsDouble> { PLAJA::StatsDouble::TRAIN_GOAL_FRACTION },
        std::list<std::string> { "GoalFraction" },
        0
    );
}

void ReductionStatistics::reset() {
    totalReducedNeurons = 0;
    reducedNeuronsPerLayer.clear();

}

void ReductionStatistics::print_statistics() const {
    std::stringstream ss;
    ss << "TotalReducedNeurons:" << PLAJA_UTILS::spaceString << totalReducedNeurons << "\n";
    ss << "ReducedNeuronsPerHiddenLayer:";
    if (reducedNeuronsPerLayer.empty()) {
        ss << PLAJA_UTILS::spaceString << "No reduction";
    } else {
        for (const auto& [layer, neurons]: reducedNeuronsPerLayer) { ss << PLAJA_UTILS::spaceString << "(" << layer << PLAJA_UTILS::commaString << PLAJA_UTILS::spaceString << (neurons) << ")"; }
    }
    ss << std::endl;
    PLAJA_LOG(ss.str())
}

void ReductionStatistics::stats_to_csv(std::ofstream& file) const {
    file << PLAJA_UTILS::commaString << totalReducedNeurons;
    for (const auto& layer_neurons: reducedNeuronsPerLayer) { file << PLAJA_UTILS::commaString << layer_neurons.second; }
}

void ReductionStatistics::stat_names_to_csv(std::ofstream& file) const {
    file << PLAJA_UTILS::commaString << "TotalReducedNeurons";
    for (const auto& layer_neurons: reducedNeuronsPerLayer) { file << PLAJA_UTILS::commaString << layer_neurons.first; }
}
