//
// This file is part of the PlaJA code base.
// Copyright (c) (2023 - 2024) Chaahat Jain.
// The algorithm is taken from LAO*: A heuristic search algorithm that finds solutions with loops by Eric Hansen and Shlomo Zilberstein
// https://www.sciencedirect.com/science/article/pii/S0004370201001060
//
#include "lao_oracle.h"
#include "../../../../../option_parser/option_parser.h"
#include "../../../../../option_parser/option_parser_aux.h"
#include "../../../../../option_parser/plaja_options.h"
#include "../../../../factories/configuration.h"
#include <utility>
#include <stack>
#include <queue>
#include "../../../../successor_generation/base/successor_distribution.h"


bool LAOOracle::no_safe_policy_under_substate(const StateID_type state) const {
    LAO(state);
    if (time_limit_reached(std::chrono::steady_clock::now())) {
        return false;
    }
    return not valueFunc[state];
}


bool LAOOracle::check_if_fault(const State& state, const ActionLabel_type action) const {
    searchStatistics->inc_attr_unsigned(PLAJA::StatsUnsigned::NUM_ORACLE_QUERIES);
    /* Iterate over all successors of policy action */
    auto successors = simulatorEnv->compute_successors(state, action);
    bool fault = false;
    auto time = std::chrono::steady_clock::now();
    for (auto t_id : successors) {
        auto target = simulatorEnv->get_state(t_id);
        bool is_fault = not no_safe_policy_under_substate(state.get_id_value()) and no_safe_policy_under_substate(target.get_id_value());
        if (is_fault) {fault = is_fault; break; }
    }
    auto diff = std::chrono::duration_cast<std::chrono::duration<double>>(std::chrono::steady_clock::now() - time).count();
    /* Increase coverage if processed within time */
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
bool LAOOracle::check_safe(const State& state) const {
    searchStatistics->inc_attr_unsigned(PLAJA::StatsUnsigned::NUM_ORACLE_QUERIES);
    const auto time = std::chrono::steady_clock::now();
    const bool safe = not no_safe_policy_under_substate(state.get_id_value());
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

void LAOOracle::dump_value_function() const {
    for (const auto& state_value : valueFunc) {
        std::cout << "{" << simulatorEnv->get_state(state_value.first).save_as_str() << ": " << state_value.second << "}" << std::endl;
    }
    std::cout << "##### PRINTING FINISHED" << std::endl;
}

/* Helper functions for LAO */
void LAOOracle::LAO(const StateID_type state_0) const {
    tips.insert(state_0);
    if (valueFunc.find(state_0) == valueFunc.end()) {
        valueFunc[state_0] = not check_avoid(simulatorEnv->get_state(state_0));
    }
    while (true) {
        if (time_limit_reached(std::chrono::steady_clock::now())) {
            return;
        }
        auto state = getBestTip(state_0);
        if (state == STATE::none) { break; } // Perform until no more successors left to expand.
        expandAndInitialize(state);
        auto ancestors = get_ancestors(state);
        VI(ancestors); // Compute value iteration on all ancestors
    }
    tips.clear();
}

StateID_type LAOOracle::getBestTip(const StateID_type state_0) const {

    if (tips.find(state_0) != tips.end()) { return state_0; }
    std::queue<StateID_type> q; q.push(state_0); std::unordered_set<StateID_type> visited;

    while (!q.empty()) {
        StateID_type state = q.front(); q.pop();
        if (visited.find(state) != visited.end()) continue;
        visited.insert(state);
        auto action_label = get_greedy_action_label(state);
        auto successors = simulatorEnv->compute_successors(simulatorEnv->get_state(state), action_label);
        for (auto successor : successors) {
            if (tips.find(successor) != tips.end()) return successor;
            if (visited.find(successor) != visited.end()) continue;
            if (is_leaf(successor)) continue;
            q.push(successor);
        }
    }
    return STATE::none;
}

void LAOOracle::expandAndInitialize(const StateID_type state) const {
    /* Increase partial state space with all states reachable from state using any applicable action. */
    expanded.insert(state);
    tips.erase(state);
    if (is_leaf(state)) {
        valueFunc[state] = not check_avoid(simulatorEnv->get_state(state));
        return;
    }
    auto action_labels = simulatorEnv->extract_applicable_actions(simulatorEnv->get_state(state), true);
    for (auto action_label : action_labels) {
        auto successors = simulatorEnv->compute_successors(simulatorEnv->get_state(state), action_label);
        for (auto successor : successors) {
            parent_map[successor].insert({state, action_label});
            if (valueFunc.find(successor) == valueFunc.end()) {
                valueFunc[successor] = not check_avoid(simulatorEnv->get_state(successor));
            }
            if (expanded.find(successor) != expanded.end()) {
                continue;
            }
            tips.insert(successor);
        }
    }
}

std::queue<StateID_type> LAOOracle::get_ancestors(const StateID_type state_0) const {
    /* Compute all ancestors that can reach this state using the greedy policy. */
    std::queue<StateID_type> ancestors; ancestors.push(state_0);
    std::queue<StateID_type> q; q.push(state_0); std::unordered_set<StateID_type> visited;
    while (!q.empty()) {
        StateID_type state = q.front(); q.pop();
        if (visited.find(state) != visited.end()) continue;
        visited.insert(state);
        auto parents = parent_map[state]; // backward search
        for (auto parent : parents) {
            if (get_greedy_action_label(parent.first) != parent.second) continue; // greedy policy parents only!
            ancestors.push(parent.first);
            q.push(parent.first);
        }
    }
    return ancestors;
}

void LAOOracle::VI(std::queue<StateID_type> ancestors) const {
    /* Perform VI on all ancestors until fixpoint*/
    while (true) {
        if (time_limit_reached(std::chrono::steady_clock::now())) {
            return;
        }
        // Copy the queue so we can reprocess all states
        std::queue<StateID_type> temp_queue = ancestors;
        bool converged = true;
        while (!temp_queue.empty()) {
            StateID_type state = temp_queue.front();
            temp_queue.pop();
            // Perform Bellman update
            if (residual(state)) {
                converged = false;
                update(state);
            }
        }

        // Check convergence
        if (converged) { return; }
    }

}

/* Value Function updates. We use V(s) = False if state is failed state and True otherwise. */

ActionLabel_type LAOOracle::get_greedy_action_label(const StateID_type& state) const {
    auto actions = simulatorEnv->extract_applicable_actions(simulatorEnv->get_state(state), true);
    PLAJA_ASSERT(actions.size() > 0);
    auto max_action = actions[0];
    auto max_val = q_val(state, max_action);

    for (auto action : actions) {
        auto val = q_val(state, action);
        if (val > max_val) { max_action = action; max_val = val; }
    }
   return max_action;
}

bool LAOOracle::q_val(const StateID_type state, const ActionLabel_type action_label) const {
    auto successors = simulatorEnv->compute_successors(simulatorEnv->get_state(state), action_label);
    for (auto succ : successors) {
        if (valueFunc.find(succ) == valueFunc.end()) {
            valueFunc[succ] = not check_avoid(simulatorEnv->get_state(succ));
        }
        if (!valueFunc[succ]) return false;
    }
    return true;
}

void LAOOracle::update(StateID_type state) const {
    if (is_leaf(state)) return;
    auto actions = simulatorEnv->extract_applicable_actions(simulatorEnv->get_state(state), true);
    for (auto action : actions) {
        if (q_val(state, action)) {
            valueFunc[state] = true;
            return;
        }
    }
    valueFunc.at(state) = false;
}

bool LAOOracle::residual(StateID_type state) const {
    if (is_leaf(state)) return false;
	auto actions = simulatorEnv->extract_applicable_actions(simulatorEnv->get_state(state), true);
    auto val = false;
    for (auto action : actions) {
        if (q_val(state, action)) { val = true; }
    }
	return not (valueFunc[state] == val);
}

bool LAOOracle::is_leaf(StateID_type state) const {
    auto s = simulatorEnv->get_state(state);
    if (check_avoid(s)) valueFunc[state] = false;
    return check_avoid(s) or check_goal(s) or check_terminal(s);
}


LAOOracle::LAOOracle(const PLAJA::Configuration& config) : InfiniteOracle(config) { PLAJA_ASSERT(cost_objective_ == MAX_COST) }