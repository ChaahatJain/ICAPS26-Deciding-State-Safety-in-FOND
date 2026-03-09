//
// This file is part of the PlaJA code base.
// Copyright (c) (2019 - 2022) Marcel Vinzent.
// See README.md in the top-level directory for licensing information.
//

#include <string>
#include "search_statistics_fault.h"
#include "../../../stats/stats_base.h"
#include "../../fd_adaptions/search_statistics.h"

void FAULT_ANALYSIS::add_stats(PLAJA::StatsBase& stats) {
    SEARCH_STATISTICS::add_basic_stats(stats);

    stats.add_unsigned_stats(
        std::list<PLAJA::StatsUnsigned> {
            PLAJA::StatsUnsigned::EPISODES, PLAJA::StatsUnsigned::DONE_GOAL, PLAJA::StatsUnsigned::DONE_AVOID, PLAJA::StatsUnsigned::PATHS_CHECKED, PLAJA::StatsUnsigned::BUGS_FOUND,
            PLAJA::StatsUnsigned::TERMINAL_STATES, PLAJA::StatsUnsigned::DETECTED_CYCLES, 
            PLAJA::StatsUnsigned::FAULT_DETECTED, PLAJA::StatsUnsigned::NUM_FAULTS, PLAJA::StatsUnsigned::NUM_BACKTRACKS, PLAJA::StatsUnsigned::NO_FAULT_PATH,
            PLAJA::StatsUnsigned::LAST_STEP_FAULT, PLAJA::StatsUnsigned::UNDONE_ORACLE, 
            PLAJA::StatsUnsigned::CYCLE_ORACLE, PLAJA::StatsUnsigned::CACHE_ORACLE,
            PLAJA::StatsUnsigned::COVERAGE, PLAJA::StatsUnsigned::NUM_ORACLE_QUERIES,
            PLAJA::StatsUnsigned::NUM_YES, PLAJA::StatsUnsigned::NUM_NO, PLAJA::StatsUnsigned::NUM_TIMEOUT,
            PLAJA::StatsUnsigned::STATE_SPACE_COMPUTATION_TIMEOUT, PLAJA::StatsUnsigned::DUPLICATE_USS
        },
        std::list<std::string> {
            "Episodes", "DoneGoal", "DoneAvoid", "PathsChecked", "BugsFound",
            "TerminalStates", "DetectedCycles",
            "FaultDetected", "NumFaults", "NumBacktracks", "NoFaultPath",
            "LastStepFault", "UndoneOracle",
            "CycleOracle", "CacheOracle",
            "Coverage", "NumOracleQueries",
            "Yes", "No", "Timeout",
            "StateSpaceComputationTimeout", "DuplicateUSS",
        },
        0
    );

    stats.add_double_stats(
        std::list<PLAJA::StatsDouble> {
            PLAJA::StatsDouble::YES_COVERAGE_TOTAL_TIME, PLAJA::StatsDouble::NO_COVERAGE_TOTAL_TIME, PLAJA::StatsDouble::TIME_FOR_QUERY, PLAJA::StatsDouble::TRAIN_FAIL_FRACTION, PLAJA::StatsDouble::TRAIN_GOAL_FRACTION, PLAJA::StatsDouble::EVAL_FAIL_FRACTION, PLAJA::StatsDouble::EVAL_GOAL_FRACTION, PLAJA::StatsDouble::FAULT_ANALYSIS_TIME,
        },
        std::list<std::string> {
            "YesCoverageTotalTime", "NoCoverageTotalTime", "TimeForQuery", "TrainFailFraction", "TrainGoalFraction", "EvalFailFraction", "EvalGoalFraction", "FaultAnalysisTime"
        },
        0.0
        );

    stats.add_string_stats(
        std::list<PLAJA::StatsString> {
            PLAJA::StatsString::START_STATE_CHECKED,
            PLAJA::StatsString::FAULT_SOLVING_TIMES,
            PLAJA::StatsString::STATES_FOR_ORACLE
        },
        std::list<std::string> {
            "StateChecked",
            "FaultSolvingTimes",
            "StatesForOracle"
        },
        ""
        );
    
    stats.add_int_stats(
        std::list<PLAJA::StatsInt> {
            PLAJA::StatsInt::NUM_FAULTS_FOUND
        },
        std::list<std::string>{
            "NumFaultsFound"
        },
        0
    );
}