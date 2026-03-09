//
// This file is part of the PlaJA code base.
// Copyright (c) (2023 - 2024) Chaahat Jain.
//

#include "statistics_smt_solver_veritas.h"
#include "../../../stats/stats_base.h"

void VERITAS_IN_PLAJA::add_solver_stats(PLAJA::StatsBase& stats) {

    stats.add_unsigned_stats(
        std::list<PLAJA::StatsUnsigned> {
            PLAJA::StatsUnsigned::VERITAS_PREPROCESSED_UNSAT, PLAJA::StatsUnsigned::VERITAS_QUERIES, PLAJA::StatsUnsigned::VERITAS_QUERIES_UNSAT, PLAJA::StatsUnsigned::VERITAS_QUERIES_UNDECIDED, PLAJA::StatsUnsigned::VERITAS_INVALID_SOLUTIONS, PLAJA::StatsUnsigned::VERITAS_NUM_SPLITS, PLAJA::StatsUnsigned::VERITAS_NUM_POPS,
            PLAJA::StatsUnsigned::VERITAS_MAX_SPLIT_DEPTH,
            PLAJA::StatsUnsigned::VERITAS_LEARNED_CONFLICTS, PLAJA::StatsUnsigned::VERITAS_CASE_SPLITS_SKIPPED, PLAJA::StatsUnsigned::VERITAS_CASE_SPLITS_FIXED,
            PLAJA::StatsUnsigned::VERITAS_BB, PLAJA::StatsUnsigned::VERITAS_BB_UNSAT, PLAJA::StatsUnsigned::VERITAS_BB_EXACT_WITH_ARGMAX_TOLERANCE,
            PLAJA::StatsUnsigned::VERITAS_BB_NON_EXACT, PLAJA::StatsUnsigned::VERITAS_BB_NO_SOLUTION,
            PLAJA::StatsUnsigned::VERITAS_BB_NON_EXACT_INT, PLAJA::StatsUnsigned::VERITAS_BB_BRANCH_OOB, PLAJA::StatsUnsigned::VERITAS_BB_BRANCH_EXACT_INT, PLAJA::StatsUnsigned::VERITAS_BB_RECURSIONS, PLAJA::StatsUnsigned::VERITAS_BB_MAX_DEPTH,
        },
        std::list<std::string> {
            "VeritasPreprocessedUnsat", "VeritasQueries", "VeritasQueriesUnsat", "VeritasQueriesUndecided", "VeritasSolutionsInvalid", "VeritasNumSplits", "VeritasNumPops", "VeritasMaxSplitDepth", "VeritasLearnedConflicts", "VeritasCaseSplitsSkipped", "VeritasCaseSplitsFixed",
            "VeritasBB", "VeritasBBUnsat", "VeritasBBExactWithArgmaxTolerance", "VeritasBBNonExact", "VeritasBBNoSolution",
            "VeritasBBNonExactInt", "VeritasBBBranchOOB", "VeritasBBBranchExactInt", "VeritasRecursions", "VeritasBBMaxDepth",
        },
        0
    );

    stats.add_double_stats(
        std::list<PLAJA::StatsDouble> { PLAJA::StatsDouble::TIME_VERITAS_PREPROCESS, PLAJA::StatsDouble::TIME_VERITAS_PREPROCESS_MAX, PLAJA::StatsDouble::TIME_VERITAS_FOR_PREPROCESSED_UNSAT, PLAJA::StatsDouble::TIME_VERITAS_PREPROCESS_UNSAT, PLAJA::StatsDouble::TIME_VERITAS, PLAJA::StatsDouble::TIME_VERITAS_MAX, PLAJA::StatsDouble::TIME_VERITAS_UNSAT,
                                        PLAJA::StatsDouble::TIME_VERITAS_PUSH, PLAJA::StatsDouble::TIME_VERITAS_POP, PLAJA::StatsDouble::TIME_VERITAS_BB, PLAJA::StatsDouble::TIME_VERITAS_BB_UNSAT, PLAJA::StatsDouble::VERITAS_BB_NON_EXACT_INT_DELTA, PLAJA::StatsDouble::TIME_VERITAS_SAT_MAX, PLAJA::StatsDouble::TIME_VERITAS_UNSAT_MAX },
        std::list<std::string> { "TimeVeritasPreprocess", "TimeVeritasPreprocessMax", "TimeVeritasForPreprocessedUnsat", "TimeVeritasPreprocessUnsat", "TimeVeritasQueries", "TimeVeritasQueryMax", "TimeVeritasQueriesUnsat", "TimeVeritasPush", "TimeVeritasPop", "TimeVeritasBB", "TimeVeritasBBUnsat", "VeritasBBNonExactIntDelta", "TimeVeritasSatMax", "TimeVeritasUnsatMax" },
        0.0
    );

}

