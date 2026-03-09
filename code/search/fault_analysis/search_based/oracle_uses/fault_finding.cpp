//
// This file is part of the PlaJA code base.
// Copyright (c) (2023 - 2024) Chaahat Jain.
//

#include "fault_finding.h"
#include "../../../../exception/plaja_exception.h"
#include "../../../../globals.h"
#include "../../../../parser/ast/expression/non_standard/objective_expression.h"
#include "../../../../parser/ast/model.h"
#include "../../../factories/configuration.h"
#include "../../../fd_adaptions/state.h"
#include "../../../fd_adaptions/timer.h"
#include "../../../information/jani_2_interface.h"
#include "../../../information/model_information.h"
#include "../../../information/property_information.h"
#include "../../../non_prob_search/initial_states_enumerator.h"
#include "../../../successor_generation/simulation_environment.h"
// #include "ATen/core/interned_strings.h"
#include <cmath>
#include "../../../factories/fault_analysis/fault_analysis_options.h"
#include "../../../factories/predicate_abstraction/search_engine_config_pa.h"


#include <numeric>
#include <iostream>
#include <fstream>
#include <vector>
#include <json.hpp>
using json = nlohmann::json;

FaultFinding::FaultFinding(const PLAJA::Configuration& config):
    FaultEngine(config),
    paths_to_sample(config.get_int_option(PLAJA_OPTION::num_paths_per_start)),
    verify(config.is_flag_set(PLAJA_OPTION::use_cegar)), 
    ic3_unsafe_path(config.has_value_option(PLAJA_OPTION::ic3_logs) ? config.get_value_option_string(PLAJA_OPTION::ic3_logs) : "")
    {
        /* Start State Sampling */
    std::unordered_set<StateID_type> start_states;
    if (startEnumerator) {
        startEnumerator->initialize();
        bool fixed_start_states = not startEnumerator->samples();
        if (fixed_start_states) {
            /* Enumerate and store states */
            for (const auto& start_state: startEnumerator->enumerate_states()) {
                const auto start_id = simulatorEnv->get_state(start_state).get_id_value();
                if (start_states.insert(start_id).second) {
                    startStates.push_back(start_id);
                    searchStatistics->inc_attr_unsigned(PLAJA::StatsUnsigned::START_STATES);
                }
            }
        } else {
            PLAJA_ASSERT(propertyInfo->get_start())
            startStates.clear();
            while (startStates.size() < numEpisodes) {
                auto start_sample = startEnumerator->sample_state();
                const auto start_state_id = simulatorEnv->get_state(*start_sample).get_id_value();
                startStates.push_back(start_state_id);
                searchStatistics->inc_attr_unsigned(PLAJA::StatsUnsigned::START_STATES);
            }
            startEnumerator = nullptr;
        }

        if (specified_start_state_index >= 0) { 
            // If we run fault analysis from a single start state, then run the entire check exactly once. 
            PLAJA_ASSERT(specified_start_state_index < static_cast<int>(startStates.size()))
            startStates = {startStates[specified_start_state_index]};
            numEpisodes = 1;
            PLAJA_LOG("Using fixed start state index " + std::to_string(specified_start_state_index) + " with state id " + std::to_string(startStates[0]))
        } else {
            // Used to split the start state set into training and testing sets.
            size_t splitIndex = static_cast<size_t>(train_test_split * startStates.size());
            if (evaluation_mode) {
                // Keep only the evaluation part
                startStates.assign(startStates.begin() + splitIndex, startStates.end());
            } else {
                // Keep only the training part
                startStates.assign(startStates.begin(), startStates.begin() + splitIndex);
            }
            numEpisodes = std::min<size_t>(startStates.size(), numEpisodes);
        }
    }
}

FaultFinding::~FaultFinding() = default;

State FaultFinding::sample_start_state() {
    const StateID_type start_id = startStates[episodesCounter % startStates.size()]; // When using a fixed start state set
    return simulatorEnv->get_state(start_id);
}

