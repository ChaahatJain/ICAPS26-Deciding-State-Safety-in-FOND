//
// This file is part of the PlaJA code base.
// Copyright (c) (2023 - 2024) Chaahat Jain.
//

#include "search_statistics_ensemble_sat.h"
#include "../../../include/ct_config_const.h"
#include "../../../stats/stats_base.h"

// SearchStatisticsNNSat::SearchStatisticsNNSat(): smtStatsZ3(nullptr), smtStatsMarabou(nullptr) {
void ENSEMBLE_SAT::add_stats(PLAJA::StatsBase& stats) {

    stats.add_unsigned_stats(
        std::list<PLAJA::StatsUnsigned> {
            PLAJA::StatsUnsigned::ENSEMBLE_SAT_QUERIES_Z3, PLAJA::StatsUnsigned::ENSEMBLE_SAT_QUERIES_Z3_UNSAT, PLAJA::StatsUnsigned::ENSEMBLE_SAT_QUERIES_Z3_UNDECIDED
#ifdef USE_ADVERSARIAL_ATTACK
            ,
            PLAJA::StatsUnsigned::AD_ATTACKS, PLAJA::StatsUnsigned::AD_ATTACKS_SUC, PLAJA::StatsUnsigned::AD_ATTACKS_SOLVED_AT_START,
            PLAJA::StatsUnsigned::AD_ATTACKS_STEPS_SUC, PLAJA::StatsUnsigned::AD_ATTACKS_STEPS_NON_SUC,
            PLAJA::StatsUnsigned::AD_ATTACKS_PROJECTIONS_SUC, PLAJA::StatsUnsigned::AD_ATTACKS_PROJECTIONS_NON_SUC
#endif
        },
        std::list<std::string> {
            "EnsembleSatQueriesZ3", "EnsembleSatQueriesZ3Unsat", "EnsembleSatQueriesZ3Undecided"
#ifdef USE_ADVERSARIAL_ATTACK
            , "AdAttacks", "AdAttacksSuc", "AdAttacksSolvedAtStart", "AdAttacksStepsSuc", "AdAttacksStepsNonSuc", "AdAttacksProjectionsSuc", "AdAttacksProjectionsNonSuc"
#endif
        },
        0
    );

#ifdef USE_ADVERSARIAL_ATTACK
    stats.add_double_stats(
        std::list<PLAJA::StatsDouble> { PLAJA::StatsDouble::TIME_AD_ATTACKS, PLAJA::StatsDouble::TIME_AD_ATTACKS_SUC };
        const std::list<std::string> { "TimeAdAttacks", "TimeAdAttacksSuc" };
    );
#endif

}

#if 0
SearchStatisticsNNSat::~SearchStatisticsNNSat() = default;

//

void SearchStatisticsNNSat::print_statistics_specific() const {
    // auxiliary outputs:
#ifdef USE_ADVERSARIAL_ATTACK
    if (get_attr_unsigned(PLAJA::StatsUnsigned::AD_ATTACKS) > 0) {
        PLAJA_LOG(PLAJA_UTILS::string_f("TimePerAdAttack: %f", get_attr_double(PLAJA::StatsDouble::TIME_AD_ATTACKS) / get_attr_unsigned(PLAJA::StatsUnsigned::AD_ATTACKS)));
        PLAJA_LOG_IF(get_attr_unsigned(PLAJA::StatsUnsigned::AD_ATTACKS_PROJECTIONS_SUC) > 0, PLAJA_UTILS::string_f("TimePerAdAttackSuc: %f", get_attr_double(PLAJA::StatsDouble::TIME_AD_ATTACKS_SUC) / get_attr_unsigned(PLAJA::StatsUnsigned::AD_ATTACKS_SUC)))
        PLAJA_LOG_IF(get_attr_unsigned(PLAJA::StatsUnsigned::AD_ATTACKS) - get_attr_unsigned(PLAJA::StatsUnsigned::AD_ATTACKS_SUC) > 0, PLAJA_UTILS::string_f("TimePerAdAttackNonSuc: %f", ( get_attr_double(PLAJA::StatsDouble::TIME_AD_ATTACKS) - get_attr_double(PLAJA::StatsDouble::TIME_AD_ATTACKS_SUC)) / ( get_attr_unsigned(PLAJA::StatsUnsigned::AD_ATTACKS) - get_attr_unsigned(PLAJA::StatsUnsigned::AD_ATTACKS_SUC) )))
        PLAJA_LOG(PLAJA_UTILS::string_f("AdAttackSucRate: %f", static_cast<double>(get_attr_unsigned(PLAJA::StatsUnsigned::AD_ATTACKS_SUC)) / get_attr_unsigned(PLAJA::StatsUnsigned::AD_ATTACKS)))
    }
#endif
}
#endif
