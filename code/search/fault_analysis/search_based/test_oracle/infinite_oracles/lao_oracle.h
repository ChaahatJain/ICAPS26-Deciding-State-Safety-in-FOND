//
// Created by Chaahat Jain on 17/01/25.
//

#ifndef PLAJA_LAO_ORACLE_H
#define PLAJA_LAO_ORACLE_H

#include "../infinite_oracle.h"

//
// Created by Chaahat Jain on 17/01/25.
//

#include <vector>
#include <unordered_set>
#include "../infinite_oracle.h"
#include <map>
#include <algorithm>
#include <queue>

class LAOOracle final : public InfiniteOracle {
private:
    mutable std::map<StateID_type, std::set<std::pair<StateID_type, ActionLabel_type>>> parent_map;
    mutable std::set<StateID_type> expanded;
    mutable std::set<StateID_type> tips;
    mutable std::map<StateID_type, bool> valueFunc;
    bool no_safe_policy_under_substate(StateID_type state) const;

    bool check_if_fault(const State& state, const ActionLabel_type action) const override;
    bool check_safe(const State& state) const override;
    void dump_value_function() const;

    void LAO(const StateID_type state) const;
    StateID_type getBestTip(StateID_type state) const;
    void expandAndInitialize(StateID_type state) const;
    std::queue<StateID_type> get_ancestors(StateID_type state) const;

    ActionLabel_type get_greedy_action_label(const StateID_type& state) const;
    bool q_val(const StateID_type state, const ActionLabel_type action_label) const;
    void update(StateID_type state) const;
    bool residual(StateID_type state) const;
    bool is_leaf(StateID_type state) const;
    void VI(std::queue<StateID_type> state) const;

    void clear(bool reset_all) const override {
        if (reset_all) {
            std::map<StateID_type, std::set<std::pair<StateID_type, ActionLabel_type>>> empty_map;
            parent_map.swap(empty_map); valueFunc.clear();
        }
         expanded.clear(); tips.clear();
    }
public:
    explicit LAOOracle(const PLAJA::Configuration& config);
    ~LAOOracle() override = default;
    DELETE_CONSTRUCTOR(LAOOracle)
};



#endif //PLAJA_LAO_ORACLE_H
