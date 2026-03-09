//
// This file is part of the PlaJA code base.
// Copyright (c) (2019 - 2021) Marcel Vinzent.
// See README.md in the top-level directory for licensing information.
//

#ifndef PLAJA_LEARNING_STATISTICS_H
#define PLAJA_LEARNING_STATISTICS_H

#include <unordered_map>
#include "../utils/default_constructors.h"
#include "../assertions.h"

namespace PLAJA { class StatsBase; }


/**
 * Wrapper for learning-specific statistics structure.
 */
class LearningStatistics final {

private:
    //
    std::unordered_map<std::string, unsigned int> epsilonGreedySelectionsPerAction;
    unsigned int totalEpsilonGreedySelections; // just fto output percentage, actual stats value is stored in base class

public:
    LearningStatistics();
    ~LearningStatistics();
    DELETE_CONSTRUCTOR(LearningStatistics)

    // specialized & auxiliary:
    inline void set_epsilon_greedy_selection(const std::string& action_label) { PLAJA_ASSERT(!epsilonGreedySelectionsPerAction.count(action_label)) epsilonGreedySelectionsPerAction.emplace(action_label, 0); }
    inline void inc_epsilon_greedy_selection(const std::string& action_label, int inc = 1) {
        totalEpsilonGreedySelections += inc;
        epsilonGreedySelectionsPerAction[action_label] += inc;
    }

    static void add_basic_stats(PLAJA::StatsBase& stats);

    void reset();

    // output
    void print_statistics() const;
    void stats_to_csv(std::ofstream& file) const;
    void stat_names_to_csv(std::ofstream& file) const;

};

#endif //PLAJA_LEARNING_STATISTICS_H
