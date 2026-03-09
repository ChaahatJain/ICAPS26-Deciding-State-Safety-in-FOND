//
// This file is part of the PlaJA code base.
// Copyright (c) (2023 - 2024) Chaahat Jain.
//
#include <cmath>
#include "dsmc_oracle.h"
#include "../../../../option_parser/option_parser.h"
#include "../../../../option_parser/option_parser_aux.h"
#include "../../../../option_parser/plaja_options.h"
#include "../../../factories/configuration.h"


namespace PLAJA_OPTION_DEFAULT {
    constexpr double unsafety_threshold(0.05); // NOLINT(cert-err58-cpp)
    constexpr double confidence(0.95); // NOLINT(cert-err58-cpp)
    constexpr double precision_rate(0.01); // NOLINT(cert-err58-cpp)

}

namespace PLAJA_OPTION {
    const std::string unsafety_threshold("unsafety-threshold");// NOLINT(cert-err58-cpp)
    const std::string confidence("confidence");                // NOLINT(cert-err58-cpp)
    const std::string precision_rate("precision-rate");        // NOLINT(cert-err58-cpp)

    namespace DSMC_ORACLE {
        void add_options(PLAJA::OptionParser& option_parser) {
            OPTION_PARSER::add_double_option(option_parser, PLAJA_OPTION::confidence, PLAJA_OPTION_DEFAULT::confidence);
            OPTION_PARSER::add_double_option(option_parser, PLAJA_OPTION::precision_rate, PLAJA_OPTION_DEFAULT::precision_rate);
            OPTION_PARSER::add_double_option(option_parser, PLAJA_OPTION::unsafety_threshold, PLAJA_OPTION_DEFAULT::unsafety_threshold);
        }

        extern void print_options() {
            OPTION_PARSER::print_double_option(PLAJA_OPTION::confidence, PLAJA_OPTION_DEFAULT::confidence, "Confidence that state is safe under policy");
            OPTION_PARSER::print_double_option(PLAJA_OPTION::precision_rate, PLAJA_OPTION_DEFAULT::precision_rate, "Tolerable precision that state is safe under policy");
            OPTION_PARSER::print_double_option(PLAJA_OPTION::unsafety_threshold, PLAJA_OPTION_DEFAULT::unsafety_threshold, "Threshold for percentage of unsafe simulation runs below which state is safe.");
        }
    }// namespace DSMC_ORACLE
}// namespace PLAJA_OPTION

bool DSMCOracle::check_safe(const State& state) const {
    searchStatistics->inc_attr_unsigned(PLAJA::StatsUnsigned::NUM_ORACLE_QUERIES);
    start_time = std::chrono::steady_clock::now();
    const bool fault = not substate_leads_to_unsafe(state.get_id_value());
    if (time_limit_reached(std::chrono::steady_clock::now())) {
        searchStatistics->inc_attr_unsigned(PLAJA::StatsUnsigned::NUM_TIMEOUT);
        searchStatistics->inc_attr_double(PLAJA::StatsDouble::NO_COVERAGE_TOTAL_TIME, seconds_per_query);
        return false;
    }
    searchStatistics->inc_attr_unsigned(PLAJA::StatsUnsigned::COVERAGE);
    if (fault) {
        searchStatistics->inc_attr_double(PLAJA::StatsDouble::YES_COVERAGE_TOTAL_TIME, std::chrono::duration_cast<std::chrono::duration<double>>(std::chrono::steady_clock::now() - start_time).count());
        searchStatistics->inc_attr_unsigned(PLAJA::StatsUnsigned::NUM_YES);
    } else {
        searchStatistics->inc_attr_double(PLAJA::StatsDouble::NO_COVERAGE_TOTAL_TIME, std::chrono::duration_cast<std::chrono::duration<double>>(std::chrono::steady_clock::now() - start_time).count());
        searchStatistics->inc_attr_unsigned(PLAJA::StatsUnsigned::NUM_NO);
    }
    return fault;
}