SearchEngine::SearchStatus FaultFinding::step() {
    if (episodesCounter >= numEpisodes) {
        return SearchStatus::SOLVED;
    }

#ifdef BUILD_PA
/* For fault analysis on unsafe paths obtained via verification. */
    if (verify) {
        std::optional<PolicySampled::Path> unsafe_path;
        if (ic3_unsafe_path.empty()) {
            policy_run_sampler->init_pa_cegar(*start);
            unsafe_path = policy_run_sampler->run_verification(*start, *avoid);
        } else {
            unsafe_path = policy_run_sampler->get_unsafe_path_from_logs(ic3_unsafe_path);
        }
        
        if (!unsafe_path) { 
            std::cout << "Returning solved because empty unsafe states" << std::endl;
            return SearchStatus::SOLVED; 
        }
        for (int i = 0; i < unsafe_path->states.size(); ++i) {
            auto state_id = (*unsafe_path).states[i];
            std::cout << "State in path: "; simulatorEnv->get_state(state_id).dump(nullptr);
        }
        oracle->check(*unsafe_path);
        return SearchStatus::SOLVED;
    }
#endif
    searchStatistics->inc_attr_unsigned(PLAJA::StatsUnsigned::EPISODES);
    
    // sample start state until choice point
    std::unique_ptr<State> current_state = sample_start_state().to_ptr();
    // std::cout << "Episodes counter is at: " << episodesCounter << "and numEpisodes at" << numEpisodes << " with new start state: "; current_state->dump();
    searchStatistics->set_attr_string(PLAJA::StatsString::STATES_FOR_ORACLE, "");
    searchStatistics->set_attr_string(PLAJA::StatsString::FAULT_SOLVING_TIMES, "");
    if (not current_state) { // drawn start state leads to "done" without any decision
        static bool print_warning(true);
        PLAJA_LOG_IF(print_warning, PLAJA_UTILS::to_red_string("Terminal start state."))
        ++episodesCounter;
        print_warning = false;
        return SearchStatus::IN_PROGRESS;
    } else { // found choice point -> count step as episode
        ++episodesCounter;
        searchStatistics->set_attr_string(PLAJA::StatsString::START_STATE_CHECKED, current_state->save_as_str());
    }

    switch (evaluation_metric) {
        case PolicyEvaluationMetric::ALL_PI_PATHS: {
            /* Enumerate all policy paths under the state */
            auto state_id = current_state->get_id_value();
            auto policy_stats = policy_run_sampler->enumerate_all_paths(state_id);
            searchStatistics->inc_attr_unsigned(PLAJA::StatsUnsigned::DONE_GOAL, policy_stats.goal_count);
            searchStatistics->inc_attr_unsigned(PLAJA::StatsUnsigned::DONE_AVOID, policy_stats.avoid_count);
            searchStatistics->inc_attr_unsigned(PLAJA::StatsUnsigned::TERMINAL_STATES, policy_stats.terminal_count);
            searchStatistics->inc_attr_unsigned(PLAJA::StatsUnsigned::PATHS_CHECKED, policy_stats.num_paths); 
            searchStatistics->inc_attr_unsigned(PLAJA::StatsUnsigned::DETECTED_CYCLES, policy_stats.cycle_count);
        }
        break;
        case PolicyEvaluationMetric::SAMPLE_PI_PATHS: {
            /* Sample policy runs to compute policy performance. */
            auto state_id = current_state->get_id_value();
            for (int i = 0; i < paths_to_sample; ++i) {
                searchStatistics->inc_attr_unsigned(PLAJA::StatsUnsigned::PATHS_CHECKED, 1);
                auto path_ps = policy_run_sampler->sample_path(state_id);
                if (path_ps.second.avoid_count > 0) {
                    oracle->check(path_ps.first);
                }
            }
        }
        break;
        case PolicyEvaluationMetric::PI_ENVELOPE: {
            /* Computing envelope to first fail state and then check if state is a bug */
            searchStatistics->inc_attr_unsigned(PLAJA::StatsUnsigned::PATHS_CHECKED, 1);
            policy_run_sampler->search_policy_envelope(std::move(current_state)); // Cheaper than enumerating all policy runs due to early termination on encountering a "fail state"
        }
            break;
        default:
            PLAJA_LOG(PLAJA_UTILS::to_red_string("Unsupported Metric Specified."))
            PLAJA_ABORT
    }
    trigger_intermediate_stats();
    return SearchStatus::IN_PROGRESS;
}