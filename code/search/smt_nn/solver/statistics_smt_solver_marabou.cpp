//
// This file is part of the PlaJA code base.
// Copyright (c) (2019 - 2022) Marcel Vinzent.
// See README.md in the top-level directory for licensing information.
//

#include "statistics_smt_solver_marabou.h"
#include "../../../stats/stats_base.h"

void MARABOU_IN_PLAJA::add_solver_stats(PLAJA::StatsBase& stats) {

    stats.add_unsigned_stats(
        std::list<PLAJA::StatsUnsigned> {
            PLAJA::StatsUnsigned::MARABOU_PREPROCESSED_UNSAT, PLAJA::StatsUnsigned::MARABOU_QUERIES, PLAJA::StatsUnsigned::MARABOU_QUERIES_UNSAT, PLAJA::StatsUnsigned::MARABOU_QUERIES_UNDECIDED, PLAJA::StatsUnsigned::MARABOU_INVALID_SOLUTIONS, PLAJA::StatsUnsigned::MARABOU_NUM_SPLITS, PLAJA::StatsUnsigned::MARABOU_NUM_POPS,
            PLAJA::StatsUnsigned::MARABOU_MAX_SPLIT_DEPTH,
            PLAJA::StatsUnsigned::MARABOU_LEARNED_CONFLICTS, PLAJA::StatsUnsigned::MARABOU_CASE_SPLITS_SKIPPED, PLAJA::StatsUnsigned::MARABOU_CASE_SPLITS_FIXED,
            PLAJA::StatsUnsigned::MARABOU_BB, PLAJA::StatsUnsigned::MARABOU_BB_UNSAT, PLAJA::StatsUnsigned::MARABOU_BB_EXACT_WITH_ARGMAX_TOLERANCE,
            PLAJA::StatsUnsigned::MARABOU_BB_NON_EXACT, PLAJA::StatsUnsigned::MARABOU_BB_NO_SOLUTION,
            PLAJA::StatsUnsigned::MARABOU_BB_NON_EXACT_INT, PLAJA::StatsUnsigned::MARABOU_BB_BRANCH_OOB, PLAJA::StatsUnsigned::MARABOU_BB_BRANCH_EXACT_INT, PLAJA::StatsUnsigned::MARABOU_BB_RECURSIONS, PLAJA::StatsUnsigned::MARABOU_BB_MAX_DEPTH,
        },
        std::list<std::string> {
            "MarabouPreprocessedUnsat", "MarabouQueries", "MarabouQueriesUnsat", "MarabouQueriesUndecided", "MarabouSolutionsInvalid", "MarabouNumSplits", "MarabouNumPops", "MarabouMaxSplitDepth", "MarabouLearnedConflicts", "MarabouCaseSplitsSkipped", "MarabouCaseSplitsFixed",
            "MarabouBB", "MarabouBBUnsat", "MarabouBBExactWithArgmaxTolerance", "MarabouBBNonExact", "MarabouBBNoSolution",
            "MarabouBBNonExactInt", "MarabouBBBranchOOB", "MarabouBBBranchExactInt", "MarabouRecursions", "MarabouBBMaxDepth",
        },
        0
    );

    stats.add_double_stats(
        std::list<PLAJA::StatsDouble> { PLAJA::StatsDouble::TIME_MARABOU_PREPROCESS, PLAJA::StatsDouble::TIME_MARABOU_PREPROCESS_MAX, PLAJA::StatsDouble::TIME_MARABOU_FOR_PREPROCESSED_UNSAT, PLAJA::StatsDouble::TIME_MARABOU_PREPROCESS_UNSAT, PLAJA::StatsDouble::TIME_MARABOU, PLAJA::StatsDouble::TIME_MARABOU_MAX, PLAJA::StatsDouble::TIME_MARABOU_UNSAT,
                                        PLAJA::StatsDouble::TIME_MARABOU_PUSH, PLAJA::StatsDouble::TIME_MARABOU_POP, PLAJA::StatsDouble::TIME_MARABOU_BB, PLAJA::StatsDouble::TIME_MARABOU_BB_UNSAT, PLAJA::StatsDouble::MARABOU_BB_NON_EXACT_INT_DELTA },
        std::list<std::string> { "TimeMarabouPreprocess", "TimeMarabouPreprocessMax", "TimeMarabouForPreprocessedUnsat", "TimeMarabouPreprocessUnsat", "TimeMarabouQueries", "TimeMarabouQueryMax", "TimeMarabouQueriesUnsat", "TimeMarabouPush", "TimeMarabouPop", "TimeMarabouBB", "TimeMarabouBBUnsat", "MarabouBBNonExactIntDelta" },
        0.0
    );

}

