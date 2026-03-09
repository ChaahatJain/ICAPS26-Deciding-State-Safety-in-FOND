//
// This file is part of the PlaJA code base.
// Copyright (c) (2023 - 2025) Chaahat Jain, Johannes Schmalz.
//

#include "pi2_oracle.h"
#include "../../../../factories/configuration.h"
#include "../../../../../option_parser/option_parser.h"
#include "../../../../../option_parser/option_parser_aux.h"
#include "../../../../../option_parser/plaja_options.h"
#include <ATen/Functions.h>
#include <ATen/NativeFunctions.h>
#include <ATen/core/stack.h>

int PI2Oracle::q_val(const StateID_type state, const ActionLabel_type action_label) const {
    auto successors = simulatorEnv->compute_successors(simulatorEnv->get_state(state), action_label);
    bool all_succs_safe = true;
    for (auto succ : successors) {
        if (unsafe_.find(succ) != unsafe_.end()) {
            return 1;
        }

        auto s = simulatorEnv->get_state(succ);
        if (check_avoid(s)) {
            // HACK
            unsafe_.insert(succ);
            return 1;
        }
        if (check_goal(s) or check_terminal(s)) {
            // HACK
            safe_.insert(succ);
        }

        all_succs_safe &= (safe_.find(succ) != safe_.end());
    }

    if (all_succs_safe) {
        safe_.insert(state);
        return -1;
    }

    return 0;
}


bool PI2Oracle::no_safe_policy_under_substate(const StateID_type state, const std::vector<StateID_type>& path) const {
    return pi(state) == StateStatus::UNSAFE;
}


PI2Oracle::StateStatus PI2Oracle::pi(const StateID_type state) const {
    size_t iters = 0;
    max_depth_seen_ = 0;
    rec_pi_calls_ = 0;
    while (true) {
        // std::cout << "pi iter " << iters << std::endl;

        if (time_limit_reached(std::chrono::steady_clock::now())) {
            return StateStatus::UNSAFE; // TODO: what's the correct return for timeout?
        }

        seen_.clear();
        saw_unsafe_ = false;
        auto x = rec_pi(state, 0);
        if (!saw_unsafe_ or x == StateStatus::SAFE) {
            std::cout << "SAFE!" << std::endl;
            std::cout << "rec_pi calls " << rec_pi_calls_ << std::endl;
            simulatorEnv->get_state(state).dump();
            return StateStatus::SAFE;
        }
        if (x == StateStatus::UNSAFE) {
            std::cout << "UNSAFE!" << std::endl;
            std::cout << "rec_pi calls " << rec_pi_calls_ << std::endl;
            simulatorEnv->get_state(state).dump();
            return StateStatus::UNSAFE;
        }

        ++iters;
    }
}

PI2Oracle::StateStatus PI2Oracle::rec_pi(const StateID_type state, size_t const depth) const {
    ++rec_pi_calls_;

    if (time_limit_reached(std::chrono::steady_clock::now())) {
        return StateStatus::CHECKING; // TODO: what's the correct return for timeout?
    }


    // Base cases from cache
    if (unsafe_.find(state) != unsafe_.end()) {
        saw_unsafe_ = true;
        // std::cout << "unsafe cache" << std::endl;
        return StateStatus::UNSAFE;
    }
    if (safe_.find(state) != safe_.end()) {
        // std::cout << "safe cache" << std::endl;
        // simulatorEnv->get_state(state).dump();
        return StateStatus::SAFE;
    }
    if (seen_.find(state) != seen_.end()) {
        // std::cout << "seen cache" << std::endl;
        return StateStatus::CHECKING;
    }

    // Base cases
    auto s = simulatorEnv->get_state(state);
    if (check_goal(s) or check_terminal(s)) {
        safe_.insert(state);
        // std::cout << "Found safe" << std::endl;
        return StateStatus::SAFE;
    }
    else if (check_avoid(s)) {
        saw_unsafe_ = true;
        unsafe_.insert(state);
        // std::cout << "Found UNsafe" << std::endl;
        return StateStatus::UNSAFE;
    }


    // Mark as seen
    seen_.insert(state);

    if (depth > max_depth_seen_) {
        max_depth_seen_ = depth;
        // std::cout << "depth = " << depth << std::endl;
    }

    // Initialise applicable actions and default "min_action"
    auto actions = simulatorEnv->extract_applicable_actions(s, true);
    ActionLabel_type min_action;

    switch (policy_init_)
    {
    case PolicyInit::INPUT_POLICY:
        min_action = get_policy_action(s);
        break;

    case PolicyInit::ANTI_INPUT_POLICY:
        if (actions[0] == get_policy_action(s) and actions.size() > 1) {
            min_action = actions[1];
        } else {
            min_action = actions[0];
        }
        break;

    case PolicyInit::RANDOM:
        min_action = actions[0]; // TODO: NOT IMPLEMENTED
        break;

    case PolicyInit::ZERO:
        min_action = actions[0];
        break;
    }

    auto min_val = q_val(state, min_action);
    PLAJA_ASSERT(actions.size() > 0);

    // Update policy(s) if there is a strictly better action
    update_policy:

    // TODO: OPTIMISATION: throw out actions with Q(s,a) = 1 --> these will never beat min
    for (auto action : actions) {
        auto val = q_val(state, action);
        if (val < min_val) { min_action = action; min_val = val; }
        if (min_val == -1) {
            safe_.insert(state);
        // std::cout << "return safe" << std::endl;
            return StateStatus::SAFE;
        }
    }

    // If the best action is unsafe then s is unsafe
    if (min_val == 1) {
        saw_unsafe_ = true;
        unsafe_.insert(state);
        // std::cout << "return unsafe" << std::endl;
        return StateStatus::UNSAFE;
    }


    // Evaluate successors -- update policy as soon as unsafety is detected
    PLAJA_ASSERT(min_val == 0);
    bool all_succs_safe = true;
    auto successors = simulatorEnv->compute_successors(s, min_action);
    for (auto succ : successors) {
        auto x = rec_pi(succ, depth+1);
        if (x == StateStatus::UNSAFE) {
            // This action is unsafe, so go pick a better one
            PLAJA_ASSERT(q_val(state, min_action) == 1);
            min_val = 1;
            goto update_policy;
        }
        all_succs_safe &= (x == StateStatus::SAFE); // TODO: is this ever true?
    }

    if (all_succs_safe) { // TODO: can this happen?
        safe_.insert(state);
        // std::cout << "return safe" << std::endl;
        return StateStatus::SAFE;
    }

        // std::cout << "return checking" << std::endl;
    return StateStatus::CHECKING;
}

