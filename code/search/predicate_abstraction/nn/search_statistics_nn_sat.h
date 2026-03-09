//
// This file is part of the PlaJA code base.
// Copyright (c) (2019 - 2022) Marcel Vinzent.
// See README.md in the top-level directory for licensing information.
//

#ifndef PLAJA_SEARCH_STATISTICS_NN_SAT_H
#define PLAJA_SEARCH_STATISTICS_NN_SAT_H

#if 0
#include "../../../stats/stats_base.h"

/** Sub class to encapsulate NN-SAT related stats.  */
class SearchStatisticsNNSat final: public PLAJA::StatsBase {

private:
    // auxiliary field
    PLAJA::StatsBase* smtStatsZ3;
    PLAJA::StatsBase* smtStatsMarabou;

    void print_statistics_specific() const override;
public:
    SearchStatisticsNNSat();
    ~SearchStatisticsNNSat() override;

    SearchStatisticsNNSat(const SearchStatisticsNNSat& other) = delete;
    SearchStatisticsNNSat(SearchStatisticsNNSat&& other) = default;
    SearchStatisticsNNSat& operator=(const SearchStatisticsNNSat& other) = delete;
    SearchStatisticsNNSat& operator=(SearchStatisticsNNSat&& other) = default;

    inline void set_smt_stats_z3(PLAJA::StatsBase* shared_stats) { smtStatsZ3 = shared_stats; }
    inline void set_smt_stats_marabou(PLAJA::StatsBase* shared_stats) { smtStatsMarabou = shared_stats; }
    [[nodiscard]] inline PLAJA::StatsBase* share_smt_stats_z3() { return smtStatsZ3; }
    [[nodiscard]] inline PLAJA::StatsBase* share_smt_stats_marabou() { return smtStatsMarabou; }
};
#endif

namespace PLAJA { class StatsBase; }
namespace NN_SAT { extern void add_stats(PLAJA::StatsBase& stats); }

#endif //PLAJA_SEARCH_STATISTICS_NN_SAT_H
