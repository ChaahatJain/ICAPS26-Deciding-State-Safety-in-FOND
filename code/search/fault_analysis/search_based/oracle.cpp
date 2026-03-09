//
// This file is part of the PlaJA code base.
// Copyright (c) (2023 - 2024) Chaahat Jain.
//

// Intuition:
// Given transition s -a-> s' on unsafe policy path, try alternate actions. Check if they are safe.
// If any alternate action leads to safety, then we have detected a fault.
#include "oracle.h"
#include "../../../globals.h"
#include "../../../option_parser/option_parser.h"
#include "../../../option_parser/option_parser_aux.h"
#include "../../../parser/ast/expression/non_standard/objective_expression.h"
#include "../../factories/configuration.h"
#include "../../factories/fault_analysis/fault_analysis_options.h"
#include "../../fd_adaptions/state.h"
#include "../../fd_adaptions/timer.h"
#include "../../non_prob_search/policy/policy_restriction.h"
#include "../../successor_generation/simulation_environment.h"
#include "../../using_search.h"
#include "../../../utils/rng.h"
#include "test_oracle/dsmc_oracle.h"
#include "test_oracle/finite_oracle.h"
#include "test_oracle/infinite_oracle.h"
#include <algorithm>
#include <vector>
#include <cmath>
#include <numeric>
#include <stack>
#include <fstream>
#include <json.hpp>
using json = nlohmann::json;

namespace PLAJA_OPTION_DEFAULT {
    const std::string oracle_mode("infinite");                        // NOLINT(cert-err58-cpp)
    constexpr int max_depth(1000);                                     // NOLINT(cert-err58-cpp)
    const std::string search_dir("forward");                          // NOLINT(cert-err58-cpp)
    constexpr int num_faults_per_path(std::numeric_limits<int>::max());// NOLINT(cert-err58-cpp)
    const std::string cost_objective("MaxCost");                       // NOLINT(cert-err58-cpp)
    constexpr double seconds_per_query(std::numeric_limits<double>::max()); // NOLINT(cert-err58-cpp)
    const std::string fault_experiment_mode("individualSearch"); // NOLINT(cert-err58-cpp)

}// namespace PLAJA_OPTION_DEFAULT

namespace PLAJA_OPTION {
    const std::string oracle_mode("oracle-mode");                // NOLINT(cert-err58-cpp)
    const std::string max_depth("max-depth");                    // NOLINT(cert-err58-cpp)
    const std::string search_dir("search-dir");                  // NOLINT(cert-err58-cpp)
    const std::string num_faults_per_path("num-faults-per-path");// NOLINT(cert-err58-cpp)
    const std::string cost_objective("cost-objective");          // NOLINT(cert-err58-cpp)
    const std::string cache_fault_search("cache-fault-search"); // NOLINT(cert-err58-cpp)
    const std::string retain_fault_cache("retain-fault-cache"); // NOLINT(cert-err58-cpp)
    const std::string seconds_per_query("seconds-per-query"); // NOLINT(cert-err58-cpp)
    const std::string fault_experiment_mode("fault-experiment-mode"); // NOLINT(cert-err58-cpp)

    namespace FA_ORACLE {
        std::string print_supported_modes() { return PLAJA::OptionParser::print_supported_options(stringToOracle); }
        std::string print_supported_search() { return PLAJA::OptionParser::print_supported_options(stringToSearch); }
        std::string print_supported_costs() { return PLAJA::OptionParser::print_supported_options(stringToObjective); }
        std::string print_supported_experiments() { return PLAJA::OptionParser::print_supported_options(stringToExperiment); }

