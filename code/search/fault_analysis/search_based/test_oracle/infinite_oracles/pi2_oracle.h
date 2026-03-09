//
// This file is part of the PlaJA code base.
// Copyright (c) (2023 - 2025) Chaahat Jain, Johannes Schmalz.
//

// Intuition:
// A state s is safe if all possible policy executions are safe from s.

#ifndef PLAJA_PI2_ORACLE_H
#define PLAJA_PI2_ORACLE_H

// #include "komihash.h"
// struct Komihash {
//     std::size_t operator()(unsigned long x) const noexcept {
//         return komihash(&x, sizeof(x), 0);
//     }
// };

#include <vector>
#include <unordered_set>
#include "../infinite_oracle.h"
#include <map>
#include <algorithm>
#include <stack>
#include <utility>
class PI2Oracle final: public InfiniteOracle {
public:
    enum class StateStatus {
        SAFE,
        UNSAFE,
        CHECKING,
    };
    enum class PolicyInit {
        INPUT_POLICY,
        ANTI_INPUT_POLICY,
        RANDOM,
        ZERO
    };

private:

    bool no_safe_policy_under_substate(const StateID_type state, const std::vector<StateID_type>& path) const;

    /* InfiniteOracle methods */
    bool check_if_fault(const State& state, const ActionLabel_type action) const override;
    bool check_safe(const State& state) const override;
    void clear(bool reset_all) const override {
        if (reset_all) {
            std::cout << "--- CLEARING CACHE!" << std::endl;
            unsafe_.clear();
            safe_.clear();
        }
    }

    /* PI2 methods */
    int q_val(const StateID_type state, const ActionLabel_type action_label) const;
    StateStatus pi(const StateID_type state) const;
    StateStatus rec_pi(const StateID_type state, size_t const depth) const;
    mutable bool saw_unsafe_;
    mutable std::unordered_set<StateID_type> seen_;
    mutable std::unordered_set<StateID_type> unsafe_;
    mutable std::unordered_set<StateID_type> safe_;

    PolicyInit const policy_init_ = PolicyInit::ZERO;

    mutable size_t max_depth_seen_ = 0;
    mutable size_t rec_pi_calls_ = 0;

public:
    explicit PI2Oracle(const PLAJA::Configuration& config);
    ~PI2Oracle() override = default;
    DELETE_CONSTRUCTOR(PI2Oracle)
};

#endif //PLAJA_PI2_ORACLE_H