//

#if 0
void VERITAS_IN_PLAJA::StatisticsSMTSolver::print_statistics_specific() const {
    // auxiliary outputs:
    if (get_attr_unsigned(PLAJA::StatsUnsigned::VERITAS_QUERIES) > 0) {
        PLAJA_LOG(PLAJA_UTILS::string_f("TimePerVeritasQuery: %f", get_attr_double(PLAJA::StatsDouble::TIME_VERITAS) / get_attr_unsigned(PLAJA::StatsUnsigned::VERITAS_QUERIES)));
        PLAJA_LOG_IF(get_attr_unsigned(PLAJA::StatsUnsigned::VERITAS_QUERIES_UNSAT) > 0, PLAJA_UTILS::string_f("TimePerVeritasQueryUnsat: %f", get_attr_double(PLAJA::StatsDouble::TIME_VERITAS_UNSAT) / get_attr_unsigned(PLAJA::StatsUnsigned::VERITAS_QUERIES_UNSAT)));
        PLAJA_LOG_IF(get_attr_unsigned(PLAJA::StatsUnsigned::VERITAS_QUERIES) - get_attr_unsigned(PLAJA::StatsUnsigned::VERITAS_QUERIES_UNSAT) > 0, PLAJA_UTILS::string_f("TimePerVeritasQuerySat: %f", (get_attr_double(PLAJA::StatsDouble::TIME_VERITAS) - get_attr_double(PLAJA::StatsDouble::TIME_VERITAS_UNSAT)) / (get_attr_unsigned(PLAJA::StatsUnsigned::VERITAS_QUERIES) - get_attr_unsigned(PLAJA::StatsUnsigned::VERITAS_QUERIES_UNSAT)) ));
    }

    if (get_attr_unsigned(PLAJA::StatsUnsigned::VERITAS_BB) > 0) {
        PLAJA_LOG(PLAJA_UTILS::string_f("TimePerBB: %f", get_attr_double(PLAJA::StatsDouble::TIME_VERITAS_BB) / get_attr_unsigned(PLAJA::StatsUnsigned::VERITAS_BB)));
        PLAJA_LOG_IF(get_attr_unsigned(PLAJA::StatsUnsigned::VERITAS_BB_UNSAT) > 0, PLAJA_UTILS::string_f("TimePerBBUnsat: %f", get_attr_double(PLAJA::StatsDouble::TIME_VERITAS_BB_UNSAT) / get_attr_unsigned(PLAJA::StatsUnsigned::VERITAS_BB_UNSAT)));
        PLAJA_LOG_IF(get_attr_unsigned(PLAJA::StatsUnsigned::VERITAS_BB) - get_attr_unsigned(PLAJA::StatsUnsigned::VERITAS_BB_UNSAT) > 0, PLAJA_UTILS::string_f("TimePerBBSat: %f", (get_attr_double(PLAJA::StatsDouble::TIME_VERITAS_BB) - get_attr_double(PLAJA::StatsDouble::TIME_VERITAS_BB_UNSAT)) / (get_attr_unsigned(PLAJA::StatsUnsigned::VERITAS_BB) - get_attr_unsigned(PLAJA::StatsUnsigned::VERITAS_BB_UNSAT)) ));
    }
}
#endif