        void add_options(PLAJA::OptionParser& option_parser) {
            OPTION_PARSER::add_option(option_parser, oracle_mode, PLAJA_OPTION_DEFAULT::oracle_mode);
            OPTION_PARSER::add_option(option_parser, cost_objective, PLAJA_OPTION_DEFAULT::cost_objective);
            OPTION_PARSER::add_int_option(option_parser, max_depth, PLAJA_OPTION_DEFAULT::max_depth);
            OPTION_PARSER::add_option(option_parser, search_dir, PLAJA_OPTION_DEFAULT::search_dir);
            OPTION_PARSER::add_int_option(option_parser, num_faults_per_path, PLAJA_OPTION_DEFAULT::num_faults_per_path);
            OPTION_PARSER::add_flag(option_parser, PLAJA_OPTION::cache_fault_search);
            OPTION_PARSER::add_flag(option_parser, PLAJA_OPTION::retain_fault_cache);
            OPTION_PARSER::add_double_option(option_parser, seconds_per_query, PLAJA_OPTION_DEFAULT::seconds_per_query);
            OPTION_PARSER::add_option(option_parser, fault_experiment_mode, PLAJA_OPTION_DEFAULT::fault_experiment_mode);
            DSMC_ORACLE::add_options(option_parser);
            FA_FINITE_ORACLE::add_options(option_parser);
            FA_INFINITE_ORACLE::add_options(option_parser);
        }

        extern void print_options() {
            OPTION_PARSER::print_option(PLAJA_OPTION::oracle_mode, OPTION_PARSER::valueStr, PLAJA_OPTION_DEFAULT::oracle_mode, "Specify the oracle to be used.", true);
            OPTION_PARSER::print_additional_specification(print_supported_modes(), true);
            OPTION_PARSER::print_int_option(PLAJA_OPTION::max_depth, PLAJA_OPTION_DEFAULT::max_depth, "Track exhaustive/enumerate search traces that are deeper than this.");
            OPTION_PARSER::print_option(PLAJA_OPTION::search_dir, OPTION_PARSER::valueStr, PLAJA_OPTION_DEFAULT::search_dir, "Specify the direction on policy path to perform fault localization", true);
            OPTION_PARSER::print_additional_specification(print_supported_search(), true);
            OPTION_PARSER::print_option(PLAJA_OPTION::cost_objective, PLAJA_OPTION_DEFAULT::cost_objective, "Specify the cost objective", true);
            OPTION_PARSER::print_additional_specification(PLAJA_OPTION::FA_ORACLE::print_supported_costs(), true);
            OPTION_PARSER::print_int_option(PLAJA_OPTION::num_faults_per_path, PLAJA_OPTION_DEFAULT::num_faults_per_path, "Number of faults to be found per path");
            OPTION_PARSER::print_double_option(seconds_per_query, PLAJA_OPTION_DEFAULT::seconds_per_query, "Number of seconds per query");
            OPTION_PARSER::print_option(PLAJA_OPTION::fault_experiment_mode, OPTION_PARSER::valueStr, PLAJA_OPTION_DEFAULT::fault_experiment_mode, "Specify the experiment mode", true);
            OPTION_PARSER::print_additional_specification(print_supported_experiments(), true);
            OPTION_PARSER::print_flag(PLAJA_OPTION::cache_fault_search, "Create a cache for states when searching for faults.");
            OPTION_PARSER::print_flag(PLAJA_OPTION::retain_fault_cache, "When not set, delete the cache when moving to a new unsafe path. When set to true, the cache will only be deleted when timeout instances are observed.");
            DSMC_ORACLE::print_options();
            FA_FINITE_ORACLE::print_options();
            FA_INFINITE_ORACLE::print_options();
        }
    }// namespace FA_ORACLE
}// namespace PLAJA_OPTION


Oracle::Oracle(const PLAJA::Configuration& config) :
search_direction_(config.get_option(PLAJA_OPTION::FA_ORACLE::stringToSearch, PLAJA_OPTION::search_dir)),
maxFaults(config.get_int_option(PLAJA_OPTION::num_faults_per_path)),
cost_objective_(config.get_option(PLAJA_OPTION::FA_ORACLE::stringToObjective, PLAJA_OPTION::cost_objective)),
use_cache(config.is_flag_set(PLAJA_OPTION::cache_fault_search)),
cacheAcrossPaths(config.is_flag_set(PLAJA_OPTION::retain_fault_cache)),
seconds_per_query(config.get_double_option(PLAJA_OPTION::seconds_per_query)),
experiment_mode(config.get_option(PLAJA_OPTION::FA_ORACLE::stringToExperiment, PLAJA_OPTION::fault_experiment_mode)) {}

/****************************** Termination Criteria **************************************************/
bool Oracle::check_avoid(const State& state) const {
    /* Fail state reached? */
    PLAJA_ASSERT(avoid)
    // std::cout << "This state is evaluated to: " << avoid->evaluateInteger(&state) << " and the boolean value is: " << static_cast<bool>(avoid->evaluateInteger(&state)) << std::endl;
    return avoid->evaluateInteger(&state);
}

