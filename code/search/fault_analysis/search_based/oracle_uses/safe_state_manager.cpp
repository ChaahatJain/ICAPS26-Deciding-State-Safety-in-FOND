//
// This file is part of the PlaJA code base.
// Copyright (c) (2023 - 2024) Chaahat Jain.
//

// For finding and saving safe states: ./PlaJA --model-file /home/chaahat/Desktop/PhD_projects/code/PlaJABenchmarks/benchmarks_archive/icaps26_fault_fixing/jani_models_and_teachers/transport/models/linetrack.jani --seed 10 --additional-properties /home/chaahat/Desktop/PhD_projects/code/PlaJABenchmarks/benchmarks_archive/icaps26_fault_fixing/jani_models_and_teachers/transport/additional_properties/safety_verification/linetrack/compact_starts_no_predicates/no_filtering/pa_compact_starts_no_predicates_linetrack_64_64_0.jani --prop 1 --engine FAULT_ANALYSIS --max-time 80000 --nn-interface /home/chaahat/Desktop/PhD_projects/code/PlaJABenchmarks/benchmarks_archive/icaps26_fault_fixing/jani_models_and_teachers/transport/networks/linetrack/no_filtering/linetrack_64_64.jani2nnet --ensemble /home/chaahat/Desktop/PhD_projects/code/PlaJABenchmarks/benchmarks_archive/icaps26_fault_fixing/ensembles/transport/linetrack/linetrack_64_64_5depth_20trees_0.4LR_it0.json --initial-state-enum sample --oracle-use safe_state_manager --retraining-train-test-split 0.8 --evaluation-set --safe-states-filepath test.json --distance-func avoid --save-states

// For loading safe states and evaluating policy on them: ./PlaJA --model-file /home/chaahat/Desktop/PhD_projects/code/PlaJABenchmarks/benchmarks_archive/icaps26_fault_fixing/jani_models_and_teachers/transport/models/linetrack.jani --seed 10 --additional-properties /home/chaahat/Desktop/PhD_projects/code/PlaJABenchmarks/benchmarks_archive/icaps26_fault_fixing/jani_models_and_teachers/transport/additional_properties/safety_verification/linetrack/compact_starts_no_predicates/no_filtering/pa_compact_starts_no_predicates_linetrack_64_64_0.jani --prop 1 --engine FAULT_ANALYSIS --max-time 80000 --nn-interface /home/chaahat/Desktop/PhD_projects/code/PlaJABenchmarks/benchmarks_archive/icaps26_fault_fixing/jani_models_and_teachers/transport/networks/linetrack/no_filtering/linetrack_64_64.jani2nnet --ensemble /home/chaahat/Desktop/PhD_projects/code/fault_testing/send_lorenzo/bad_ensembles/model_repair/store/store-v2/models/gurobi/linetrack/linetrack_64_64_5depth_20trees_0.4LR_it2.json --initial-state-enum sample --oracle-use safe_state_manager --retraining-train-test-split 0.8 --evaluation-set --safe-states-filepath test.json --distance-func avoid --data-filepath test2.json


#include "safe_state_manager.h"

#include "../../../../option_parser/option_parser.h"
#include "../../../../option_parser/option_parser_aux.h"

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

namespace PLAJA_OPTION {
    const std::string safe_states_filepath("safe-states-filepath"); // NOLINT(cert-err58-cpp)
    const std::string save_states_flag("save-states"); // NOLINT(cert-err58-cpp)
    const std::string data_filepath("data-filepath"); //NOLINT(cert-err58-cpp)

    namespace SAFE_STATE_MANAGER {
        void add_options(PLAJA::OptionParser& option_parser) {
            OPTION_PARSER::add_value_option(option_parser, safe_states_filepath);
            OPTION_PARSER::add_value_option(option_parser, data_filepath);
            OPTION_PARSER::add_flag(option_parser, save_states_flag);
        }

        extern void print_options() {
            OPTION_PARSER::print_option(PLAJA_OPTION::safe_states_filepath, "<file>", "Filepath (json) of safe states as a list of states, policy actions. Can be used for loading or writing to.");
            OPTION_PARSER::print_option(PLAJA_OPTION::data_filepath, "<file>", "Filepath (json) for writing listof states, policy actions as well as number of unsafe states we found.");
            OPTION_PARSER::print_flag(PLAJA_OPTION::save_states_flag, "Set flag to store safe states found. Else load safe states.", false);
        }
    }// namespace SAFE_STATE_MANAGER
} // namespace PLAJA_OPTION


