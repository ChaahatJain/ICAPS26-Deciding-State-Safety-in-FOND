//
// This file is part of the PlaJA code base.
// Copyright (c) (2019 - 2020) Marcel Vinzent.
// See README.md in the top-level directory for licensing information.
//

#ifndef PLAJA_SEARCH_STATISTICS_PA_H
#define PLAJA_SEARCH_STATISTICS_PA_H

#if 0

#include "../fd_adaptions/search_statistics_basic.h"

class SearchStatisticsPA: public SearchStatisticsBasic {

#if 0
private:
    std::unique_ptr<PLAJA::StatsBase> statsHeuristic;
    std::unique_ptr<PLAJA::StatsBase> statsSMTZ3;
    std::unique_ptr<PLAJA::StatsBase> statsSMTMarabou;
    std::unique_ptr<PLAJA::StatsBase> statsNNSat;

protected:
    void reset_specific() override;

    // output
    void print_statistics_specific() const override;
    void stats_to_csv_specific(std::ofstream& file) const override;
    void stat_names_to_csv_specific(std::ofstream& file) const override;
#endif

public:
    SearchStatisticsPA();
    ~SearchStatisticsPA() override;

#if 0
    inline PLAJA::StatsBase& share_heuristic_stats() { return *statsHeuristic; }
    inline PLAJA::StatsBase& share_smt_stats_z3() { return *statsSMTZ3; }
    inline PLAJA::StatsBase& share_smt_stats_marabou() { return *statsSMTMarabou; }
    inline PLAJA::StatsBase& share_nn_sat_stats() { return *statsNNSat; }
#endif


    DELETE_CONSTRUCTOR(SearchStatisticsPA)
};

#endif

namespace PLAJA { class StatsBase; }
namespace SEARCH_PA_BASE { extern void add_stats(PLAJA::StatsBase& stats); }

#endif //PLAJA_SEARCH_STATISTICS_PA_H
