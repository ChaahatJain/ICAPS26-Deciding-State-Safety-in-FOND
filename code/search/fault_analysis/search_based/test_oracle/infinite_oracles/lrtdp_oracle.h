//
// Created by Chaahat Jain on 17/01/25.
//

#ifndef PLAJA_LRTDP_ORACLE_H
#define PLAJA_LRTDP_ORACLE_H

#include "../infinite_oracle.h"
#include <algorithm>
#include <map>
#include <stack>
#include <unordered_set>
#include <vector>

class LRTDPOracle final: public InfiniteOracle {
    private:
    /* LRTDP substructures */
    mutable std::set<StateID_type> solved;
    mutable std::map<StateID_type, bool> valueFunc;
    mutable std::set<StateID_type> tips;
    mutable std::set<StateID_type> expanded;

    /* Safety Algorithm calls*/
    bool no_safe_policy_under_substate(StateID_type state, const std::vector<StateID_type>& path) const;
    bool check_if_fault(const State& state, const ActionLabel_type action) const override;
    bool check_safe(const State& state) const override;


    /* Helper functions for LRTDP */
    void expandAndInitialize(StateID_type state) const;
    bool is_leaf(StateID_type state) const;
    void runTrial(const StateID_type state) const;
    STMT_IF_DEBUG( void print_trial(std::stack<StateID_type> trial_stack) const;)
    bool check_solved(const StateID_type state) const;

    /* Value function interactions*/
    ActionLabel_type get_greedy_action_label(const StateID_type& state) const;
    bool q_val(const StateID_type state, const ActionLabel_type action_label) const;
    void update(StateID_type state) const;
    bool residual(StateID_type state) const;

   FCT_IF_DEBUG(void print_solved() const {
        std::cout << "Solved states are: {";
        for (const auto& state : solved) {
            std::cout << state << " ";
        }
        std::cout << "}" << std::endl;
    })

    FCT_IF_DEBUG(void print_valueFunc() const {
        std::cout << "Value Function states are: [";
        for (const auto& state : valueFunc) {
            std::cout << "{" << state.first << ", " << state.second << "}, ";
        }
        std::cout << "]" << std::endl;
    })

    void clear(bool reset_all) const override {
        if (reset_all) {
            solved.clear(); valueFunc.clear();
        }
        expanded.clear(); tips.clear();
    }

public:
    explicit LRTDPOracle(const PLAJA::Configuration& config);
    ~LRTDPOracle() override = default;
    DELETE_CONSTRUCTOR(LRTDPOracle)
};



#endif //PLAJA_LRTDP_ORACLE_H