SafeStateManager::SafeStateManager(const PLAJA::Configuration& config):
    FaultEngine(config),
    safeStatesPath(config.get_value_option_string(PLAJA_OPTION::safe_states_filepath)),
    dataFilePath(config.has_value_option(PLAJA_OPTION::data_filepath) ? config.get_value_option_string(PLAJA_OPTION::data_filepath) : ""),
    save_states(config.is_flag_set(PLAJA_OPTION::save_states_flag))
{
    PLAJA_ASSERT(policy)
    PLAJA_ASSERT(config.has_value_option(PLAJA_OPTION::ensemble))
    
};

SafeStateManager::~SafeStateManager() = default;

void SafeStateManager::sample_start_states() {
    while (startStates.size() < numEpisodes) {
        auto start_sample = startEnumerator->sample_state();
        const auto start_state_id = simulatorEnv->get_state(*start_sample).get_id_value();
        startStates.push_back(start_state_id);
        searchStatistics->inc_attr_unsigned(PLAJA::StatsUnsigned::START_STATES);
    }

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


std::stringstream SafeStateManager::get_safe_states() {
    std::set<StateID_type> safe_states_global;
    for (int i = 0; i < startStates.size(); ++i) {
        auto current_state_id = startStates[i];
        auto safe_states = policy_run_sampler->get_safe_states_in_policy_envelope(current_state_id);
        // std::cout << "Number of safe states for start state at position : " << i << " is " << safe_states.size() << std::endl;
        safe_states_global.insert(safe_states.begin(), safe_states.end());
    }

    json j;
    j["states"] = json::array();
    j["actions"] = json::array();
    std::cout << "Number of safe states: " << safe_states_global.size() << std::endl;
    for (auto s : safe_states_global) {
        auto state_str = simulatorEnv->get_state(s).to_str();
        j["states"].push_back(json::parse(state_str));
        j["actions"].push_back(policy_run_sampler->get_policy_action(simulatorEnv->get_state(s)));
    }

    std::stringstream ss;
    ss << j.dump();
    return ss;
}

void SafeStateManager::save_data(std::string filepath, std::string buffer_data) {
    std::ofstream eval_out_file(filepath.c_str(), std::ios::app);
    if (eval_out_file.fail()) {
        throw PlajaException(PLAJA_EXCEPTION::fileNotFoundString + filepath.c_str());
    }
    eval_out_file << buffer_data << std::endl;
    eval_out_file.close();
    buffer_data.clear();
}

void SafeStateManager::load_safe_states_data(std::string filepath) {
    startStates.clear();
    std::ifstream in(filepath);
    json j;
    in >> j;
    PLAJA_ASSERT(j.contains("states") && j["states"].is_array())
    for (const auto& state_json : j["states"]) {

        std::vector<PLAJA::integer> ints;
        std::vector<PLAJA::floating> floats;

        for (const auto& val : state_json) {
            if (val.is_number_integer()) {
                ints.push_back(val.get<int>());
            } else if (val.is_number_float()) {
                floats.push_back(val.get<double>());
            }
        }

        StateValues state = StateValues(ints, floats);
        startStates.push_back(simulatorEnv->get_state(state).get_id());
    }
}

std::stringstream SafeStateManager::count_unsafe_states() {
    json j;
    j["states"] = json::array();
    j["actions"] = json::array();
    j["unsafe_count"] = 0;

    for (auto s: startStates) {
        auto state = simulatorEnv->get_state(s);
        auto action = policy_run_sampler->get_policy_action(state);
        j["states"].push_back(json::parse(state.to_str()));
        j["actions"].push_back(action);
    }
    int num_unsafe_states = policy_run_sampler->search_envelope_for_fail_states(startStates);
    j["unsafe_count"] = num_unsafe_states;
    std::stringstream ss;
    ss << j.dump();
    return ss;
}

SearchEngine::SearchStatus SafeStateManager::step() {
    searchStatistics->inc_attr_unsigned(PLAJA::StatsUnsigned::EPISODES);
    // Faults on start state set
    if (save_states) {
        // run policy on start states. Collect all safe states found.
        sample_start_states();
        std::cout << "Total episodes: " << numEpisodes << std::endl;
        auto data = get_safe_states();
        save_data(safeStatesPath, data.str());
    } else {
        // run policy on loaded set of states. Count number of states that are unsafe.
        load_safe_states_data(safeStatesPath);
        auto data = count_unsafe_states();
        save_data(dataFilePath, data.str());
    }
    return SearchStatus::SOLVED;
}