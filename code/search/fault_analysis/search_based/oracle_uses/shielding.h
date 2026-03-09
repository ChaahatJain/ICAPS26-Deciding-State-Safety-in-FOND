//
// This file is part of the PlaJA code base.
// Copyright (c) (2023 - 2024) Chaahat Jain.
//

// Intuition: Keep track of states explored by policy. If we find unsafe state, then backtrack and try sub-optimal actions. If we reach safety, then we detected a fault.

#ifndef PLAJA_FAULT_SHIELDING_H
#define PLAJA_FAULT_SHIELDING_H

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

class FaultShielding: public FaultEngine {
private:


    /** **/
    /* fault search task specification. */
    std::vector<StateID_type> trainStartStates;
    std::vector<StateID_type> evaluationStartStates;

    // To reach first choice point starting from start state.
    State sample_start_state(bool using_evaluation_set, int index); 
    
    SearchStatus step() override;

    bool collect_faults();
    void compute_fail_fraction(bool use_evaluation_set);
    void compute_goal_fraction(bool use_evaluation_set);

public:
    explicit FaultShielding(const PLAJA::Configuration& config);
    ~FaultShielding() override;
    DELETE_CONSTRUCTOR(FaultShielding)

};

#endif //PLAJA_FAULT_SHIELDING_H
