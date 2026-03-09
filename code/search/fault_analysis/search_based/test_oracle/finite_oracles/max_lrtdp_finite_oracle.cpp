//
// This file is part of the PlaJA code base.
// Copyright (c) (2023 - 2024) Chaahat Jain.
//
#include "max_lrtdp_finite_oracle.h"
#include "../../../../../option_parser/option_parser.h"
#include "../../../../../option_parser/option_parser_aux.h"
#include "../../../../../option_parser/plaja_options.h"
#include "../../../../factories/configuration.h"
#include <utility>
#include <stack>
#include "../../../../successor_generation/base/successor_distribution.h"

bool MaxLRTDPFinOracle::no_safe_policy_under_substate(const StateID_type state, const std::vector<StateID_type>& path) const {
    // Resume search on suboptimal actions.
    if (valueFunc.find(state) == valueFunc.end()) {
        valueFunc[state] = check_avoid(simulatorEnv->get_state(state)) ? policy_deviations + 1 : 0;
    }
    tips.insert(state);
    while (true) {
        if (solved.find(state) != solved.end()) { break; }
        runTrial(state);
        if (time_limit_reached(std::chrono::steady_clock::now())) {
            // std::cout << "Timeout for state: "; simulatorEnv->get_state(state).dump(true);
            return false;
        }
    }
    return valueFunc[state] > policy_deviations;
}