DSMCOracle::DSMCOracle(const PLAJA::Configuration& config): Oracle(config),
    unsafety_threshold(config.get_double_option(PLAJA_OPTION::unsafety_threshold))
  , confidence(config.get_double_option(PLAJA_OPTION::confidence))
  , precision(config.get_double_option(PLAJA_OPTION::precision_rate))
  , delta(1 - confidence)
 {
    search_direction_ = config.get_option(PLAJA_OPTION::FA_ORACLE::stringToSearch, PLAJA_OPTION::search_dir);
    maxFaults = config.get_int_option(PLAJA_OPTION::num_faults_per_path);

    std::cout << "Confidence is: " << confidence << std::endl;
    std::cout << "precision rate is: " << precision << std::endl;
    std::cout << "Delta is: " << delta << std::endl;
    num_runs = std::ceil((4/(precision*precision))*log2(2/delta));
    std::cout << "Num runs is: " << num_runs << std::endl;
    seconds_per_query = config.get_int_option(PLAJA_OPTION::seconds_per_query);
    experiment_mode = config.get_option(PLAJA_OPTION::FA_ORACLE::stringToExperiment, PLAJA_OPTION::fault_experiment_mode);

 }

bool DSMCOracle::substate_leads_to_unsafe(const StateID_type state) const {
    // auto safe_count = 0;
    for (int i = 0; i < num_runs; ++i) {
        auto currentState = simulatorEnv->get_state(state).to_ptr();
        std::vector<StateID_type> currentPath;
        // bool undone = false;
        for (auto t = 0; t < 1000; ++t) {
            if (time_limit_reached(std::chrono::steady_clock::now())) return false;
            // undone = false; 
            if (check_goal(*currentState)) {
                // ++safe_count;
                break;
            }
            if (check_terminal(*currentState)) {
                // ++safe_count;
                break;
            }
            if (check_avoid(*currentState)) {
                return true;
                break;
            }
            if (check_cycle(currentState->get_id_value(), currentPath)) {
                // ++safe_count;
                break;
            }
            currentPath.push_back(currentState->get_id_value());
            auto action = get_policy_action(*currentState);
            auto succ_state = simulatorEnv->compute_successor_if_applicable(*currentState, action);
            currentState = std::move(succ_state);
            // undone = true;
        }
        // if (undone) ++safe_count;
    }
    return false;
    // double frac_safe = static_cast<double>(safe_count)/num_runs;
    // double frac_unsafe = 1 - frac_safe;
    // return frac_unsafe > unsafety_threshold;
}

bool DSMCOracle::check_if_fault(const State& state, const ActionLabel_type action) const {
    // state s is a fault if all successors from an action label are safe.
    searchStatistics->inc_attr_unsigned(PLAJA::StatsUnsigned::NUM_ORACLE_QUERIES);
    auto successors = simulatorEnv->compute_successors(state, action);
    bool fault = false;
    for (auto t_id : successors) {
        auto target = simulatorEnv->get_state(t_id);
        bool is_fault = not substate_leads_to_unsafe(state.get_id_value()) and substate_leads_to_unsafe(target.get_id_value());
        if (is_fault) {fault = is_fault; break; }
    }
    if (time_limit_reached(std::chrono::steady_clock::now())) {
        searchStatistics->inc_attr_unsigned(PLAJA::StatsUnsigned::NUM_TIMEOUT);
        searchStatistics->inc_attr_double(PLAJA::StatsDouble::NO_COVERAGE_TOTAL_TIME, seconds_per_query);
        return false;
    }
    searchStatistics->inc_attr_unsigned(PLAJA::StatsUnsigned::COVERAGE);
    if (fault) {
        searchStatistics->inc_attr_double(PLAJA::StatsDouble::YES_COVERAGE_TOTAL_TIME, std::chrono::duration_cast<std::chrono::duration<double>>(std::chrono::steady_clock::now() - start_time).count());
        searchStatistics->inc_attr_unsigned(PLAJA::StatsUnsigned::NUM_YES);
    } else {
        searchStatistics->inc_attr_double(PLAJA::StatsDouble::NO_COVERAGE_TOTAL_TIME, std::chrono::duration_cast<std::chrono::duration<double>>(std::chrono::steady_clock::now() - start_time).count());
        searchStatistics->inc_attr_unsigned(PLAJA::StatsUnsigned::NUM_NO);
    }
    return fault;
}

