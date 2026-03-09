//
// This file is part of the PlaJA code base.
// Copyright (c) (2023 - 2024) Chaahat Jain.
//

#include "fault_engine.h"
#include "../../../option_parser/option_parser.h"
#include "../../../option_parser/option_parser_aux.h"
#include "../../../exception/plaja_exception.h"
#include "../../../globals.h"
#include "../../../parser/ast/expression/non_standard/objective_expression.h"
#include "../../../parser/ast/model.h"
#include "../../factories/configuration.h"
#include "../../fd_adaptions/state.h"
#include "../../fd_adaptions/timer.h"
#include "../../information/jani_2_interface.h"
#include "../../information/model_information.h"
#include "../../information/property_information.h"
#include "../../non_prob_search/initial_states_enumerator.h"
#include "../../successor_generation/simulation_environment.h"
// #include "ATen/core/interned_strings.h"
#include <cmath>
#include "../../factories/fault_analysis/fault_analysis_options.h"
#include <numeric>
#include <iostream>
#include <fstream>
#include <vector>
#include <json.hpp>
using json = nlohmann::json;

namespace PLAJA_OPTION_DEFAULT {
    const std::string evaluation_method("sample_policy_paths"); // NOLINT(cert-err58-cpp)
    const int fixed_start_state_index(-1); // NOLINT(cert-err58-cpp)
} // namespace PLAJA_OPTION_DEFAULT

namespace PLAJA_OPTION {
    const std::string evaluation_method("evaluation-method"); // NOLINT(cert-err58-cpp)
    const std::string fixed_start_state_index("fixed-start-state-index"); // NOLINT(cert-err58-cpp)

    namespace FA_ENGINE {
        std::string print_supported_evaluation_metrics() { return PLAJA::OptionParser::print_supported_options(stringToFailMetric); }

        void add_options(PLAJA::OptionParser& option_parser) {
            OPTION_PARSER::add_option(option_parser, evaluation_method, PLAJA_OPTION_DEFAULT::evaluation_method);
            OPTION_PARSER::add_int_option(option_parser, PLAJA_OPTION::fixed_start_state_index, PLAJA_OPTION_DEFAULT::fixed_start_state_index);
            PLAJA_OPTION::POLICY_RUN_SAMPLER::add_options(option_parser);
        }

        extern void print_options() {
            OPTION_PARSER::print_option(PLAJA_OPTION::evaluation_method, OPTION_PARSER::valueStr, PLAJA_OPTION_DEFAULT::evaluation_method, "Specify the method for evaluating the policy", true);
            OPTION_PARSER::print_additional_specification(print_supported_evaluation_metrics(), true);
            OPTION_PARSER::print_int_option(PLAJA_OPTION::fixed_start_state_index, PLAJA_OPTION_DEFAULT::fixed_start_state_index, "If set to a non-negative value, the start state is fixed to the indexed state in the enumerated start state set. Otherwise, all start states are used.", false);
            PLAJA_OPTION::POLICY_RUN_SAMPLER::print_options();

        }
    }// namespace FA_ENGINE
} // namespace PLAJA_OPTION

