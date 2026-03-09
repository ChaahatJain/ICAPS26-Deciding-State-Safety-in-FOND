//
// This file is part of the PlaJA code base.
// Copyright (c) (2023 - 2024) Chaahat Jain.
//

#include "max_tarjan_finite_oracle.h"
#include "../../../../../option_parser/option_parser.h"
#include "../../../../../option_parser/option_parser_aux.h"
#include "../../../../../option_parser/plaja_options.h"
#include "../../../../factories/configuration.h"
#include "../../../../fd_adaptions/timer.h"


bool MaxTarjanFinOracle::no_safe_policy_under_substate(const StateID_type state, const std::map<StateID_type, int>& path) const {
    bool no_safe_policy = no_safe_policy_under_substate_recursive(state, 0, path);
    if (!no_safe_policy) {
        if (safeCache.find(state) == safeCache.end()) {
            safeCache[state] = policy_deviations;
        } else {
            safeCache[state] = std::min(safeCache[state], policy_deviations);
        }
    } else {
        unsafeCache[state] = policy_deviations + 1;
    }
   return no_safe_policy;
}

bool MaxTarjanFinOracle::no_safe_policy_under_substate_recursive(const StateID_type state, int policyDistance, const std::map<StateID_type, int>& path) const {
    if (time_limit_reached(std::chrono::steady_clock::now())) {
        return true;
    }

    /* Early Termination */
    if (use_cache) {
        if (safeCache.find(state) != safeCache.end()) {
            if (safeCache.find(state)->second <= policy_deviations - policyDistance) { // if the budget to find a safe policy is less than or equal to remaining budget
                searchStatistics->inc_attr_unsigned(PLAJA::StatsUnsigned::CACHE_ORACLE);
                return false;
            }
        }
        if (unsafeCache.find(state) != unsafeCache.end()) {
            if (unsafeCache.find(state)->second >= policy_deviations - policyDistance) { // If the budget under which no safe policy is found is more than the remaining budget
                searchStatistics->inc_attr_unsigned(PLAJA::StatsUnsigned::CACHE_ORACLE);
                return true;
            }
        }
    }

    const auto currentState = simulatorEnv->get_state(state);
    if (check_avoid(currentState)) {
        if (use_cache) {
            unsafeCache[state] = policy_deviations + 1;
        }
        return true;
    }

    if (check_goal(currentState) or check_terminal(currentState)) {
        if (use_cache) {
            safeCache[state] = 0;
        }
        return false;
    }

    /* Maintain SCC data structures*/
    auto stack_index = state_stack.size();
    state_stack.push(state); lowlink[state] = stack_index; on_stack.insert(state);
    budgetMap[state] = policyDistance;

    bool no_safe_policy = false;
    const auto policy_action = get_policy_action(currentState);
    auto applicableActions = simulatorEnv->extract_applicable_actions(currentState, true);
    for (const auto action: applicableActions) {
        auto pol_dev = action == policy_action ? 0 : 1; // count divergences from original policy
        auto new_policyDistance = policyDistance + pol_dev;
        if (new_policyDistance > policy_deviations) continue;

        const auto successor_states = simulatorEnv->compute_successors(currentState, action);
        no_safe_policy = false;
        for (auto s : successor_states) {
            if (no_safe_policy) {
                break;
            }
            if (on_stack.find(s) != on_stack.end()) {
                lowlink[state] = std::min(lowlink[s], lowlink[state]); // update state lowlink if we are in a cycle.
                if (budgetMap[s] < new_policyDistance) {  return true; } // There was a deviation somewhere to create cycle. No safe policy within budget on this path.
                if (budgetMap[s] == new_policyDistance) { return false; } // There was no deviation to reach this cycle. So, we can stop the search for this branch.
            } else {
                no_safe_policy = no_safe_policy_under_substate_recursive(s, new_policyDistance, path);
            }
        }
        if (time_limit_reached(std::chrono::steady_clock::now())) {
            return true;
        }
        if (!no_safe_policy) {
            if (lowlink[state] == stack_index) {
                safeCache[state] = policyDistance; // keeping track of how much budget used
                while (!state_stack.empty()) {
                    StateID_type v = state_stack.top();
                    state_stack.pop();
                    on_stack.erase(v);
                    budgetMap.erase(v);
                    if (v == state) break;
                }
            }
            return false;
        }
    }

    if (time_limit_reached(std::chrono::steady_clock::now())) {
        return true;
    }
    if (lowlink[state] == stack_index) {
        unsafeCache[state] = policyDistance; // keeping track of how much budget used
        while (!state_stack.empty()) {
            StateID_type v = state_stack.top();
            state_stack.pop();
            on_stack.erase(v);
            budgetMap.erase(v);
            if (v == state) break;
        }
    }

    return true;
}

bool MaxTarjanFinOracle::check_if_fault(const State& state, const ActionLabel_type action) const {
    searchStatistics->inc_attr_unsigned(PLAJA::StatsUnsigned::NUM_ORACLE_QUERIES);
    /* Iterate over all successors */
    auto successors = simulatorEnv->compute_successors(state, action);
    bool fault = false;
    auto time = std::chrono::steady_clock::now();
    for (auto t_id : successors) {
        auto target = simulatorEnv->get_state(t_id);
        bool is_fault = not no_safe_policy_under_substate(
                           state.get_id_value(),
                           std::map<StateID_type, int> {}) and
                       no_safe_policy_under_substate(
                           target.get_id_value(),
                           std::map<StateID_type, int> {});
        if (is_fault) {fault = is_fault; break; }
    }

    /* Increase coverage if result found before time limit.*/
    auto diff = std::chrono::duration_cast<std::chrono::duration<double>>(std::chrono::steady_clock::now() - time).count();
    if (time_limit_reached(std::chrono::steady_clock::now())) {
        clear(!use_cache);
        searchStatistics->inc_attr_unsigned(PLAJA::StatsUnsigned::NUM_TIMEOUT);
        searchStatistics->inc_attr_double(PLAJA::StatsDouble::NO_COVERAGE_TOTAL_TIME, seconds_per_query);
        safety_deciding_algorithm_stats.push_back({false, seconds_per_query});
        return false;
    }
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

bool MaxTarjanFinOracle::check_safe(const State& state) const {
    searchStatistics->inc_attr_unsigned(PLAJA::StatsUnsigned::NUM_ORACLE_QUERIES);
    const auto time = std::chrono::steady_clock::now();
    const bool safe = not no_safe_policy_under_substate(
                           state.get_id_value(),
                           std::map<StateID_type, int> {});

    /* Increase coverage if result found before time limit.*/
    auto diff = std::chrono::duration_cast<std::chrono::duration<double>>(std::chrono::steady_clock::now() - time).count();
    if (time_limit_reached(std::chrono::steady_clock::now())) {
        clear(!use_cache);
        searchStatistics->inc_attr_unsigned(PLAJA::StatsUnsigned::NUM_TIMEOUT);
        searchStatistics->inc_attr_double(PLAJA::StatsDouble::NO_COVERAGE_TOTAL_TIME, seconds_per_query);
        safety_deciding_algorithm_stats.push_back({false, seconds_per_query});
        return false;
    }
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

MaxTarjanFinOracle::MaxTarjanFinOracle(const PLAJA::Configuration& config) : FiniteOracle(config) { PLAJA_ASSERT(cost_objective_ == MAX_COST) }