
//
// This file is part of the PlaJA code base.
// Copyright (c) (2023 - 2024) Chaahat Jain.
//

// This file is used to see if policy actions are faults on states that we have found already.

#include "policy_action_fault_on_state.h"

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



// Sample command:  "/home/chaahat/Desktop/PhD_projects/code/plaja/build/PlaJA" --model-file /home/chaahat/Desktop/PhD_projects/code/PlaJABenchmarks/benchmarks_archive/icaps26_fault_fixing/jani_models_and_teachers//one_way_line/models/non_det_with_park/one_way_line.jani --seed 10 --engine FAULT_ANALYSIS --max-time 43200 --ensemble-interface /home/chaahat/Desktop/PhD_projects/code/PlaJABenchmarks/benchmarks_archive/icaps26_fault_fixing/jani_models_and_teachers//one_way_line/networks/non_det_with_park/one_way_line/one_way_line_32_32.jani2nnet --ensemble /home/chaahat/Desktop/PhD_projects/code/fault_testing/send_lorenzo/bad_ensembles/model_repair/store/store-v2/models/gurobi/one_way_line/one_way_line_32_32_non_det_with_park_5depth_20trees_0.4LR_it0.json --initial-state-enum inc --oracle-use policy_action_fault_on_state --oracle-mode infinite --infinite-oracle PI3 --applicability-filtering 1 --fault-experiment-mode aggregate --search-dir backward --seconds-per-query 1800 --num-starts 10000 --distance-func avoid --save-intermediate-stats --stats-file test.csv --cache-fault-search --retain-fault-cache --additional-properties /home/chaahat/Desktop/PhD_projects/code/PlaJABenchmarks/benchmarks_archive/icaps26_fault_fixing/jani_models_and_teachers/one_way_line/additional_properties/safety_verification/det/compact_starts_no_predicates/one_way_line/pa_one_way_line_compact_starts_no_predicates.jani --prop 1 --faulty-states-filepath test.json

namespace PLAJA_OPTION {
    const std::string faulty_states_filepath("faulty-states-filepath"); // NOLINT(cert-err58-cpp)
    const std::string count_unsafe_states("count-unsafe-states"); // NOLINT(cert-err58-cpp)

    namespace FA_POLICY_ACTION_IS_FAULT {
        void add_options(PLAJA::OptionParser& option_parser) {
            OPTION_PARSER::add_value_option(option_parser, faulty_states_filepath);
            OPTION_PARSER::add_flag(option_parser, count_unsafe_states);
        }

        extern void print_options() {
            OPTION_PARSER::print_option(PLAJA_OPTION::faulty_states_filepath, "<file>", "Filepath to json of faulty states stored as a list.");
            OPTION_PARSER::print_flag(PLAJA_OPTION::count_unsafe_states, "Flag set to true if we checking if policy is safe on faulty states.");
        }
    }// namespace FA_DATASET_GENERATION
} // namespace PLAJA_OPTION


PolicyActionFaultOnState::PolicyActionFaultOnState(const PLAJA::Configuration& config):
    FaultEngine(config),
    faultsPath(config.get_value_option_string(PLAJA_OPTION::faulty_states_filepath)),
    count_unsafety(config.is_flag_set(PLAJA_OPTION::count_unsafe_states))
{
    PLAJA_ASSERT(policy)
    PLAJA_ASSERT(oracle)
    PLAJA_ASSERT(simulatorEnvironment)
    // load faults from file
    std::ifstream in(faultsPath);
    json j;
    in >> j;
    PLAJA_ASSERT(j.contains("states") && j["states"].is_array())
    for (const auto& state_json : j["states"]) {

        std::vector<PLAJA::integer> ints;
        std::vector<PLAJA::floating> floats;
        ints.push_back(0); // For the automaton

        for (const auto& val : state_json) {
            if (val.is_number_integer()) {
                ints.push_back(val.get<int>());
            } else if (val.is_number_float()) {
                floats.push_back(val.get<double>());
            }
        }

        StateValues state = StateValues(ints, floats);
        states.push_back(simulatorEnv->get_state(state).get_id());
    }
}

PolicyActionFaultOnState::~PolicyActionFaultOnState() = default;

void PolicyActionFaultOnState::check_if_policy_action_fault() {
    for (auto state_id : states) {
        auto state = simulatorEnv->get_state(state_id);
        auto action = policy->get_action(state);
        Oracle::STATE_SAFETY is_fault = oracle->is_state_action_fault(state_id, action);
        std::cout << "###############################" << std::endl;
        std::cout << "Source: "; state.dump();
        if (is_fault == Oracle::STATE_SAFETY::TIMEOUT) {
            std::cout << "Policy Action: " << action << " timed out." << std::endl;
        } else {
            std::cout << "Policy Action: " << action  << " is " << (is_fault == Oracle::STATE_SAFETY::FAULT ? "a fault.": "not a fault.") << std::endl;
        }
        std::cout << "Applicable Actions: [";
        for (const auto action: simulatorEnv->extract_applicable_actions(state, false)) {
            std::cout << " " << action;
        }
        std::cout << "]" << std::endl;
        std::cout << "###############################" << std::endl;
    }
}

void  PolicyActionFaultOnState::check_if_policy_safe() {
    int num_unsafe_states = policy_run_sampler->search_envelope_for_fail_states(states);
    std::cout << "Number of unsafe states is: " << num_unsafe_states << std::endl;
    std::cout << "Total states checked is: " << states.size() << std::endl;
}


SearchEngine::SearchStatus PolicyActionFaultOnState::step() {
    searchStatistics->inc_attr_unsigned(PLAJA::StatsUnsigned::EPISODES);
    if (count_unsafety) {
        // Is policy safe on start state set
        check_if_policy_safe();
    } else {
        // Faults on start state set
        check_if_policy_action_fault();
    }
    return SearchStatus::SOLVED;
}