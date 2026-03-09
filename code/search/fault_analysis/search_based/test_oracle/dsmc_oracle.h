//
// This file is part of the PlaJA code base.
// Copyright (c) (2023 - 2024) Chaahat Jain.
//

// Intuition: 
// A policy execution is safe if it does not end up in unsafe state.
// Given confidence b, precision rate epsilon, and delta = 1 - b; compute total runs N.
// With (1 - delta) confidence, the true probability of reaching an unsafe state is less than epsilon.


#ifndef PLAJA_DSMC_ORACLE_H
#define PLAJA_DSMC_ORACLE_H

#include <vector>
#include <unordered_set>
#include "../oracle.h"
#include <algorithm>

class DSMCOracle : public Oracle {

private:
    double unsafety_threshold;
    double confidence;
    double precision;
    double delta;
    int num_runs;
    bool substate_leads_to_unsafe(const StateID_type state) const;
    bool check_if_fault(const State& state, const ActionLabel_type action) const override;
    bool check_safe(const State& state) const override;
    void clear(bool reset_all) const override {};
public:
    explicit DSMCOracle(const PLAJA::Configuration& config);
    ~DSMCOracle() = default;
    DELETE_CONSTRUCTOR(DSMCOracle)
};

namespace PLAJA_OPTION {
    extern const std::string unsafety_threshold;
    extern const std::string confidence;
    extern const std::string precision_rate;

    namespace DSMC_ORACLE {
        extern void add_options(PLAJA::OptionParser& option_parser);
        extern void print_options();
    }

}

#endif //PLAJA_DSMC_ORACLE_H