bool PI2Oracle::check_if_fault(const State& state, const ActionLabel_type action) const {
    // INCREASE QUERY COUNTER
    searchStatistics->inc_attr_unsigned(PLAJA::StatsUnsigned::NUM_ORACLE_QUERIES);
    auto successors = simulatorEnv->compute_successors(state, action);
    bool fault = false;
    auto time = std::chrono::steady_clock::now();
    for (auto t_id : successors) {
        auto target = simulatorEnv->get_state(t_id);
        bool is_fault = not no_safe_policy_under_substate(
                           state.get_id_value(),
                           {state.get_id_value()}) and
                       no_safe_policy_under_substate(
                           target.get_id_value(),
                           {target.get_id_value()});
        if (is_fault) {fault = is_fault; break; }
    }
    auto diff = std::chrono::duration_cast<std::chrono::duration<double>>(std::chrono::steady_clock::now() - time).count();
    if (time_limit_reached(std::chrono::steady_clock::now())) {
        clear(!use_cache);
        searchStatistics->inc_attr_unsigned(PLAJA::StatsUnsigned::NUM_TIMEOUT);
        searchStatistics->inc_attr_double(PLAJA::StatsDouble::NO_COVERAGE_TOTAL_TIME, seconds_per_query);
        safety_deciding_algorithm_stats.push_back({false, seconds_per_query});
        return false;
    }
    // INCREASE QUERY COVERAGE
    searchStatistics->inc_attr_unsigned(PLAJA::StatsUnsigned::COVERAGE);
    if (fault) {
        searchStatistics->inc_attr_double(PLAJA::StatsDouble::YES_COVERAGE_TOTAL_TIME, diff);
        searchStatistics->inc_attr_unsigned(PLAJA::StatsUnsigned::NUM_YES);
    } else {
        searchStatistics->inc_attr_double(PLAJA::StatsDouble::NO_COVERAGE_TOTAL_TIME, diff);
        searchStatistics->inc_attr_unsigned(PLAJA::StatsUnsigned::NUM_NO);
    }
    safety_deciding_algorithm_stats.push_back({fault, diff});
    clear(!use_cache);
    return fault;
}
bool PI2Oracle::check_safe(const State& state) const {
    searchStatistics->inc_attr_unsigned(PLAJA::StatsUnsigned::NUM_ORACLE_QUERIES);
    const auto time = std::chrono::steady_clock::now();
    const bool safe = not no_safe_policy_under_substate(state.get_id_value(), {state.get_id_value()});
    auto diff = std::chrono::duration_cast<std::chrono::duration<double>>(std::chrono::steady_clock::now() - time).count();
    if (time_limit_reached(std::chrono::steady_clock::now())) {
        clear(!use_cache);
        searchStatistics->inc_attr_unsigned(PLAJA::StatsUnsigned::NUM_TIMEOUT);
        searchStatistics->inc_attr_double(PLAJA::StatsDouble::NO_COVERAGE_TOTAL_TIME, seconds_per_query);
        safety_deciding_algorithm_stats.push_back({false, seconds_per_query});
        return false;
    }
    // INCREASE QUERY COVERAGE
    searchStatistics->inc_attr_unsigned(PLAJA::StatsUnsigned::COVERAGE);
    if (safe) {
        searchStatistics->inc_attr_double(PLAJA::StatsDouble::YES_COVERAGE_TOTAL_TIME, std::chrono::duration_cast<std::chrono::duration<double>>(std::chrono::steady_clock::now() - start_time).count());
        searchStatistics->inc_attr_unsigned(PLAJA::StatsUnsigned::NUM_YES);
    } else {
        searchStatistics->inc_attr_double(PLAJA::StatsDouble::NO_COVERAGE_TOTAL_TIME, std::chrono::duration_cast<std::chrono::duration<double>>(std::chrono::steady_clock::now() - start_time).count());
        searchStatistics->inc_attr_unsigned(PLAJA::StatsUnsigned::NUM_NO);
    }
    safety_deciding_algorithm_stats.push_back({safe, diff});
    STMT_IF_DEBUG(
        if (safe) {
            std::cout << "Safe policy found for state: "; state.dump(true);
        }
        if (!safe) {
        std::cout << "No safe policy found for state: "; state.dump(true);
        }
    )

    clear(!use_cache);
    return safe;
}

PI2Oracle::PI2Oracle(const PLAJA::Configuration& config) : InfiniteOracle(config) { PLAJA_ASSERT(cost_objective_ == MAX_COST); }