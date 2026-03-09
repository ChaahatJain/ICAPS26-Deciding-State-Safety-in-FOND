//
// This file is part of the PlaJA code base.
// Copyright (c) (2023 - 2024) Chaahat Jain.
//

// Intuition: Keep track of states explored by policy. If we find unsafe state, then backtrack and try sub-optimal actions. If we reach safety, then we detected a fault.

#ifndef PLAJA_POLICY_ACTION_FAULT_ON_STATE_H
#define PLAJA_POLICY_ACTION_FAULT_ON_STATE_H

#include <vector>
#include <unordered_set>
#include "../../../../parser/ast/expression/forward_expression.h"
#include "../fault_engine.h"
#include "../../../states/forward_states.h"
#include "../../../successor_generation/forward_successor_generation.h"
#include "../../../using_search.h"
#include <stack>
#include "../oracle.h"
#include "../search_statistics_fault.h"

class PolicyActionFaultOnState: public FaultEngine {
private:
    /** **/
    
    /* fault search task specification. */
    std::string faultsPath;
    bool count_unsafety = false;
    std::vector<StateID_type> states;

    SearchStatus step() override;

    void check_if_policy_safe();
    void check_if_policy_action_fault();

public:
    explicit PolicyActionFaultOnState(const PLAJA::Configuration& config);
    ~PolicyActionFaultOnState() override;
    DELETE_CONSTRUCTOR(PolicyActionFaultOnState)

};

namespace PLAJA_OPTION {
    extern const std::string faulty_states_filepath;
    extern const std::string count_unsafe_states;

    namespace FA_POLICY_ACTION_IS_FAULT {
        extern void add_options(PLAJA::OptionParser& option_parser);
        extern void print_options();
    } // namespace FA_DATASET_GENERATION

}// namespace PLAJA_OPTION

#endif //PLAJA_POLICY_ACTION_FAULT_ON_STATE_H
