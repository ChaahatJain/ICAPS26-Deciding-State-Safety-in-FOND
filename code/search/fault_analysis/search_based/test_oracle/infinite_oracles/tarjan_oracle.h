//
// This file is part of the PlaJA code base.
// Copyright (c) (2023 - 2024) Chaahat Jain.
//

// Intuition:
// A state s is safe if all possible policy executions are safe from s.

#ifndef PLAJA_TARJAN_ORACLE_H
#define PLAJA_TARJAN_ORACLE_H

#include <vector>
#include <unordered_set>
#include "../infinite_oracle.h"
#include <map>
#include <algorithm>
#include <stack>
#include <utility>
class TarjanOracle final: public InfiniteOracle {
public:
    enum StateStatus {
        SAFE,
        UNSAFE,
        CHECKING,
    };

private:
    // SCC handling data structures
    mutable std::stack<StateID_type> state_stack;
    mutable std::unordered_map<StateID_type, int> lowlink; // Tarjan’s indices to track SCCs
    mutable std::unordered_set<StateID_type> on_stack; // Set to track nodes currently in the stack for SCC detection

    mutable size_t rec_calls_ = 0;

    mutable std::map<StateID_type, StateStatus> cachedStates;
    bool no_safe_policy_under_substate(StateID_type state, const std::vector<StateID_type>& path) const;
    bool no_safe_policy_under_substate_recursive(StateID_type state, int steps, const std::vector<StateID_type>& path) const;
    bool check_if_fault(const State& state, const ActionLabel_type action) const override;
    bool check_safe(const State& state) const override;
    void clear(bool reset_all) const override {
        while (!state_stack.empty()) state_stack.pop(); // Clear the stack
        // index.clear();
        lowlink.clear();
        on_stack.clear();
        // idx = 0;
        if (reset_all) {
            // std::cout << "Clearing the cache" << std::endl;
            cachedStates.clear();
        }
    }

    // Tarjans Algorithm Helper Functions: 
    bool is_inactive(const StateID_type s) const; 

public:
    explicit TarjanOracle(const PLAJA::Configuration& config);
    ~TarjanOracle() override = default;
    DELETE_CONSTRUCTOR(TarjanOracle)
};

#endif //PLAJA_TARJAN_ORACLE_H
