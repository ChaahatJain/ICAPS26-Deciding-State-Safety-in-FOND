//
// This file is part of the PlaJA code base.
// Copyright (c) (2023 - 2024) Chaahat Jain.
//

// "/home/chaahat/Desktop/PhD_projects/code/plaja/build/PlaJA" --model-file /home/chaahat/Desktop/PhD_projects/code/PlaJABenchmarks/benchmarks_archive/aaai26/transport/models/linetrack.jani --seed 10 --additional-properties /home/chaahat/Desktop/PhD_projects/code/PlaJABenchmarks/benchmarks_archive/aaai26/transport/additional_properties/safety_verification/linetrack/compact_starts_no_predicates/no_filtering/pa_compact_starts_no_predicates_linetrack_64_64_0.jani --prop 1 --engine FAULT_ANALYSIS --max-time 80000 --nn-interface /home/chaahat/Desktop/PhD_projects/code/PlaJABenchmarks/benchmarks_archive/aaai26/transport/networks/linetrack/no_filtering/linetrack_64_64.jani2nnet --nn /home/chaahat/Desktop/PhD_projects/code/PlaJABenchmarks/benchmarks_archive/aaai26/transport/networks/linetrack/no_filtering/linetrack_64_64.nnet --initial-state-enum sample --oracle-use dataset_generation --oracle-mode infinite --infinite-oracle Tarjan --applicability-filtering 1 --fault-experiment-mode ignore --search-dir backward --seconds-per-query 3600 --num-starts 1000 --distance-func avoid --rand-prob 1 --max-steps-per-start 1010 --retraining-train-test-split 1.0 --dataset-filepath /home/chaahat/Desktop/PhD_projects/code/fault_testing/send_lorenzo/bad_ensembles/model_repair/store/store-v2/data-chaahat-v2/transport//linetrack_64_64_0.json --faults-filepath /home/chaahat/Desktop/PhD_projects/code/fault_testing/send_lorenzo/bad_ensembles/model_repair/store/store-v2/models/linetrack/linetrack_64_64_5depth_20trees_0LR_it0_faults_found.json

#include "retraining_data_generation.h"

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
    const std::string faults_filepath("faults-filepath"); // NOLINT(cert-err58-cpp)
    const std::string dataset_filepath("dataset-filepath"); // NOLINT(cert-err58-cpp)

    namespace FA_DATASET_GENERATION {
        void add_options(PLAJA::OptionParser& option_parser) {
            OPTION_PARSER::add_value_option(option_parser, faults_filepath);
            OPTION_PARSER::add_value_option(option_parser,dataset_filepath);
        }

        extern void print_options() {
            OPTION_PARSER::print_option(PLAJA_OPTION::faults_filepath, "<file>", "Filepath to json of faults stored as a list of state, actions.");
            OPTION_PARSER::print_option(PLAJA_OPTION::dataset_filepath, "<file>", "Filepath to store data after simulation with shield");
        }
    }// namespace FA_DATASET_GENERATION
} // namespace PLAJA_OPTION


FaultDatasetGeneration::FaultDatasetGeneration(const PLAJA::Configuration& config):
    FaultEngine(config),
    faultsPath(config.get_value_option_string(PLAJA_OPTION::faults_filepath)),
    saveAgentActions(config.has_value_option(PLAJA_OPTION::dataset_filepath) ? new std::string(config.get_value_option_string(PLAJA_OPTION::dataset_filepath)) : nullptr)
{
    PLAJA_ASSERT(policy)
    PLAJA_ASSERT(config.has_value_option(PLAJA_OPTION::nn))
    int num_training_start_states = 1000;
    datasetGenerationStartStates.reserve(num_training_start_states);
    std::unordered_set<StateID_type> start_states;
    for (int i = 0; i < num_training_start_states; ++i) {
        if (!startEnumerator->samples()) {
            if (i == startStates.size()) {
                datasetGenerationStartStates.resize(startStates.size());
                break;
            }
            datasetGenerationStartStates.push_back(startStates[i]);
        } else {
            auto start_sample = startEnumerator->sample_state();
            PLAJA_ASSERT(start_sample)
            auto state_id = simulatorEnv->get_state(*start_sample).get_id();
            datasetGenerationStartStates.push_back(state_id);
        }
    }
    policy_run_sampler->load_policy_shield(faultsPath);
}

FaultDatasetGeneration::~FaultDatasetGeneration() = default;


std::stringstream FaultDatasetGeneration::policy_path_to_data(std::vector<StateID_type> start_states) {
    std::stringstream saveAgentActionsBuffer;
    for (int i = 0; i < start_states.size(); ++i) {
        auto current_state_id = start_states[i];
        auto path_ps = policy_run_sampler->sample_path(current_state_id);
        for (auto t = 0; t < path_ps.first.states.size(); ++t) {
            /* Get action and compute successor*/
            saveAgentActionsBuffer << "{\"state\":" << simulatorEnv->get_state(path_ps.first.states[t]).to_str() << PLAJA_UTILS::commaString;
            saveAgentActionsBuffer << "\"label\":" << path_ps.first.actions[t] << "}" << PLAJA_UTILS::commaString;
            // PLAJA_ASSERT(simulatorEnv->is_applicable(simulatorEnv->get_state(current_state_id), action_label));
        }
        // searchStatistics->inc_attr_unsigned(PLAJA::StatsUnsigned::UNDONE_ORACLE);
    }
    return saveAgentActionsBuffer;
}

void FaultDatasetGeneration::generate_training_data() {
    auto buffer_data = policy_path_to_data(datasetGenerationStartStates);
    std::ofstream eval_out_file(saveAgentActions->c_str(), std::ios::app);
    if (eval_out_file.fail()) {
        throw PlajaException(PLAJA_EXCEPTION::fileNotFoundString + saveAgentActions->c_str());
    }
    auto data = buffer_data.str();
    data.pop_back();
    eval_out_file << "{\"sample\":[" << data << "]}" << std::endl;
    eval_out_file.close();
    buffer_data.clear();
}

SearchEngine::SearchStatus FaultDatasetGeneration::step() {
    searchStatistics->inc_attr_unsigned(PLAJA::StatsUnsigned::EPISODES);
    // Faults on start state set
    generate_training_data();
    return SearchStatus::SOLVED;
}