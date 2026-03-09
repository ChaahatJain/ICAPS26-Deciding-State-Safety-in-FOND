//
// This file is part of the PlaJA code base.
// Copyright (c) (2023 - 2024) Chaahat Jain.
//

// Intuition: Keep track of states explored by policy. If we find unsafe state, then backtrack and try sub-optimal actions. If we reach safety, then we detected a fault.

#ifndef PLAJA_FAULT_FINDING_H
#define PLAJA_FAULT_FINDING_H

#include <vector>
#include <unordered_set>
#include "../../../../parser/ast/expression/forward_expression.h"
#include "../fault_engine.h"
#include "../../../information/jani_2_interface.h"
#include "../../../states/forward_states.h"
#include "../../../successor_generation/forward_successor_generation.h"
#include "../../../using_search.h"
#include "../../../non_prob_search/policy/policy_restriction.h"
#include <stack>
#include "../oracle.h"
#include "../search_statistics_fault.h"
#include "../../../../search/smt/model/model_z3.h"
#include "../../../../search/smt/bias_functions/distance_function.h"
#include "../../../predicate_abstraction/cegar/pa_cegar.h"
#include "../../../information/property_information.h"



class FaultFinding: public FaultEngine {
private:
    int paths_to_sample = 1;
    bool verify = false;
    std::string ic3_unsafe_path = "";
    
    SearchStatus step() override;
    State sample_start_state(); 

public:
    explicit FaultFinding(const PLAJA::Configuration& config);
    ~FaultFinding() override;
    DELETE_CONSTRUCTOR(FaultFinding)

};

#endif //PLAJA_FAULT_FINDING_H
