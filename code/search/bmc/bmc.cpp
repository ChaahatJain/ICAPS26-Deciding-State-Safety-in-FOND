//
// This file is part of the PlaJA code base.
// Copyright (c) (2019 - 2021) Marcel Vinzent.
// See README.md in the top-level directory for licensing information.
//

#include "bmc.h"
#include "../../option_parser/enum_option_values_set.h"
#include "../../option_parser/plaja_options.h"
#include "../../utils/utils.h"
#include "../../stats/stats_base.h"
#include "../factories/bmc/bmc_options.h"
#include "../factories/configuration.h"
#include "../information/property_information.h"
#include "solution_checker_bmc.h"

namespace PLAJA_OPTION {

    std::unique_ptr<PLAJA_OPTION::EnumOptionValuesSet> construct_bmc_mode_enum() {
        return std::make_unique<PLAJA_OPTION::EnumOptionValuesSet>(
            std::list<PLAJA_OPTION::EnumOptionValue> {
                PLAJA_OPTION::EnumOptionValue::BmcUnsafe,
                PLAJA_OPTION::EnumOptionValue::BmcLoopFree,
                PLAJA_OPTION::EnumOptionValue::BmcUnsafeLoopFree,
                PLAJA_OPTION::EnumOptionValue::BmcLoopFreeUnsafe,
            },
            PLAJA_OPTION::EnumOptionValue::BmcUnsafe
        );
    }

}

BoundedMC::BoundedMC(const PLAJA::Configuration& config):
    SearchEngine(config)
    , bmcMode(config.get_enum_option(PLAJA_OPTION::bmc_mode).copy_value())
    , solutionChecker(new SolutionCheckerBmc(config, *model, propertyInfo->get_nn_interface()))
    , savePath(config.has_value_option(PLAJA_OPTION::savePath) ? new std::string(config.get_value_option_string(PLAJA_OPTION::savePath)) : nullptr)
    , maxSteps(config.get_int_option(PLAJA_OPTION::max_steps))
    , findShortestPathOnly(config.get_bool_option(PLAJA_OPTION::find_shortest_path_only))
    , constrainNoStart(config.is_flag_set(PLAJA_OPTION::constrain_no_start))
    , constrainLoopFree(config.is_flag_set(PLAJA_OPTION::constrain_loop_free))
    , constrainNoReach(config.is_flag_set(PLAJA_OPTION::constrain_no_reach))
    , currentStep(0)
    , shortestUnsafePath(-1)
    , longestLoopFreePath(-1) {

    /* Stats. */
    searchStatistics->add_int_stats(PLAJA::StatsInt::BMC_STEPS, "Steps", currentStep);
    searchStatistics->add_int_stats(PLAJA::StatsInt::BMC_SHORTEST_UNSAFE_PATH, "ShortestUnsafePath", shortestUnsafePath);
    searchStatistics->add_double_stats(PLAJA::StatsDouble::BMC_TIME_SHORTEST_UNSAFE_PATH, "TimeShortestUnsafePath", 0.0);
    searchStatistics->add_int_stats(PLAJA::StatsInt::BMC_LONGEST_LOOP_FREE_PATH, "LongestLoopFreePath", longestLoopFreePath);
    searchStatistics->add_double_stats(PLAJA::StatsDouble::BMC_TIME_LONGEST_LOOP_FREE_PATH, "TimeLongestLoopFreePath", 0.0);

}

BoundedMC::~BoundedMC() = default;

/* */


SearchEngine::SearchStatus BoundedMC::initialize() {
    PLAJA_LOG(PLAJA_UTILS::string_f("Step: %i", currentStep))

    if (bmcMode->get_value() != PLAJA_OPTION::EnumOptionValue::BmcLoopFree and check_start_smt()) {
        PLAJA_LOG("0-step reachability!");
        shortestUnsafePath = 0;
        searchStatistics->set_attr_int(PLAJA::StatsInt::BMC_SHORTEST_UNSAFE_PATH, shortestUnsafePath);
        searchStatistics->set_attr_double(PLAJA::StatsDouble::BMC_TIME_SHORTEST_UNSAFE_PATH, get_engine_time());
        return findShortestPathOnly ? SearchEngine::SOLVED : SearchEngine::IN_PROGRESS;
    } else { return SearchEngine::IN_PROGRESS; }

}

SearchEngine::SearchStatus BoundedMC::step() {

    // increment step counter
    ++currentStep;
    searchStatistics->inc_attr_int(PLAJA::StatsInt::BMC_STEPS);

    // termination criterion
    if (currentStep > maxSteps) { return SearchEngine::SOLVED; }

    PLAJA_LOG(PLAJA_UTILS::string_f("Current search time: %f", get_engine_time()))
    PLAJA_LOG(PLAJA_UTILS::string_f("Step: %i", currentStep))

    auto status = SearchStatus::IN_PROGRESS;

    switch (bmcMode->get_value()) {

        case PLAJA_OPTION::EnumOptionValue::BmcUnsafe: {
            status = check_unsafe();
            break;
        }

        case PLAJA_OPTION::EnumOptionValue::BmcLoopFree: {
            status = check_loop_free();
            break;
        }

        case PLAJA_OPTION::EnumOptionValue::BmcUnsafeLoopFree: {
            status = check_unsafe();
            if (status == SearchStatus::IN_PROGRESS) { status = check_loop_free(); }
            break;
        }

        case PLAJA_OPTION::EnumOptionValue::BmcLoopFreeUnsafe: {
            status = check_loop_free();
            if (status == SearchStatus::IN_PROGRESS) { status = check_unsafe(); } // if there is no loop-free path, there is no unsafe path (due to incremental checking)
            break;
        }

        default: { PLAJA_ABORT }

    }

    if (status == SearchStatus::IN_PROGRESS) { trigger_intermediate_stats(); }
    return status;
}

SearchEngine::SearchStatus BoundedMC::check_loop_free() {

    if (not check_loop_free_smt(currentStep)) {
        PLAJA_LOG("No loop-free path exists!")
        if (longestLoopFreePath < 0) {
            longestLoopFreePath = currentStep - 1;
            searchStatistics->set_attr_int(PLAJA::StatsInt::BMC_LONGEST_LOOP_FREE_PATH, longestLoopFreePath);
            searchStatistics->set_attr_double(PLAJA::StatsDouble::BMC_TIME_LONGEST_LOOP_FREE_PATH, get_engine_time());
            return findShortestPathOnly ? SearchEngine::SOLVED : SearchEngine::IN_PROGRESS;
        }
    } else { PLAJA_LOG("Loop free path exists!"); }

    return SearchStatus::IN_PROGRESS;

}

SearchEngine::SearchStatus BoundedMC::check_unsafe() {

    if (check_unsafe_smt(currentStep)) {
        PLAJA_LOG("Found unsafe path");
        if (shortestUnsafePath < 0) {
            shortestUnsafePath = currentStep;
            searchStatistics->set_attr_int(PLAJA::StatsInt::BMC_SHORTEST_UNSAFE_PATH, shortestUnsafePath);
            searchStatistics->set_attr_double(PLAJA::StatsDouble::BMC_TIME_SHORTEST_UNSAFE_PATH, get_engine_time());
            return findShortestPathOnly ? SearchEngine::SOLVED : SearchEngine::IN_PROGRESS;
        }
    }

    return SearchStatus::IN_PROGRESS;

}
