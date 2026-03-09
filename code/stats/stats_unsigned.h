//
// This file is part of the PlaJA code base.
// Copyright (c) (2019 - 2023) Marcel Vinzent.
// See README.md in the top-level directory for licensing information.
//

#ifndef PLAJA_STATS_UNSIGNED_H
#define PLAJA_STATS_UNSIGNED_H

#include "../include/ct_config_const.h"

namespace PLAJA {

    enum class StatsUnsigned {

        // SearchStatisticsBasic
        EXPANDED_STATES,
        GENERATED_STATES,
        ACTION_LABELS, // nr of (applicable) action (labels) accumulated over states
        ACTION_OPS,    // nr of (applicable) action operators accumulated over states
        TRANSITIONS,
        TERMINAL_STATES, // nr states without outgoing transition
        POLICY_TERMINAL_STATES,
        START_STATES,
        REACHABLE_STATES,
        GOAL_STATES,
        GoalPathStates,
        DEAD_END_STATES,

        /* Non-prob search. */
        InevitableGoalPathStates,
        InevitableGoalPathStatesStart,
        PolicyGoalPathStates,
        PolicyGoalPathStatesStart,
        EvitablePolicyGoalPathStates,

        /* Biased Sampling */
        UNSAFE_START_STATES, // # of unique start states leading to unsafety.
        RESAMPLES,           // # of states that were previously sampled.
        TIE_BREAKS,          // # of tie breaks occurred when successor states are equally good.

#ifdef BUILD_PROB_SEARCH
        // ProbSearchStatistics
        DONT_CARE_STATES, // states for which we do not care about the policy choice, for more information see e.g. ValueInitializer
        FIXED_CHOICES, // states for which the choice is fixed
#endif

#ifdef BUILD_POLICY_LEARNING
        // LearningStatistics
        EPISODES,
        POLICY_ACTS, // Excluding deterministic state transitions.
        SUB_RANK_ACTS, // Steps where (under applicability-filtering) an action that is not ranked highest has been chosen.
        EPSILON_GREEDY_ACTS,
        TeacherActs,
        TeacherActsUnsuccessful,
        DONE_GOAL,
        DONE_AVOID,
        DONE_STALLING,
        UNDONE,
        DETECTED_CYCLES,
#endif

#ifdef BUILD_PA
        // SearchStatisticsCegar
        ITERATIONS,
        REF_DUE_TO_CONCRETIZATION, // concrete path state is not a concretization of abstract path state
#ifdef ENABLE_TERMINAL_STATE_SUPPORT
        REF_DUE_TO_TERMINAL, // non-terminal does not hold
#endif
        REF_DUE_TO_GUARD_APP,          // guard not applicable
        REF_DUE_TO_ACTION_SEL,         // action not selected
        REF_DUE_TO_TARGET,             // final path state is not a reachability target
        GLOBAL_PREDICATES_ADDED,       // may exceed total number of preds for lazy pa
        LOCAL_PREDICATES_ADDED,        // may exceed total number of preds for lazy pa
        PREDICATES_FOR_ACTION_SEL_REF, // predicates per refinement

        WsFallBack,     // Perform witness-agnostic refinement due to missing witness.
        WsCandidates,   // Accumulated number of candidate variables for witness splitting.
        WsVars,         // Accumulated number of used split vars.
        EntailedSplits, // Accumulated number of variables split at path-prefix entailed value.
        // RefWithEntailment,              // Accumulated number of refinement using entailment.

        // SearchStatisticsPA
        PREDICATES,

    // SearchStatisticsPATest
#ifdef ENABLE_TERMINAL_STATE_SUPPORT
        TERMINAL_TESTS_SAT,
#endif
        APP_TESTS,
        APP_TESTS_UNSAT,
        TRANSITION_TESTS,
        TRANSITION_TESTS_UNSAT,
        NN_SELECTION_TESTS,
        NN_SELECTION_TESTS_UNSAT,
        NN_APP_TESTS,
        NN_APP_TESTS_UNSAT,
        NN_TRANSITION_TESTS,
        NN_TRANSITION_TESTS_UNSAT,
        NN_STALLING_TESTS,
        NN_STALLING_TESTS_UNSAT,
        REUSED_SOLUTIONS,            // solutions successfully reused from previous tests
        REUSED_TRANSITION_EXISTENCE, // solutions successfully reused from iterations (for cegar)
        REUSED_TRANSITION_EXCLUSION,
        TRANSITION_WITHOUT_WITNESS,

        // StatisticsHeuristic (PA)
        HEURISTIC_CALLS,
        HEURISTIC_CACHE_HITS, // reusing heuristic estimates already computed
        UPDATED_ESTIMATES,    // heuristic estimates may be updated/improved

