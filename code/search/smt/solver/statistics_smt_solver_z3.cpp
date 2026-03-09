//
// This file is part of the PlaJA code base.
// Copyright (c) (2019 - 2022) Marcel Vinzent.
// See README.md in the top-level directory for licensing information.
//

#include "statistics_smt_solver_z3.h"
#include "../../../stats/stats_base.h"

void Z3_IN_PLAJA::add_solver_stats(PLAJA::StatsBase& stats) {

    stats.add_unsigned_stats(
        std::list<PLAJA::StatsUnsigned> { PLAJA::StatsUnsigned::Z3_QUERIES, PLAJA::StatsUnsigned::Z3_QUERIES_UNSAT, PLAJA::StatsUnsigned::Z3_QUERIES_UNDECIDED },
        std::list<std::string> { "Z3Queries", "Z3QueriesUnsat", "Z3QueriesUndecided" },
        0
    );

    stats.add_double_stats(
        std::list<PLAJA::StatsDouble> { PLAJA::StatsDouble::TIME_Z3, PLAJA::StatsDouble::TIME_Z3_UNSAT, PLAJA::StatsDouble::TIME_Z3_PUSH, PLAJA::StatsDouble::TIME_Z3_POP, PLAJA::StatsDouble::TIME_Z3_RESET },
        std::list<std::string> { "TimeZ3Queries", "TimeZ3QueriesUnsat", "TimeZ3Push", "TimeZ3Pop", "TimeZ3Reset" },
        0.0
    );

}

//

#if 0
void Z3_IN_PLAJA::StatisticsSMTSolver::print_statistics_specific() const {
    if (get_attr_unsigned(PLAJA::StatsUnsigned::Z3_QUERIES) > 0) {
        PLAJA_LOG(PLAJA_UTILS::string_f("TimePerZ3Query: %f", get_attr_double(PLAJA::StatsDouble::TIME_Z3) / get_attr_unsigned(PLAJA::StatsUnsigned::Z3_QUERIES)));
        PLAJA_LOG_IF(get_attr_unsigned(PLAJA::StatsUnsigned::Z3_QUERIES_UNSAT) > 0, PLAJA_UTILS::string_f("TimePerZ3QueryUnsat: %f", get_attr_double(PLAJA::StatsDouble::TIME_Z3_UNSAT) / get_attr_unsigned(PLAJA::StatsUnsigned::Z3_QUERIES_UNSAT)));
        PLAJA_LOG_IF(get_attr_unsigned(PLAJA::StatsUnsigned::Z3_QUERIES) - get_attr_unsigned(PLAJA::StatsUnsigned::Z3_QUERIES_UNSAT) > 0, PLAJA_UTILS::string_f("TimePerZ3QuerySat: %f", (get_attr_double(PLAJA::StatsDouble::TIME_Z3) - get_attr_double(PLAJA::StatsDouble::TIME_Z3_UNSAT)) / (get_attr_unsigned(PLAJA::StatsUnsigned::Z3_QUERIES) - get_attr_unsigned(PLAJA::StatsUnsigned::Z3_QUERIES_UNSAT)) ));
    }
}
#endif