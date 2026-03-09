//
// This file is part of the PlaJA code base.
// Copyright (c) (2023 - 2024) Chaahat Jain.
//

#include "tarjan_oracle.h"
#include "../../../../factories/configuration.h"
#include "../../../../../option_parser/option_parser.h"
#include "../../../../../option_parser/option_parser_aux.h"
#include "../../../../../option_parser/plaja_options.h"
#include <ATen/Functions.h>
#include <ATen/NativeFunctions.h>
#include <ATen/core/stack.h>

bool TarjanOracle::no_safe_policy_under_substate(const StateID_type state, const std::vector<StateID_type>& path) const {
    rec_calls_ = 0;
    //Resume search on suboptimal actions.
    bool no_safe_policy = no_safe_policy_under_substate_recursive(state, 0, path);
    if (time_limit_reached(std::chrono::steady_clock::now())) return true;
    if (use_cache) {
        cachedStates[state] = no_safe_policy ? StateStatus::UNSAFE : StateStatus::SAFE;
    }
    STMT_IF_DEBUG(std::cout << "rec_calls_ = " << rec_calls_ << std::endl;)
    return no_safe_policy;
}

bool TarjanOracle::no_safe_policy_under_substate_recursive(const StateID_type state, const int steps, const std::vector<StateID_type>& path) const {
    ++rec_calls_;

    // std::cout << "Recursion check on steps: " << steps << " and state: " << state << std::endl;
    if (time_limit_reached(std::chrono::steady_clock::now())) return true;
     const auto currentState = simulatorEnv->get_state(state).to_ptr();
    if (check_avoid(*currentState)) {
        if (use_cache) cachedStates.insert({state, StateStatus::UNSAFE});
        return true;
    }

    if (check_goal(*currentState) or check_terminal(*currentState)) {
        if (use_cache) cachedStates.insert({state, StateStatus::SAFE});
        return false;
    }

    if (cachedStates.find(state) != cachedStates.end()) {
        const auto status = cachedStates.at(state);
        PLAJA_ASSERT(status != StateStatus::CHECKING);
        searchStatistics->inc_attr_unsigned(PLAJA::StatsUnsigned::CACHE_ORACLE);
        return status == StateStatus::UNSAFE;
    }

    auto stack_index = state_stack.size();
    state_stack.push(state); lowlink[state] = stack_index; on_stack.insert(state);
    auto applicable_actions = simulatorEnv->extract_applicable_actions(*currentState, true);
    for (const auto action: applicable_actions) {
        lowlink[state] = stack_index;

        // make sure stack is clean and uncontaminated by recursive calls on other actions
        //
        // TODO: not sure if this is necessary
        while (!state_stack.empty()) {
            StateID_type v = state_stack.top();
            if (v == state) break;
            state_stack.pop();
            on_stack.erase(v);
            lowlink.erase(v);
        }

        auto successor_states = simulatorEnv->compute_successors(*currentState, action);
        bool no_safe_policy = false;
        for (auto s: successor_states) {
            // std::cout << "Successor in recurse: " << s << " on applying action: " << action << " to state: " << currentState->get_id_value() << std::endl;
            // simulatorEnv->get_state(s).dump(true);
            if (no_safe_policy) {
                // make sure stack is clean and uncontaminated by recursive calls on other actions
                //
                // TODO: not sure if this is necessary
                while (!state_stack.empty()) {
                    StateID_type v = state_stack.top();
                    if (v == state) break;
                    state_stack.pop();
                    on_stack.erase(v);
                    lowlink.erase(v);
                }

                break; // escape as soon as one successor is unsafe
            }
            if (on_stack.find(s) != on_stack.end()) {
                // Assume this cycle is safe because we are in the infinite case.
                //
                // BUT! we do not update the lowlink yet.
            } else {
                auto copy = path;
                copy.push_back(s);
                no_safe_policy = no_safe_policy_under_substate_recursive(s, steps + 1, copy);
            }
        }
        if (time_limit_reached(std::chrono::steady_clock::now())) return true;
        if (!no_safe_policy) {

            // Update lowlink now, after "committing" to our current action
            for (auto s: successor_states) {
                if (on_stack.find(s) != on_stack.end()) {
                    lowlink[state] = std::min(lowlink[s], lowlink[state]); // update state lowlink if we are in a cycle. Assume this cycle is safe because we are in the infinite case.
                }
            }

            // std::cout << "Successor in recurse: " << s << " on applying action: " << action << " to state: " << currentState->get_id_value() << std::endl;
            if (lowlink[state] == stack_index) {
                if (cachedStates.find(state) == cachedStates.end()) {
                    // std::cout << "Caching state as safe: "; simulatorEnv->get_state(state).dump();
                    // PLAJA_ASSERT(false)
                    cachedStates[state] = StateStatus::SAFE;
                }
                while (!state_stack.empty()) {
                    StateID_type v = state_stack.top();
                    state_stack.pop();
                    on_stack.erase(v);
                    if (v == state) break;
                }
            }
            return false;
        }
    }
    if (lowlink[state] == stack_index) {
        cachedStates[state] = StateStatus::UNSAFE;
        // std::cout << "Caching state as UNSAFE: "; simulatorEnv->get_state(state).dump();
        while (!state_stack.empty()) {
            StateID_type v = state_stack.top();
            state_stack.pop();
            on_stack.erase(v);
            if (v == state) break;
        }
    }
    return true;
}

bool TarjanOracle::check_if_fault(const State& state, const ActionLabel_type action) const {
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
bool TarjanOracle::check_safe(const State& state) const {
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

TarjanOracle::TarjanOracle(const PLAJA::Configuration& config) : InfiniteOracle(config) { PLAJA_ASSERT(cost_objective_ == MAX_COST) }