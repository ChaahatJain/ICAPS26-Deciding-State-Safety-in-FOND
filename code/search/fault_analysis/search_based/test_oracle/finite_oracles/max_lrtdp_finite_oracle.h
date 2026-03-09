//
// Created by chaahat on 10.02.25.
//

#ifndef MAX_LRTDP_FINITE_ORACLE_H
#define MAX_LRTDP_FINITE_ORACLE_H

#include <vector>
#include "../finite_oracle.h"
#include <map>
#include <algorithm>
#include <utility>
#include <stack>

class MaxLRTDPFinOracle : public FiniteOracle {
    mutable std::set<StateID_type> solved;
    mutable std::map<StateID_type, int> valueFunc;
    mutable std::set<StateID_type> tips;
    mutable std::set<StateID_type> expanded;
    bool no_safe_policy_under_substate(StateID_type state, const std::vector<StateID_type>& path) const;
    bool check_if_fault(const State& state, const ActionLabel_type action) const override;
    bool check_safe(const State& state) const override;

    void expandAndInitialize(StateID_type state) const;

    /* Helper functions for LRTDP */
    void runTrial(const StateID_type state) const;
    STMT_IF_DEBUG( void print_trial(std::stack<StateID_type> trial_stack) const; )
    bool check_solved(const StateID_type state) const;
    ActionLabel_type get_greedy_action_label(const StateID_type& state) const;
    int q_val(const StateID_type state, const ActionLabel_type action_label) const;
    void update(StateID_type state) const;
    bool residual(StateID_type state) const;
    bool is_leaf(StateID_type state) const;
    void print_solved() const {
        std::cout << "Solved states are: {";
        for (const auto& state : solved) {
            std::cout << state << " ";
        }
        std::cout << "}" << std::endl;
    }

    void print_valueFunc() const {
        std::cout << "Value Function states are: [";
        for (const auto& state : valueFunc) {
            std::cout << "{" << state.first << ", " << state.second << "}, ";
        }
        std::cout << "]" << std::endl;
    }

    void clear(bool reset_all) const override {
        if (reset_all) {
            solved.clear(); valueFunc.clear();
        }
        expanded.clear(); tips.clear();
    }

public:
    explicit MaxLRTDPFinOracle(const PLAJA::Configuration& config);
    ~MaxLRTDPFinOracle() override = default;
    DELETE_CONSTRUCTOR(MaxLRTDPFinOracle)
};



#endif //MAX_LRTDP_FINITE_ORACLE_H