bool Oracle::check_goal(const State& state) const {
    /* Goal state reached? */
    auto* goal = objective->get_goal();
    return goal and goal->evaluateInteger(&state);
}

bool Oracle::check_terminal(const State& state) const {
    /* Terminal state reached? */
    auto applicable_actions = simulatorEnv->extract_applicable_actions(state, true);
    return applicable_actions.empty();
};

bool Oracle::check_cycle(const StateID_type state, std::vector<StateID_type> explored_path) {
    /* Cycle reached? */
    return std::find(explored_path.begin(), explored_path.end(), state) != explored_path.end();
}

Oracle::Status Oracle::get_status(const State& state) const {
    if (check_goal(state)) {
        return Status::GOAL;
    }
    if (check_terminal(state)) {
        return Status::TERMINAL;
    }
    if (check_avoid(state)) {
        return Status::AVOID;
    }
    return Status::UNDONE;
}

/************************************* Oracle Usage ***************************************************/

bool Oracle::is_initial_state_bug(PolicySampled::Path path) {
    /* Given policy_path is an unsafe path, is the initial state on the path a bug -- does a safe policy exist for the initial state. */
    auto s_id = path.states[0];

    auto state = simulatorEnv->get_state(s_id);
    searchStatistics->set_attr_string(PLAJA::StatsString::STATES_FOR_ORACLE, state.save_as_str());
    start_time = std::chrono::steady_clock::now();
    PUSH_LAP(searchStatistics, PLAJA::StatsDouble::TIME_FOR_QUERY);
    bool is_bug = check_safe(state);
    POP_LAP_IF(searchStatistics, PLAJA::StatsDouble::TIME_FOR_QUERY);
    if (is_bug) {
        searchStatistics->inc_attr_unsigned(PLAJA::StatsUnsigned::BUGS_FOUND);
    }
    clear(true); // Clear the cache/value function at the end of the call
    return is_bug;
}

bool Oracle::are_states_on_path_bugs(PolicySampled::Path path) {
    /* Given policy_path is an unsafe path, what states on the paths are bugs -- does a safe policy exist for those states? */
    std::string pathString = get_policy_path_str(path.states);
    searchStatistics->set_attr_string(PLAJA::StatsString::STATES_FOR_ORACLE, pathString);
    bool atleast_one_bug = false;
    for (int i = 0; i < path.states.size() - 1; ++i) {
        auto s_id = path.states[i];
        start_time = std::chrono::steady_clock::now();
        bool is_bug = check_safe(simulatorEnv->get_state(s_id));
        clear(true); // Clear the cache/value function at the end of each state (keep different state processing independent)
        if (is_bug) {
            atleast_one_bug = true;
            searchStatistics->inc_attr_unsigned(PLAJA::StatsUnsigned::BUGS_FOUND);
        }
    }
    searchStatistics->set_attr_string(PLAJA::StatsString::FAULT_SOLVING_TIMES, get_safety_deciding_algorithm_stats_str());
    safety_deciding_algorithm_stats.clear();
    return atleast_one_bug;
}

Oracle::STATE_SAFETY Oracle::is_state_safe(StateID_type state) {
    start_time = std::chrono::steady_clock::now();
    bool is_safe = check_safe(simulatorEnv->get_state(state));
    auto current_time = std::chrono::steady_clock::now();
    auto output = is_safe ? Oracle::STATE_SAFETY::SAFE : Oracle::STATE_SAFETY::UNSAFE;
    if (time_limit_reached(current_time)) {
        output = Oracle::STATE_SAFETY::TIMEOUT;
    }
    clear(true);
    return output;
}

Oracle::STATE_SAFETY Oracle::is_state_action_fault(StateID_type state_id, ActionLabel_type action) {
    start_time = std::chrono::steady_clock::now();
    auto state = simulatorEnv->get_state(state_id);
    bool is_fault = check_if_fault(state, action);
    auto current_time = std::chrono::steady_clock::now();
    if (time_limit_reached(current_time)) {
        return Oracle::STATE_SAFETY::TIMEOUT;
    }    
    clear(true);
    return is_fault ? Oracle::STATE_SAFETY::FAULT : Oracle::STATE_SAFETY::NOT_FAULT;
}