FaultEngine::FaultEngine(const PLAJA::Configuration& config):
    SearchEngine(config)
    , numEpisodes(config.get_int_option(PLAJA_OPTION::num_starts))
    , episodesCounter(0)
    , evaluation_mode(config.is_flag_set(PLAJA_OPTION::evaluation_set))
    , train_test_split(config.get_double_option(PLAJA_OPTION::retraining_train_test_split))
    , startEnumerator(propertyInfo->get_start() ? new InitialStatesEnumerator(config, *propertyInfo->get_start()) : nullptr)
    , z3model(get_or_create_z3model(config))
    , specified_start_state_index(config.get_int_option(PLAJA_OPTION::fixed_start_state_index))
    , evaluation_metric(config.get_option(PLAJA_OPTION::FA_ENGINE::stringToFailMetric, PLAJA_OPTION::evaluation_method))
{
    FAULT_ANALYSIS::add_stats(*searchStatistics);
    /* Set goal and fail conditions */
    PLAJA_ASSERT(propertyInfo->get_learning_objective())
    PLAJA_ASSERT(propertyInfo->get_reach())
    objective = propertyInfo->get_learning_objective(); // objective is goal
    start = propertyInfo->get_start();
    avoid = propertyInfo->get_reach(); // reach is avoid, i.e., the safety property to check
    /* Interface for policies*/
    jani2Interface = propertyInfo->get_interface();
    PLAJA_ASSERT(jani2Interface)
    PLAJA_ASSERT(jani2Interface->_do_applicability_filtering())
    PLAJA_ASSERT(train_test_split <= 1.0)
    policy = std::make_shared<PolicyRestriction>(config, *jani2Interface);

    /* Simulation Environment */
    distance_to_avoid = std::make_shared<Bias::DistanceFunction>(*avoid, *z3model, Bias::get_function_type(config.get_value_option_string(PLAJA_OPTION::distance_func)));
    simulatorEnv = std::make_shared<SimulationEnvironment>(config, *model);
    PLAJA_GLOBAL::randomizeNonDetEval = true; // For simulation.
    policy_run_sampler = std::make_unique<PolicyRunSampler>(config, simulatorEnv, policy, objective, avoid, searchStatistics, distance_to_avoid);

    /* Oracle to determine whether a state is safe. */
    oracle = FA_ORACLE::construct(config);
    oracle->initialize_oracle(simulatorEnv, policy, objective, avoid, searchStatistics);

}

FaultEngine::~FaultEngine() = default;

std::shared_ptr<const ModelZ3> FaultEngine::get_or_create_z3model(const PLAJA::Configuration& config) {
    /* Required for distance function computation */
    if (!config.has_sharable(PLAJA::SharableKey::MODEL_Z3)) {
        config.set_sharable(PLAJA::SharableKey::MODEL_Z3, std::make_shared<ModelZ3>(config));
    }
    return config.get_sharable_as_const<ModelZ3>(PLAJA::SharableKey::MODEL_Z3);
}

std::unique_ptr<State> FaultEngine::simulate_until_choice_point(const State& state) const {
    /* Simulate until more than one action is applicable (the choice point). */
    std::unique_ptr<State> successor_state = state.to_ptr();

    // simulate until next choice point
    auto applicableActions = simulatorEnv->extract_applicable_actions(*successor_state, true);
    while (applicableActions.size() <= 1) {

        if (applicableActions.empty()) {
            searchStatistics->inc_attr_unsigned(PLAJA::StatsUnsigned::TERMINAL_STATES);
            return nullptr;
        }

        // step:
        const ActionLabel_type next_action = applicableActions[0];
        if (jani2Interface->is_learned(next_action)) { break; } // may happen on ipc benchmarks
        PLAJA_ASSERT(next_action == ACTION::silentAction)

        successor_state = simulatorEnv->compute_successor(*successor_state, next_action);

        // termination checks
        if (oracle->get_status(*successor_state) == Oracle::Status::GOAL) {
            searchStatistics->inc_attr_unsigned(PLAJA::StatsUnsigned::GOAL_STATES);
            return nullptr;
        }
        if (oracle->get_status(*successor_state) == Oracle::Status::AVOID) { return nullptr; }

        // applicable actions for next iteration
        applicableActions = simulatorEnv->extract_applicable_actions(*successor_state, true);
    }

    return successor_state;
}

//

SearchEngine::SearchStatus FaultEngine::initialize() { return SearchStatus::IN_PROGRESS; }

/* Statistics handling */
void FaultEngine::print_statistics() const {
    searchStatistics->print_statistics();
}

void FaultEngine::stats_to_csv(std::ofstream& file) const {
    searchStatistics->set_attr_unsigned(PLAJA::StatsUnsigned::EPISODES, episodesCounter); // work-around to order stats properly
    searchStatistics->stats_to_csv(file);
}

void FaultEngine::stat_names_to_csv(std::ofstream& file) const {
    searchStatistics->stat_names_to_csv(file);
}