bool MaxLRTDPFinOracle::check_if_fault(const State& state, const ActionLabel_type action) const {
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

    /* Increase coverage if solved within time limit */
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

bool MaxLRTDPFinOracle::check_safe(const State& state) const {
    searchStatistics->inc_attr_unsigned(PLAJA::StatsUnsigned::NUM_ORACLE_QUERIES);
    const auto time = std::chrono::steady_clock::now();
    const bool safe = not no_safe_policy_under_substate(state.get_id_value(), {state.get_id_value()});

    /* Increase coverage if solved within time limit. */
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


void MaxLRTDPFinOracle::expandAndInitialize(const StateID_type state) const {
    /* Expand state at the tip using all applicable actions. */
    expanded.insert(state);
    tips.erase(state);
    if (is_leaf(state)) {
        //  std::cout << "Leaf state: " << state << std::endl;
        valueFunc[state] = check_avoid(simulatorEnv->get_state(state)) ? policy_deviations + 1 : 0;
        return;
    }
    auto action_labels = simulatorEnv->extract_applicable_actions(simulatorEnv->get_state(state), true);
    for (auto action_label : action_labels) {
        auto successors = simulatorEnv->compute_successors(simulatorEnv->get_state(state), action_label);
        for (auto successor : successors) {
            if (valueFunc.find(successor) == valueFunc.end()) {
                valueFunc[successor] = check_avoid(simulatorEnv->get_state(state)) ? policy_deviations + 1 : 0;
            }
            if (expanded.find(successor) != expanded.end()) {
                // std::cout << "Skipping stuff in expansion" << std::endl;
                continue;
            }
            tips.insert(successor);
        }
    }
}


/* Helper functions for LRTDP */
void MaxLRTDPFinOracle::runTrial(const StateID_type state_0) const {
	std::stack<StateID_type> trial_stack;
	std::set<StateID_type> trial_set;
    auto state = state_0;
    int budget = policy_deviations;
    while (solved.find(state) == solved.end()) {
        if (time_limit_reached(std::chrono::steady_clock::now())) {
            return ;
        }
    	if (trial_set.find(state) != trial_set.end()) {
        	break;
        }
        if (tips.find(state) != tips.end()) {
            expandAndInitialize(state);
        }
        trial_stack.push(state); trial_set.insert(state);
        if (is_leaf(state)) {
            solved.insert(state); break;
        }
        /* Termination condition if policy deviations exceeded. */
        if (valueFunc.find(state) != valueFunc.end()) {
            if (valueFunc[state] > policy_deviations) {
                solved.insert(state); break;
            }
        }

        /* Follow greedy policy to expand state */
        ActionLabel_type label;
        auto policy_label = get_policy_action(simulatorEnv->get_state(state));
        if (budget > 0) { // sampling policy runs of specific size
            label = get_greedy_action_label(state);
            if (label != policy_label) budget -= 1;
        } else {
            label = policy_label;
        }

        update(state); // VI update
        /* Sample successor */
		auto successor_distribution = simulatorEnv->compute_successor_distribution(simulatorEnv->get_state(state), label);
        const auto current_state = simulatorEnv->sample(*successor_distribution);
        state = current_state->get_id_value();
    }
    /* Try labelling visited states in reverse order */
    while (not trial_stack.empty()) {
    	state = trial_stack.top(); trial_stack.pop();
        if (not check_solved(state)) break;
    }
}

STMT_IF_DEBUG(
void MaxLRTDPFinOracle::print_trial(std::stack<StateID_type> trial_stack) const {
    std::cout << "Trial Stack is: ["  << std::endl;;
    while (not trial_stack.empty()) {
        auto state = trial_stack.top(); trial_stack.pop();
        std::cout << state << " "; simulatorEnv->get_state(state).dump(false);
    }
    std::cout << "]" << std::endl;
}
)

bool MaxLRTDPFinOracle::check_solved(const StateID_type state_0) const {
	bool consistent = true; std::vector<StateID_type> open, closed;
    if (solved.find(state_0) == solved.end()) { open.push_back(state_0); }
    while (!open.empty()) {
        auto state = open.back(); open.pop_back();
		closed.push_back(state);
        if (residual(state)) { consistent = false; continue; }
        auto action = get_greedy_action_label(state);
        auto successors = simulatorEnv->compute_successors(simulatorEnv->get_state(state), action);
        for (auto succ : successors) {
            if (solved.find(succ) == solved.end() and std::find(open.begin(), open.end(), succ) == open.end() and std::find(closed.begin(), closed.end(), succ) == closed.end()) {
                if (is_leaf(succ)) { solved.insert(succ); continue; }
                if (valueFunc[succ] > policy_deviations) { solved.insert(succ); continue; }
            	open.push_back(succ);
            }
        }
    }
    if (consistent) {
		while (not closed.empty()) {
            auto state = closed.back(); closed.pop_back(); solved.insert(state);
		}

        // print_solved();
    } else {
		while (not closed.empty()) {
            auto state = closed.back(); closed.pop_back();
            update(state);
		}
    }
    return consistent;
}

/* Value function updates
 V(s) = k + 1 if s is a fail state
 V(s) = 0 otherwise.

 In a Bellmann equation, we perform V(s) = min_a (max_{s -a-> s'} [[pi(s) != a]] + V(s')). Cap at k + 1.
 [[pi(s) = a]] is the iverson bracket penalizing deviations.
 */


ActionLabel_type MaxLRTDPFinOracle::get_greedy_action_label(const StateID_type& state) const {
    auto actions = simulatorEnv->extract_applicable_actions(simulatorEnv->get_state(state), true);
    PLAJA_ASSERT(actions.size() > 0);
    auto min_action = actions[0];
    auto min_val = q_val(state, min_action);

    for (auto action : actions) {
        auto val = q_val(state, action);
        if (val < min_val) { min_action = action; min_val = val; }
    }
   return min_action;
}

int MaxLRTDPFinOracle::q_val(const StateID_type state, const ActionLabel_type action_label) const {
    const auto policy_action = get_policy_action(simulatorEnv->get_state(state));
    int action_cost = policy_action == action_label ? 0 : 1; // Computing [[pi(s) != a]]

    auto successors = simulatorEnv->compute_successors(simulatorEnv->get_state(state), action_label); // enumerate successors

    /* Maximimize over successors.*/
    int value = std::numeric_limits<int>::min();
    for (auto succ: successors) {
        if (valueFunc.find(succ) == valueFunc.end()) {
            valueFunc[succ] = check_avoid(simulatorEnv->get_state(succ)) ? policy_deviations + 1: 0;
        }
        value = std::max(value, valueFunc[succ]);
    }

    return std::min(value + action_cost, policy_deviations + 1);
}

void MaxLRTDPFinOracle::update(StateID_type state) const {
    if (is_leaf(state)) { return; }
    auto actions = simulatorEnv->extract_applicable_actions(simulatorEnv->get_state(state), true);
    int value = std::numeric_limits<int>::max();
    for (auto action : actions) {
        value = std::min(value, q_val(state, action));
    }
    valueFunc[state] = std::min(value, policy_deviations + 1);
}

bool MaxLRTDPFinOracle::residual(StateID_type state) const {
    if (is_leaf(state)) return false;
	auto actions = simulatorEnv->extract_applicable_actions(simulatorEnv->get_state(state), true);
    int value = std::numeric_limits<int>::max();
    for (auto action : actions) {
        value = std::min(value, q_val(state, action));
    }
	return not (valueFunc[state] == value);
}

bool MaxLRTDPFinOracle::is_leaf(StateID_type state) const {
    auto s = simulatorEnv->get_state(state);
    if (check_avoid(s)) valueFunc[state] = policy_deviations + 1;
    return check_avoid(s) or check_goal(s) or check_terminal(s);
}


MaxLRTDPFinOracle::MaxLRTDPFinOracle(const PLAJA::Configuration& config) : FiniteOracle(config) { PLAJA_ASSERT(cost_objective_ == MAX_COST) }