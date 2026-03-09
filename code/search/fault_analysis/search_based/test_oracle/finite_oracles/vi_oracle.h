//
// Created by Chaahat Jain on 04/11/24.
//

#ifndef VI_ORACLE_H
#define VI_ORACLE_H

#include "../finite_oracle.h"
#include <map>
#include <stack>
#include <unordered_set>
#include <vector>

class VIOracle : public FiniteOracle {
// Currently solved labelling is not implemented or required.

private:
    mutable std::map<const StateID_type, int> valueFunc;
    bool check_if_fault(const State& state, const ActionLabel_type action) const override;
    bool check_safe(const State& state) const override;
    void computeVI(const State& state) const;
    void computeVI(const std::vector<StateID_type>& scc) const;
    std::vector<std::vector<StateID_type>> compute_state_space(const State& state) const;

    /* Value Function Helper Functions */
    int get_maximum_value(const std::vector<StateID_type>& successors) const;
    int get_value_under_action(const State& state, ActionLabel_type action) const;
    bool bellmann(const State& state) const;
    int get_cost(const State& state, ActionLabel_type action) const;
    void clear(bool reset_all) const override {
        if (reset_all) {
            valueFunc.clear();
        }
    }
public:
    explicit VIOracle(const PLAJA::Configuration& config);
    ~VIOracle() = default;
    DELETE_CONSTRUCTOR(VIOracle)
};


#endif //VI_ORACLE_H