//

#if 0
void MARABOU_IN_PLAJA::StatisticsSMTSolver::print_statistics_specific() const {
    // auxiliary outputs:
    if (get_attr_unsigned(PLAJA::StatsUnsigned::MARABOU_QUERIES) > 0) {
        PLAJA_LOG(PLAJA_UTILS::string_f("TimePerMarabouQuery: %f", get_attr_double(PLAJA::StatsDouble::TIME_MARABOU) / get_attr_unsigned(PLAJA::StatsUnsigned::MARABOU_QUERIES)));
        PLAJA_LOG_IF(get_attr_unsigned(PLAJA::StatsUnsigned::MARABOU_QUERIES_UNSAT) > 0, PLAJA_UTILS::string_f("TimePerMarabouQueryUnsat: %f", get_attr_double(PLAJA::StatsDouble::TIME_MARABOU_UNSAT) / get_attr_unsigned(PLAJA::StatsUnsigned::MARABOU_QUERIES_UNSAT)));
        PLAJA_LOG_IF(get_attr_unsigned(PLAJA::StatsUnsigned::MARABOU_QUERIES) - get_attr_unsigned(PLAJA::StatsUnsigned::MARABOU_QUERIES_UNSAT) > 0, PLAJA_UTILS::string_f("TimePerMarabouQuerySat: %f", (get_attr_double(PLAJA::StatsDouble::TIME_MARABOU) - get_attr_double(PLAJA::StatsDouble::TIME_MARABOU_UNSAT)) / (get_attr_unsigned(PLAJA::StatsUnsigned::MARABOU_QUERIES) - get_attr_unsigned(PLAJA::StatsUnsigned::MARABOU_QUERIES_UNSAT)) ));
    }

    if (get_attr_unsigned(PLAJA::StatsUnsigned::MARABOU_BB) > 0) {
        PLAJA_LOG(PLAJA_UTILS::string_f("TimePerBB: %f", get_attr_double(PLAJA::StatsDouble::TIME_MARABOU_BB) / get_attr_unsigned(PLAJA::StatsUnsigned::MARABOU_BB)));
        PLAJA_LOG_IF(get_attr_unsigned(PLAJA::StatsUnsigned::MARABOU_BB_UNSAT) > 0, PLAJA_UTILS::string_f("TimePerBBUnsat: %f", get_attr_double(PLAJA::StatsDouble::TIME_MARABOU_BB_UNSAT) / get_attr_unsigned(PLAJA::StatsUnsigned::MARABOU_BB_UNSAT)));
        PLAJA_LOG_IF(get_attr_unsigned(PLAJA::StatsUnsigned::MARABOU_BB) - get_attr_unsigned(PLAJA::StatsUnsigned::MARABOU_BB_UNSAT) > 0, PLAJA_UTILS::string_f("TimePerBBSat: %f", (get_attr_double(PLAJA::StatsDouble::TIME_MARABOU_BB) - get_attr_double(PLAJA::StatsDouble::TIME_MARABOU_BB_UNSAT)) / (get_attr_unsigned(PLAJA::StatsUnsigned::MARABOU_BB) - get_attr_unsigned(PLAJA::StatsUnsigned::MARABOU_BB_UNSAT)) ));
    }
}
#endif