        // MATCH TREE
        MT_SIZE,
        MT_MAX_DEPTH,

#endif

#ifdef USE_Z3
        // StatisticsSMTSolver (Z3)
        Z3_QUERIES,
        Z3_QUERIES_UNSAT,
        Z3_QUERIES_UNDECIDED, // SMT queries, that could not be decided by the underlying SMT-solver
#endif

#ifdef USE_MARABOU
        // StatisticsSMTSolverMarabou
        MARABOU_PREPROCESSED_UNSAT, // Queries inferred to be unsat during pre-processing (not counted as marabou queries).
        MARABOU_QUERIES,
        MARABOU_QUERIES_UNSAT,
        MARABOU_QUERIES_UNDECIDED, // Queries that could not be decided.
        MARABOU_INVALID_SOLUTIONS, // Queries sat but with an invalid-solution (in release mode we only check OOB). This is possible due to numeric issues.
        MARABOU_NUM_SPLITS,          // Marabou internal
        MARABOU_NUM_POPS,            // Marabou internal
        MARABOU_MAX_SPLIT_DEPTH,     // Marabou internal
        MARABOU_LEARNED_CONFLICTS,   // PlaJA-Marabou internal
        MARABOU_CASE_SPLITS_SKIPPED, // PlaJA-Marabou internal
        MARABOU_CASE_SPLITS_FIXED,   // PlaJA-Marabou internal
        MARABOU_BB,
        MARABOU_BB_UNSAT,
        MARABOU_BB_EXACT_WITH_ARGMAX_TOLERANCE, // Marabou BB concluded with exact solution up to argmax tolerance.
        MARABOU_BB_NON_EXACT,     // Marabou in BB concluded sat with non-exact solution (due to floating point).
        MARABOU_BB_NO_SOLUTION,   // Marabou in BB concluded sat without solution.
        MARABOU_BB_NON_EXACT_INT, // Marabou queries with non-exact integer solution (during BB).
        MARABOU_BB_BRANCH_OOB,    // Branching over variable with out-of-bound value in current solution (during BB)
        MARABOU_BB_BRANCH_EXACT_INT, // Branching over variable with exact integer value (floor == ceil) in current solution (during BB)
        MARABOU_BB_RECURSIONS,
        MARABOU_BB_MAX_DEPTH,
#endif

        // SearchStatisticsNNSat
        NN_SAT_QUERIES_Z3,
        NN_SAT_QUERIES_Z3_UNSAT,
        NN_SAT_QUERIES_Z3_UNDECIDED,

#ifdef USE_VERITAS
        // StatisticsSMTSolverMarabou
        VERITAS_PREPROCESSED_UNSAT, // Queries inferred to be unsat during pre-processing (not counted as marabou queries).
        VERITAS_QUERIES,
        VERITAS_QUERIES_UNSAT,
        VERITAS_QUERIES_UNDECIDED, // Queries that could not be decided.
        VERITAS_INVALID_SOLUTIONS, // Queries sat but with an invalid-solution (in release mode we only check OOB). This is possible due to numeric issues.
        VERITAS_NUM_SPLITS,          // Marabou internal
        VERITAS_NUM_POPS,            // Marabou internal
        VERITAS_MAX_SPLIT_DEPTH,     // Marabou internal
        VERITAS_LEARNED_CONFLICTS,   // PlaJA-Marabou internal
        VERITAS_CASE_SPLITS_SKIPPED, // PlaJA-Marabou internal
        VERITAS_CASE_SPLITS_FIXED,   // PlaJA-Marabou internal
        VERITAS_BB,
        VERITAS_BB_UNSAT,
        VERITAS_BB_EXACT_WITH_ARGMAX_TOLERANCE, // Marabou BB concluded with exact solution up to argmax tolerance.
        VERITAS_BB_NON_EXACT,     // Marabou in BB concluded sat with non-exact solution (due to floating point).
        VERITAS_BB_NO_SOLUTION,   // Marabou in BB concluded sat without solution.
        VERITAS_BB_NON_EXACT_INT, // Marabou queries with non-exact integer solution (during BB).
        VERITAS_BB_BRANCH_OOB,    // Branching over variable with out-of-bound value in current solution (during BB)
        VERITAS_BB_BRANCH_EXACT_INT, // Branching over variable with exact integer value (floor == ceil) in current solution (during BB)
        VERITAS_BB_RECURSIONS,
        VERITAS_BB_MAX_DEPTH,
#endif

        // SearchStatisticsEnsembleSat
        ENSEMBLE_SAT_QUERIES_Z3,
        ENSEMBLE_SAT_QUERIES_Z3_UNSAT,
        ENSEMBLE_SAT_QUERIES_Z3_UNDECIDED,

#ifdef USE_ADVERSARIAL_ATTACK
        // StatisticsAdversarialAttacks
        AD_ATTACKS,
        AD_ATTACKS_SUC,
        AD_ATTACKS_SOLVED_AT_START,
        AD_ATTACKS_STEPS_SUC,
        AD_ATTACKS_STEPS_NON_SUC,
        AD_ATTACKS_PROJECTIONS_SUC,
        AD_ATTACKS_PROJECTIONS_NON_SUC,
#endif

#ifdef BUILD_FAULT_ANALYSIS
        FAULT_DETECTED,
        NUM_FAULTS,
        NUM_BACKTRACKS,
        NO_FAULT_PATH,
        LAST_STEP_FAULT,
        UNDONE_ORACLE,
        DUPLICATE_USS,
        CYCLE_ORACLE,
        CACHE_ORACLE,
        NUM_ORACLE_QUERIES,
        COVERAGE,
        NUM_YES,
        NUM_NO,
        NUM_TIMEOUT,
        STATE_SPACE_COMPUTATION_TIMEOUT,
        BUGS_FOUND,
        PATHS_CHECKED,
#endif
#ifdef BUILD_SAFE_START_GENERATOR
        // testing:
        TESTING_FAILED, // # of times testing produced no unsafe paths.
        UNSAFE_PATHS,  // # of unsafe paths encountered during testing.
        UNSAFE_STATES, // # of unsafe states encountered during testing.
        CYCLES,        // # of cycles encountered during testing.
        DEAD_ENDS,      // # of dead-end states encountered during testing.
        // verification:
        UNSAFE_STATES_VERIFIED, // # of unsafe states found by verification in seconds.
        // per iteration stats
        // UNSAFE_STATES_TESTING, // # of unsafe states found by testing.
        // UNSAFE_STATES_VERIFICATION, // # of unsafe states found by verification.
        TIME_LIMIT_REACHED, //
#endif

    };

} // namespace PLAJA

#endif //PLAJA_STATS_UNSIGNED_H
