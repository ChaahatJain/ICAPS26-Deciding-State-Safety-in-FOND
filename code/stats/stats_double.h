//
// This file is part of the PlaJA code base.
// Copyright (c) (2019 - 2023) Marcel Vinzent.
// See README.md in the top-level directory for licensing information.
//

#ifndef PLAJA_STATS_DOUBLE_H
#define PLAJA_STATS_DOUBLE_H

#include "../include/ct_config_const.h"

namespace PLAJA {

    enum class StatsDouble {

        // SearchStatistics
        PREPROCESSING_TIME,  // parsing + checks
        CONSTRUCTION_TIME,   // time to construct engine
        INITIALIZATION_TIME, // initialization time of the engine
        SEARCH_TIME,         // (primary) computational time spent by the engine
        FINALIZATION_TIME,   // time spent by the engine "finalizing", e.g., analyzing main results

    // LearningStatistics
#if defined(BUILD_POLICY_LEARNING) || defined(BUILD_FAULT_ANALYSIS)
        BEST_SCORE,
        AVERAGE_LENGTH_BEST_SCORE,
        LAST_SCORE,
        LAST_AVERAGE_LENGTH,
        EPSILON,
        TimeForTeacher,

        /* policy-run sampling stats */
        AVG_SAMPLED_POLICY_LENGTH, // Average length of sampled policy.
        DISCRIMINATION_RATE,       // Discrimination rate of the policy.

        /* Retraining stats */
        TRAIN_FAIL_FRACTION,
        TRAIN_GOAL_FRACTION,
        EVAL_FAIL_FRACTION,
        EVAL_GOAL_FRACTION,
        FAULT_ANALYSIS_TIME,
#endif

#ifdef BUILD_PA
        // SearchStatisticsCegar
        INCREMENTATION_TIME,
        TIME_FOR_PA,       // time spent in sub-engines computing PA
        AvDistanceWsState, // Average of normalized distance between witness and concretization value on all variables.
        AvDistanceWsStateCandidates, // Average of normalized distance between witness and concretization value on distinct/candidate variables.
        // AvDistanceWs,                // Average of normalized size of witness split.
        AvPertBound,  // Average perturbation for variable selection
        RelevantVars, // Average fraction of relevant/irrelevant variable for variable selection.
        IrrelevantVars,

        // SearchStatisticsPA
        PA_SS_REFINE_TIME,
        PA_SUC_GEN_EXPLORE_TIME,
        PA_ACTION_LABEL_IT_TIME,
        PA_ACTION_OP_IT_TIME,
        PA_UPDATE_IT_TIME,
        PA_UPD_ENTAILMENT_TIME,
        PA_SUCCESSOR_ENUM_TIME,
        //
        TIME_APP_TEST,
        TIME_TRANSITION_TEST,
        TIME_NN_SELECTION_TEST,
        TIME_NN_APP_TEST,
        TIME_NN_TRANSITION_TEST,
        //
        PA_PUSH_POP_SRC_TIME,
        PA_PUSH_POP_ACTION_LABEL_TIME,
        PA_PUSH_POP_ACTION_OP_TIME,
        PA_PUSH_POP_UPD_TIME,

        // StatisticsHeuristic (PA)
        HEURISTIC_COMPUTATION_TIME,

        // MATCH TREE
        MT_AV_DEPTH,

#endif

#ifdef USE_Z3
        // StatisticsSMTSolver (Z3)
        TIME_Z3,
        TIME_Z3_UNSAT,
        TIME_Z3_PUSH,
        TIME_Z3_POP,
        TIME_Z3_RESET,
#endif

#ifdef USE_MARABOU
        // StatisticsSMTSolverMarabou
        TIME_MARABOU_PREPROCESS,
        TIME_MARABOU_PREPROCESS_MAX,
        TIME_MARABOU_FOR_PREPROCESSED_UNSAT,
        TIME_MARABOU_PREPROCESS_UNSAT,
        TIME_MARABOU,
        TIME_MARABOU_MAX,
        TIME_MARABOU_UNSAT,
        TIME_MARABOU_BB,
        TIME_MARABOU_BB_UNSAT,
        TIME_MARABOU_PUSH,
        TIME_MARABOU_POP,
        //
        MARABOU_BB_NON_EXACT_INT_DELTA,
#endif

#ifdef USE_VERITAS
        // StatisticsSMTSolverMarabou
        TIME_VERITAS_PREPROCESS,
        TIME_VERITAS_PREPROCESS_MAX,
        TIME_VERITAS_FOR_PREPROCESSED_UNSAT,
        TIME_VERITAS_PREPROCESS_UNSAT,
        TIME_VERITAS,
        TIME_VERITAS_MAX,
        TIME_VERITAS_UNSAT,
        TIME_VERITAS_BB,
        TIME_VERITAS_BB_UNSAT,
        TIME_VERITAS_PUSH,
        TIME_VERITAS_POP,
        TIME_VERITAS_SAT_MAX,
        TIME_VERITAS_UNSAT_MAX,
        //
        VERITAS_BB_NON_EXACT_INT_DELTA,
#endif
#ifdef BUILD_FAULT_ANALYSIS
        YES_COVERAGE_TOTAL_TIME,
        NO_COVERAGE_TOTAL_TIME,
        TIME_FOR_QUERY,
#endif

#ifdef USE_ADVERSARIAL_ATTACK
        // StatisticsAdversarialAttacks
        TIME_AD_ATTACKS,
        TIME_AD_ATTACKS_SUC,
#endif

        // BMC
        BMC_TIME_SHORTEST_UNSAFE_PATH,
        BMC_TIME_LONGEST_LOOP_FREE_PATH,
#ifdef BUILD_SAFE_START_GENERATOR
        TOTAL_REFINING_TIME,
        TOTAL_TESTING_TIME,      // time taken for testing in seconds.
        TOTAL_VERIFICATION_TIME, // time taken for verification.
        // per iteration stats:
        REFINING_TIME,
        SEARCHING_TIME, // duration of unsafe state identification
        UNSAFETY_EVAL, // time taken to evaluate a state against the unsafety condition.
        BOX_SIZE, // size of box relative to statespace.


#endif

    };

} // namespace PLAJA

#endif //PLAJA_STATS_DOUBLE_H
