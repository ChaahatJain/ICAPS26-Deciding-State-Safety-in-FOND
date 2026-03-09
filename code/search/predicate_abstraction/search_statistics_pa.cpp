//
// This file is part of the PlaJA code base.
// Copyright (c) (2019 - 2020) Marcel Vinzent.
// See README.md in the top-level directory for licensing information.
//

#include "search_statistics_pa.h"
#include "../../stats/stats_base.h"
#include "../fd_adaptions/search_statistics.h"
#include "../smt/solver/statistics_smt_solver_z3.h"
#include "../smt_nn/solver/statistics_smt_solver_marabou.h"
#include "heuristic/statistics_heuristic.h"
#include "nn/search_statistics_nn_sat.h"
#include "ensemble/search_statistics_ensemble_sat.h"
#include "../smt_ensemble/solver/statistics_smt_solver_veritas.h"

// SearchStatisticsPA::SearchStatisticsPA(): SearchStatisticsBasic() {
void SEARCH_PA_BASE::add_stats(PLAJA::StatsBase& stats) {

    SEARCH_STATISTICS::add_basic_stats(stats);

    stats.add_int_stats(PLAJA::StatsInt::PATH_LENGTH, "PathLength", -1);

    stats.add_unsigned_stats(
        std::list<PLAJA::StatsUnsigned> {
            PLAJA::StatsUnsigned::PREDICATES,
#ifdef ENABLE_TERMINAL_STATE_SUPPORT
            PLAJA::StatsUnsigned::TERMINAL_TESTS_SAT,
#endif
            PLAJA::StatsUnsigned::APP_TESTS, PLAJA::StatsUnsigned::APP_TESTS_UNSAT, PLAJA::StatsUnsigned::TRANSITION_TESTS, PLAJA::StatsUnsigned::TRANSITION_TESTS_UNSAT,
            PLAJA::StatsUnsigned::NN_SELECTION_TESTS, PLAJA::StatsUnsigned::NN_SELECTION_TESTS_UNSAT,
            PLAJA::StatsUnsigned::NN_APP_TESTS, PLAJA::StatsUnsigned::NN_APP_TESTS_UNSAT, PLAJA::StatsUnsigned::NN_TRANSITION_TESTS, PLAJA::StatsUnsigned::NN_TRANSITION_TESTS_UNSAT,
            PLAJA::StatsUnsigned::NN_STALLING_TESTS, PLAJA::StatsUnsigned::NN_STALLING_TESTS_UNSAT,
            PLAJA::StatsUnsigned::REUSED_SOLUTIONS, PLAJA::StatsUnsigned::REUSED_TRANSITION_EXISTENCE, PLAJA::StatsUnsigned::REUSED_TRANSITION_EXCLUSION,
            PLAJA::StatsUnsigned::TRANSITION_WITHOUT_WITNESS

        },
        std::list<std::string> {
            "Predicates",
#ifdef ENABLE_TERMINAL_STATE_SUPPORT
            "TerminalTestsSat",
#endif
            "AppTests", "AppTestsUnsat", "TransitionTests", "TransitionTestsUnsat",
            "NNSelectionTests", "NNSelectionTestsUnsat", "NNAppTests", "NNAppTestsUnsat", "NNTransitionTests", "NNTransitionTestsUnsat", "NNStallingTests", "NNStallingTestsUnsat",
            "ReusedSolutions", "ReusedTransitionExistence", "ReusedTransitionExclusion", "TransitionWithoutWitness"
        },
        0
    );

    stats.add_double_stats(
        std::list<PLAJA::StatsDouble> {
            PLAJA::StatsDouble::PA_SS_REFINE_TIME, PLAJA::StatsDouble::PA_SUC_GEN_EXPLORE_TIME,
            PLAJA::StatsDouble::PA_ACTION_LABEL_IT_TIME, PLAJA::StatsDouble::PA_ACTION_OP_IT_TIME,
            PLAJA::StatsDouble::PA_UPDATE_IT_TIME, PLAJA::StatsDouble::PA_UPD_ENTAILMENT_TIME, PLAJA::StatsDouble::PA_SUCCESSOR_ENUM_TIME,
            PLAJA::StatsDouble::TIME_APP_TEST, PLAJA::StatsDouble::TIME_TRANSITION_TEST,
            PLAJA::StatsDouble::TIME_NN_SELECTION_TEST, PLAJA::StatsDouble::TIME_NN_APP_TEST, PLAJA::StatsDouble::TIME_NN_TRANSITION_TEST,
            PLAJA::StatsDouble::PA_PUSH_POP_SRC_TIME, PLAJA::StatsDouble::PA_PUSH_POP_ACTION_LABEL_TIME, PLAJA::StatsDouble::PA_PUSH_POP_ACTION_OP_TIME, PLAJA::StatsDouble::PA_PUSH_POP_UPD_TIME
        },
        std::list<std::string> {
            "PAStateSpaceRefineTime", "PASucGenExplorationTime",
            "PAActionLabelItTime", "PAActionOpItTime",
            "PAUpdateItTime", "PAUpdateEntailmentTime", "PASuccessorEnumerationTime",
            "TimeAppTest", "TimeTransitionTest",
            "TimeNNSelTest", "TimeNNAppTest", "TimeNNTransitionTest",
            "PAPushPopSrcTime", "PAPushPopActionLabelTime", "PAPushPopActionOpTime", "PAPushPopUpdTime"
        },
        0.0
    );

    ABSTRACT_HEURISTIC::add_stats(stats);
    NN_SAT::add_stats(stats);
    #ifdef USE_VERITAS
    ENSEMBLE_SAT::add_stats(stats);
    #endif
    Z3_IN_PLAJA::add_solver_stats(stats);
    MARABOU_IN_PLAJA::add_solver_stats(stats);
    #ifdef USE_VERITAS
    VERITAS_IN_PLAJA::add_solver_stats(stats);
    #endif
}

#if 0

SearchStatisticsPA::~SearchStatisticsPA() = default;

void SearchStatisticsPA::reset_specific() {
    SearchStatisticsBasic::reset_specific();
    statsHeuristic->reset();
    statsSMTZ3->reset();
    statsSMTMarabou->reset();
    statsNNSat->reset();
}

void SearchStatisticsPA::print_statistics_specific() const {
    SearchStatisticsBasic::print_statistics_specific();
    statsHeuristic->print_statistics();
    statsSMTZ3->print_statistics();
    statsSMTMarabou->print_statistics();
    statsNNSat->print_statistics();
}

void SearchStatisticsPA::stats_to_csv_specific(std::ofstream& file) const {
    SearchStatisticsBasic::stats_to_csv_specific(file);
    statsHeuristic->stats_to_csv(file);
    statsSMTZ3->stats_to_csv(file);
    statsSMTMarabou->stats_to_csv(file);
    statsNNSat->stats_to_csv(file);
}

void SearchStatisticsPA::stat_names_to_csv_specific(std::ofstream& file) const {
    SearchStatisticsBasic::stat_names_to_csv_specific(file);
    statsHeuristic->stat_names_to_csv(file);
    statsSMTZ3->stat_names_to_csv(file);
    statsSMTMarabou->stat_names_to_csv(file);
    statsNNSat->stat_names_to_csv(file);
}

#endif
