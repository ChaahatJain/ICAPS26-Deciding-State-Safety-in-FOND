//
// This file is part of the PlaJA code base.
// Copyright (c) (2023 - 2024) Chaahat Jain.
//

// Intuition: Keep track of states explored by policy. If we find unsafe state, then backtrack and try sub-optimal actions. If we reach safety, then we detected a fault.

#ifndef PLAJA_FAULT_RETRAINING_H
#define PLAJA_FAULT_RETRAINING_H

#include <vector>
#include <unordered_set>
#include "../../../../parser/ast/expression/forward_expression.h"
#include "../fault_engine.h"
#include "../../../information/jani2nnet/jani_2_nnet.h"
#include "../../../states/forward_states.h"
#include "../../../successor_generation/forward_successor_generation.h"
#include "../../../using_search.h"
#include "../../../non_prob_search/policy/policy_restriction.h"
#include <stack>
#include "../oracle.h"
#include "../search_statistics_fault.h"

class FaultDatasetGeneration: public FaultEngine {
private:
    /** **/
    
    /* fault search task specification. */
    std::string faultsPath;
    std::unique_ptr<std::string> saveAgentActions;
    std::vector<StateID_type> datasetGenerationStartStates;
    std::unique_ptr<Jani2NNet> teacher_interface;
    std::shared_ptr<PolicyRestriction> teacher_policy;

    SearchStatus step() override;

    void generate_training_data();
    std::stringstream policy_path_to_data(std::vector<StateID_type> start_states);


public:
    explicit FaultDatasetGeneration(const PLAJA::Configuration& config);
    ~FaultDatasetGeneration() override;
    DELETE_CONSTRUCTOR(FaultDatasetGeneration)

};

namespace PLAJA_OPTION {
    extern const std::string faults_filepath;
    extern const std::string dataset_filepath;

    namespace FA_DATASET_GENERATION {
        extern void add_options(PLAJA::OptionParser& option_parser);
        extern void print_options();
    } // namespace FA_DATASET_GENERATION

}// namespace PLAJA_OPTION

#endif //PLAJA_FAULT_RETRAINING_H
