//
// This file is part of the PlaJA code base.
// Copyright (c) (2019 - 2022) Marcel Vinzent.
// See README.md in the top-level directory for licensing information.
//

#include <string>
#include "search_statistics_cegar.h"
#include "../../../stats/stats_base.h"
#include "../../smt/solver/statistics_smt_solver_z3.h"
#include "../../smt_nn/solver/statistics_smt_solver_marabou.h"
#include "../../smt_ensemble/solver/statistics_smt_solver_veritas.h"

#if 0

const std::list<PLAJA::StatsUnsigned> SearchStatisticsCegar::unsignedAttrs { PLAJA::StatsUnsigned::ITERATIONS, PLAJA::StatsUnsigned::REF_DUE_TO_CONCRETIZATION, PLAJA::StatsUnsigned::REF_DUE_TO_GUARD_APP, PLAJA::StatsUnsigned::REF_DUE_TO_ACTION_SEL, PLAJA::StatsUnsigned::REF_DUE_TO_TARGET, PLAJA::StatsUnsigned::PREDICATES_FOR_ACTION_SEL_REF };
const std::list<std::string> SearchStatisticsCegar::unsignedAttrsStr { "Iterations", "RefDueToConcretization", "RefDueToGuardApp", "RefDueToActionSel", "RefDueToTarget", "PredicatesForActionSelRef" };
const std::list<PLAJA::StatsDouble> SearchStatisticsCegar::doubleAttrs { PLAJA::StatsDouble::INCREMENTATION_TIME, PLAJA::StatsDouble::TIME_FOR_PA };
const std::list<std::string> SearchStatisticsCegar::doubleAttrsStr { "IncrementationTime", "TimeForPA" };

SearchStatisticsCegar::SearchStatisticsCegar() { init_aux(unsignedAttrs); init_aux(doubleAttrs, 0.0); }

SearchStatisticsCegar::~SearchStatisticsCegar() = default;

void SearchStatisticsCegar::reset_specific() { SearchStatistics::reset_specific(); init_aux(unsignedAttrs); init_aux(doubleAttrs, 0.0); }

//

void SearchStatisticsCegar::print_statistics_specific() const {
    SearchStatistics::print_statistics_specific();
    print_aux(unsignedAttrs, unsignedAttrsStr, false);
    print_aux(doubleAttrs, doubleAttrsStr, false);
}

void SearchStatisticsCegar::stats_to_csv_specific(std::ofstream& file) const {
    SearchStatistics::stats_to_csv_specific(file);
    stats_to_csv_aux(file, unsignedAttrs);
    stats_to_csv_aux(file, doubleAttrs);
}

void SearchStatisticsCegar::stat_names_to_csv_specific(std::ofstream& file) const {
    SearchStatistics::stat_names_to_csv_specific(file);
    stat_names_to_csv_aux(file, unsignedAttrsStr);
    stat_names_to_csv_aux(file, doubleAttrsStr);
}

#endif

void PA_CEGAR::add_stats(PLAJA::StatsBase& stats) {

    stats.add_int_stats(
        std::list<PLAJA::StatsInt> { PLAJA::StatsInt::HAS_GOAL_PATH },
        { "HasGoalPath" },
        -1 // unknown
    );

    stats.add_unsigned_stats(
        std::list<PLAJA::StatsUnsigned> { PLAJA::StatsUnsigned::ITERATIONS, PLAJA::StatsUnsigned::REF_DUE_TO_CONCRETIZATION,
#ifdef ENABLE_TERMINAL_STATE_SUPPORT
                                          PLAJA::StatsUnsigned::REF_DUE_TO_TERMINAL,
#endif
                                          PLAJA::StatsUnsigned::REF_DUE_TO_GUARD_APP, PLAJA::StatsUnsigned::REF_DUE_TO_ACTION_SEL, PLAJA::StatsUnsigned::REF_DUE_TO_TARGET },
        std::list<std::string> { "Iterations", "RefDueToConcretization",
#ifdef ENABLE_TERMINAL_STATE_SUPPORT
                                 "RefDueToTerminal",
#endif
                                 "RefDueToGuardApp", "RefDueToActionSel", "RefDueToTarget" },
        0
    );

    stats.add_unsigned_stats(PLAJA::StatsUnsigned::GLOBAL_PREDICATES_ADDED, "GlobalPredicatesAdded", 0);
    if constexpr (PLAJA_GLOBAL::lazyPA) { stats.add_unsigned_stats(PLAJA::StatsUnsigned::LOCAL_PREDICATES_ADDED, "LocalPredicatesAdded", 0); }
    stats.add_unsigned_stats(PLAJA::StatsUnsigned::PREDICATES_FOR_ACTION_SEL_REF, "PredicatesForActionSelRef", 0);

    stats.add_double_stats(
        std::list<PLAJA::StatsDouble> { PLAJA::StatsDouble::INCREMENTATION_TIME, PLAJA::StatsDouble::TIME_FOR_PA },
        std::list<std::string> { "IncrementationTime", "TimeForPA" },
        0.0
    );

    Z3_IN_PLAJA::add_solver_stats(stats);
    MARABOU_IN_PLAJA::add_solver_stats(stats);
    #ifdef USE_VERITAS
    VERITAS_IN_PLAJA::add_solver_stats(stats);
    #endif

}