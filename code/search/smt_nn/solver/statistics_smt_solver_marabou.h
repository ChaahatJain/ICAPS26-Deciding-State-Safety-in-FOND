//
// This file is part of the PlaJA code base.
// Copyright (c) (2019 - 2022) Marcel Vinzent.
// See README.md in the top-level directory for licensing information.
//

#ifndef PLAJA_STATISTICS_SMT_SOLVER_MARABOU_H
#define PLAJA_STATISTICS_SMT_SOLVER_MARABOU_H

#if 0
#include "../../../stats/stats_base.h"

namespace MARABOU_IN_PLAJA {

class StatisticsSMTSolver final: public PLAJA::StatsBase {
private:
    void print_statistics_specific() const override;
public:
    StatisticsSMTSolver();
    ~StatisticsSMTSolver() override;

    StatisticsSMTSolver(const StatisticsSMTSolver& other) = default;
    StatisticsSMTSolver(StatisticsSMTSolver&& other) = default;
    StatisticsSMTSolver& operator=(const StatisticsSMTSolver& other) = default;
    StatisticsSMTSolver& operator=(StatisticsSMTSolver&& other) = default;
};

}
#endif

namespace PLAJA { class StatsBase; }
namespace MARABOU_IN_PLAJA { extern void add_solver_stats(PLAJA::StatsBase& stats); }

#endif //PLAJA_STATISTICS_SMT_SOLVER_MARABOU_H
