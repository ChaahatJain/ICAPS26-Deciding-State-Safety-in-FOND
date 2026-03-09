//
// This file is part of the PlaJA code base.
// Copyright (c) (2019 - 2020) Marcel Vinzent.
// See README.md in the top-level directory for licensing information.
//

#ifndef PLAJA_PROB_SEARCH_NODE_INFO_H
#define PLAJA_PROB_SEARCH_NODE_INFO_H

#include <set>
#include "../using_search.h"

/**
 * Basic info for a probabilistic search node. To be used with basic probabilistic search node, namely ProbSearchNode.
 */
struct ProbSearchNodeInfo {
    enum NodeStatus {
        NEW = 0,
        OPEN = 1,
        CLOSED = 2,
        FIXED = 3, // fixed, i.e., value is determined and choice is made (as well as for all states reachable w.r.t the choice)
                   // currently we only use this for choosing 100% self-loop in FRET for MINPROB (besides the special cases below)
        // SPECIAL CASES of fixed states
        DEAD_END = 4,
        GOAL = 5,
        DONT_CARE = 6 // in general: states which are verified by the search algorithm to be value invariant to the policy choice (with value known);
                      // e.g. for LRTDP on MAXPROB dead-ends, or on MINPROB states with goal probability 1 for any policy
    };

    // StateID_type stateID;

    /* Status */
    NodeStatus status;
    bool marked; // can be used to indicate membership to a list, stack, deque ...
    unsigned int solved;

    /* Value functions */
    QValue_type v_current;

    // state cost: only exit cost as steps cost are evaluated before resetting transient variables,
    // i.e., they depend on the transition not the source state and action only.
    PLAJA::Cost_type cost;

    LocalOpIndex_type policy_choice; // which operator (by local index) is chosen by policy, currently local in state space structure

    explicit ProbSearchNodeInfo():
        status(NEW), marked(false), solved(0),
        v_current(0), cost(0), policy_choice(ACTION::noneLocalOp) {}
    virtual ~ProbSearchNodeInfo() = default;
};

struct UpperAndLowerPNodeInfo: public ProbSearchNodeInfo {
    float v_alternative; // alternative value, typically inadmissible, while default value is admissible, hence e.g. lower bound for PROBMAX
    LocalOpIndex_type policy_alternative;

    UpperAndLowerPNodeInfo(): v_alternative(-1), policy_alternative(ACTION::noneLocalOp) {}
    ~UpperAndLowerPNodeInfo() override = default;
};


#endif //PLAJA_PROB_SEARCH_NODE_INFO_H
