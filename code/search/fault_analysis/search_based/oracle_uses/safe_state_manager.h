//
// This file is part of the PlaJA code base.
// Copyright (c) (2023 - 2024) Chaahat Jain.
//

// Intuition: Keep track of states explored by policy. If we find unsafe state, then backtrack and try sub-optimal actions. If we reach safety, then we detected a fault.

#ifndef PLAJA_SAFE_STATE_MANAGER_H
#define PLAJA_SAFE_STATE_MANAGER_H

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

class SafeStateManager: public FaultEngine {
private:
    /** **/
    
    /* fault search task specification. */
    std::string safeStatesPath;
    std::string dataFilePath;
    bool save_states; // specify whether to save states or load them
    std::vector<StateID_type> safeStates;

    SearchStatus step() override;

    void sample_start_states();

    void save_data(std::string filepath, std::string buffer_data);
    std::stringstream get_safe_states();
    void load_safe_states_data(std::string filepath);
    std::stringstream count_unsafe_states();

public:
    explicit SafeStateManager(const PLAJA::Configuration& config);
    ~SafeStateManager() override;
    DELETE_CONSTRUCTOR(SafeStateManager)

};

namespace PLAJA_OPTION {
    extern const std::string safe_states_filepath;
    extern const std::string save_states_flag;
    extern const std::string data_filepath;

    namespace SAFE_STATE_MANAGER {
        extern void add_options(PLAJA::OptionParser& option_parser);
        extern void print_options();
    } // namespace SAFE_STATE_MANAGER

}// namespace PLAJA_OPTION

#endif //PLAJA_SAFE_STATE_MANAGER_H