bool Oracle::search_fault_on_path(PolicySampled::Path path) {
    /* Given policy_path is an unsafe path, what policy decisions are faults? */
    std::string pathString = get_policy_path_str(path.states);
    searchStatistics->set_attr_string(PLAJA::StatsString::STATES_FOR_ORACLE, pathString);
    int num_faults_found = 0;
    /* Iterator for the unsafe path */
    int start, end, step;
    if (search_direction_ == SEARCH_DIRECTION::BACKWARD) {
        start = path.states.size() - 2;
        end   = -1;
        step  = -1;
    } else {
        PLAJA_ASSERT(search_direction_ == SEARCH_DIRECTION::FORWARD);
        start = 0;
        end   = path.states.size() - 1;
        step  = 1;
    }

    /* Set start time for call to oracle and iterate through states on the unsafe path. */
    start_time = std::chrono::steady_clock::now();
    for (int i = start; i != end; i += step) {
        auto s_id = path.states[i];
        auto action = path.actions[i];
        auto state = simulatorEnv->get_state(s_id);
        bool fault_found = check_if_fault(state, action);
        if (fault_found) {
            ++num_faults_found;
            faults_found[s_id].push_back(action);
            /* Logging */
            std::cout << "###############################" << std::endl;
            std::cout << "Source: "; state.dump();
            auto action = get_policy_action(state);
            std::cout << "Selected Action: " << action << std::endl;
            std::cout << "Applicable Actions: [";
            for (const auto action: simulatorEnv->extract_applicable_actions(state, false)) {
                std::cout << " " << action;
            }
            std::cout << "]" << std::endl;
            std::cout << "###############################" << std::endl;
        }
        if (num_faults_found >= maxFaults) {
            break;
        }
    }
    searchStatistics->set_attr_string(PLAJA::StatsString::FAULT_SOLVING_TIMES, get_safety_deciding_algorithm_stats_str());
    bool timeout = time_limit_reached(start_time);
    // clear the oracle cache
    clear(timeout || !cacheAcrossPaths); // If caching across paths, then do not wipe everything.
    safety_deciding_algorithm_stats.clear();
    return num_faults_found > 0;
}

Oracle::Status Oracle::check(PolicySampled::Path path) {
    switch (experiment_mode) {
        case EXPERIMENT_MODE::START_STATE_BUG_CHECK: {
            bool bug_found = is_initial_state_bug(path);
            return bug_found ? Oracle::Status::BUG : Oracle::Status::NOT_BUG;
        }
        case EXPERIMENT_MODE::ENTIRE_PATH_BUG_CHECK: {
            bool bug_found = are_states_on_path_bugs(path);
            return bug_found ? Oracle::Status::BUG : Oracle::Status::NOT_BUG;
        }
        case EXPERIMENT_MODE::FAULT_LOCALIZATION: {
            bool fault_detected = search_fault_on_path(path);
            if (fault_detected) { return Oracle::Status::FAULT_ON_PATH; }
            searchStatistics->inc_attr_unsigned(PLAJA::StatsUnsigned::NO_FAULT_PATH);
            return Oracle::Status::NO_FAULT_ON_PATH;
        }
        case EXPERIMENT_MODE::IGNORE: {
            return Oracle::Status::AVOID;
        }
    }
    PLAJA_ABORT  
}

/* Helpers */
ActionLabel_type Oracle::get_policy_action(const State& state) const {
    /* Function to output the policy action on a state. */
    return policy->get_action(state);
}

namespace FA_ORACLE {

    std::unique_ptr<Oracle> construct(const PLAJA::Configuration& config) {
        switch (config.get_option(PLAJA_OPTION::FA_ORACLE::stringToOracle, PLAJA_OPTION::oracle_mode)) {
            case Oracle::ORACLE_MODES::FINITE: {
                return FA_FINITE_ORACLE::construct(config);
            }
            case Oracle::ORACLE_MODES::DSMC: {
                return std::make_unique<DSMCOracle>(config);
            }
            case Oracle::ORACLE_MODES::INFINITE: {
                return FA_INFINITE_ORACLE::construct(config);
            }
            default: {
                PLAJA_ABORT
            }
        }
        PLAJA_ABORT
    }
}// namespace FA_ORACLE

