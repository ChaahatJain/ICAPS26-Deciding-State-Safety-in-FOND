//
// This file is part of the PlaJA code base.
// Copyright (c) (2023 - 2024) Chaahat Jain.
//

#include "shielding.h"
#include "../../../../exception/plaja_exception.h"
#include "../../../../globals.h"
#include "../../../../parser/ast/expression/non_standard/objective_expression.h"
#include "../../../../parser/ast/model.h"
#include "../../../factories/configuration.h"
#include "../../../factories/fault_analysis/fault_analysis_options.h"
#include "../../../fd_adaptions/state.h"
#include "../../../fd_adaptions/timer.h"
#include "../../../information/jani_2_interface.h"
#include "../../../information/model_information.h"
#include "../../../information/property_information.h"
#include "../../../non_prob_search/initial_states_enumerator.h"
#include "../../../successor_generation/simulation_environment.h"
// #include "ATen/core/interned_strings.h"
#include <cmath>
#include "../../../factories/predicate_abstraction/search_engine_config_pa.h"


#include <numeric>

FaultShielding::FaultShielding(const PLAJA::Configuration& config):
    FaultEngine(config)
{
    std::unordered_set<StateID_type> start_states;
    if (startEnumerator) {
        startEnumerator->initialize();

        if (not startEnumerator->samples()) {
            std::vector<StateID_type> startStates;
            for (const auto& start_state: startEnumerator->enumerate_states()) {
                const auto start_id = simulatorEnv->get_state(start_state).get_id_value();
                if (start_states.insert(start_id).second) {
                    startStates.push_back(start_id);
                    searchStatistics->inc_attr_unsigned(PLAJA::StatsUnsigned::START_STATES);
                }
            }

            // Used to split the start state set into training and testing sets.
            size_t splitIndex = static_cast<size_t>(train_test_split * startStates.size());
            std::cout << "Split index is: " << splitIndex << std::endl;
            // Keep only the evaluation part
            evaluationStartStates.assign(startStates.begin() + splitIndex, startStates.end());
            // Keep only the training part
            trainStartStates.assign(startStates.begin(), startStates.begin() + splitIndex);
            startEnumerator = nullptr;
        } 
        else {
            /* Generate training start states and evaluation states by randomly sampling one state per episode */
            for (int i = 0; i < numEpisodes; ++i) {
                auto train_sample = startEnumerator->sample_state();
                PLAJA_ASSERT(train_sample)
                auto start_id = simulatorEnv->get_state(*train_sample).get_id_value();
                if (start_states.insert(start_id).second) {
                    trainStartStates.push_back(start_id);
                    searchStatistics->inc_attr_unsigned(PLAJA::StatsUnsigned::START_STATES);
                }

                auto eval_sample = startEnumerator->sample_state();
                PLAJA_ASSERT(eval_sample)
                start_id = simulatorEnv->get_state(*eval_sample).get_id_value();
                if (start_states.insert(start_id).second) {
                    evaluationStartStates.push_back(start_id);
                    searchStatistics->inc_attr_unsigned(PLAJA::StatsUnsigned::START_STATES);
                }
            }
        }
    }
    startEnumerator = nullptr;
}

FaultShielding::~FaultShielding() = default;

State FaultShielding::sample_start_state(bool using_evaluation_set, int index) {
    const StateID_type start_id = trainStartStates[index % trainStartStates.size()]; // When using a fixed start state set
    return simulatorEnv->get_state(start_id);
}

//

void FaultShielding::compute_fail_fraction(bool use_evaluation_set) {
    // Compute bug fraction for all states in the training set.
    int max_index = use_evaluation_set ? evaluationStartStates.size() : trainStartStates.size();
    int fail_count = 0;
    for (int i = 0; i < max_index; ++i) {
            std::unique_ptr<State> current_state = sample_start_state(use_evaluation_set, i).to_ptr();
            bool fail = false;
            switch (evaluation_metric) {
                case PolicyEvaluationMetric::PI_ENVELOPE: {
                    fail = policy_run_sampler->search_policy_envelope(std::move(current_state));
                    break;
                }
                default: {
                    PLAJA_ABORT
                }
            }
            
            if (fail) {
                fail_count++;
            }
    }
    double failFrac = static_cast<double>(fail_count) / max_index;
    if (use_evaluation_set) {
        searchStatistics->set_attr_double(PLAJA::StatsDouble::EVAL_FAIL_FRACTION, failFrac);
    } else {
        searchStatistics->set_attr_double(PLAJA::StatsDouble::TRAIN_FAIL_FRACTION, failFrac);
    }
}

void FaultShielding::compute_goal_fraction(bool use_evaluation_set) {
    // Compute goal fraction for all states in the training set.
    int max_index = use_evaluation_set ? evaluationStartStates.size() : trainStartStates.size();
    int goal_count = 0;
    for (int i = 0; i < max_index; ++i) {
            std::unique_ptr<State> current_state = sample_start_state(use_evaluation_set, i).to_ptr();
            auto path_ps = policy_run_sampler->sample_path(current_state->get_id_value());
            goal_count += path_ps.second.goal_count;
    }
    // std::cout << "GoalCount is: " << goal_count << std::endl;
    double goalFrac = static_cast<double>(goal_count) / max_index;
    if (use_evaluation_set) {
        searchStatistics->set_attr_double(PLAJA::StatsDouble::EVAL_GOAL_FRACTION, goalFrac);
    } else {
        searchStatistics->set_attr_double(PLAJA::StatsDouble::TRAIN_GOAL_FRACTION, goalFrac);
    }
}

bool FaultShielding::collect_faults() {
    int max_index = trainStartStates.size();
    int faults_found = 0;
    for (int i = 0; i < max_index; ++i) {
        std::unique_ptr<State> current_state = sample_start_state(false, i).to_ptr();
        auto path_ps = policy_run_sampler->sample_path(current_state->get_id_value());
        if (path_ps.second.avoid_count > 0) {
            bool fault = oracle->check(path_ps.first);
            if (fault) {
                faults_found++;
            }
        }
    }
    // update stats with number of faults found
    searchStatistics->inc_attr_int(PLAJA::StatsInt::NUM_FAULTS_FOUND, faults_found);
    return faults_found > 0;
}

SearchEngine::SearchStatus FaultShielding::step() {
    if (episodesCounter >= numEpisodes) {
        std::cout << "Terminated due to reaching max iteration" << std::endl;
        return SearchStatus::SOLVED;
    }

    searchStatistics->inc_attr_unsigned(PLAJA::StatsUnsigned::EPISODES);

    // Policy performance on training set
    compute_goal_fraction(false);
    compute_fail_fraction(false);
    // std::cout << "Training set computation done" << std::endl;
    
    // Policy performance on evaluation set
    compute_goal_fraction(true);
    compute_fail_fraction(true);
    // std::cout << "Eval set computation done" << std::endl;

    // Faults on training set
    bool faults_found = collect_faults();
    if (!faults_found) {
        std::cout << "No faults found" << std::endl;
        return SearchStatus::SOLVED;
    } else {
        std::cout << "In shielding need to update the policy in policy_run_sampler" << std::endl;
        PLAJA_ABORT;
        /*
        * TODO: oracle->get_faults_found. Then policy_run_sampler->set_faults_found.
        */
        trigger_intermediate_stats();
        return SearchStatus::IN_PROGRESS;
    }
}