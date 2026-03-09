//
// This file is part of the PlaJA code base.
// Copyright (c) (2023 - 2024) Chaahat Jain.
//

// Intuition: 
// A state s is safe if all possible policy executions are safe from s.

#ifndef PLAJA_MAX_TARJAN_FINITE_ORACLE_H
#define PLAJA_MAX_TARJAN_FINITE_ORACLE_H

#include <vector>
#include <unordered_set>
#include "../finite_oracle.h"
#include <map>
#include <algorithm>
#include <utility>
#include <stack>
class MaxTarjanFinOracle final: public FiniteOracle {
private:
    // SCC handling data structures
    mutable std::stack<StateID_type> state_stack;
    mutable std::unordered_map<StateID_type, int> index, lowlink; // Tarjan’s indices to track SCCs
    mutable std::unordered_set<StateID_type> on_stack; // Set to track nodes currently in the stack for SCC detection
    mutable int idx = 0; // Counter to assign unique indices to states
    mutable std::unordered_map<StateID_type, int> budgetMap;

    /* Caching mechanisms */
    // The pair (s, k) in a safe cache means that there exists a safe policy which deviates with k cost.
    // The pair (s, k) in an unsafe cache means that there does not exist a safe policy with atmost k deviations.
    mutable std::map<StateID_type, int> safeCache;
    mutable std::map<StateID_type, int> unsafeCache;

    bool no_safe_policy_under_substate(StateID_type state, const std::map<StateID_type, int>& path) const;
    bool no_safe_policy_under_substate_recursive(StateID_type state, int policyDistance, const std::map<StateID_type, int>& path) const;
    bool check_if_fault(const State& state, const ActionLabel_type action) const override;
    bool check_safe(const State& state) const override;

    void clear(bool reset_all) const override {
        while (!state_stack.empty()) state_stack.pop(); // Clear the stack
        index.clear();
        lowlink.clear();
        on_stack.clear();
        budgetMap.clear();
        idx = 0;
        if (reset_all) {
            safeCache.clear();  unsafeCache.clear();
        }
    }
public:
    explicit MaxTarjanFinOracle(const PLAJA::Configuration& config);
    ~MaxTarjanFinOracle() override = default;
    DELETE_CONSTRUCTOR(MaxTarjanFinOracle)
};

#endif //PLAJA_MAX_TARJAN_FINITE_ORACLE_H
