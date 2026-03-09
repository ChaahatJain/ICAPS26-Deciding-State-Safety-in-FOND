//
// This file is part of the PlaJA code base.
// Copyright (c) (2023 - 2024) Chaahat Jain.
// The algorithm is taken from Labeled RTDP: Improving the Convergence of Real-Time Dynamic Programming by Blai Bonet and Hector Geffner
// Implemented based on the pseudocode in https://bonetblai.github.io/reports/lrtdp.pdf
//


#include "lrtdp_oracle.h"

#include "../../../../../option_parser/option_parser.h"
#include "../../../../../option_parser/option_parser_aux.h"
#include "../../../../../option_parser/plaja_options.h"
#include "../../../../factories/configuration.h"
#include <utility>
#include <stack>
#include "../../../../successor_generation/base/successor_distribution.h"

bool LRTDPOracle::no_safe_policy_under_substate(const StateID_type state, const std::vector<StateID_type>& path) const {
    /* Initialize value function with state */
    if (valueFunc.find(state) == valueFunc.end()) {
        valueFunc[state] = not check_avoid(simulatorEnv->get_state(state));
    }
    tips.insert(state);

    while (true) {
        if (solved.find(state) != solved.end()) { break; }
        runTrial(state); // sample greedy policy runs from state until solved
        if (time_limit_reached(std::chrono::steady_clock::now())) {
            return false;
        }
    }
    return not valueFunc[state];
}

bool LRTDPOracle::check_if_fault(const State& state, const ActionLabel_type action) const {
    searchStatistics->inc_attr_unsigned(PLAJA::StatsUnsigned::NUM_ORACLE_QUERIES);
    /* Iterate over all successors to see if any are in the unsafe region. */
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
        /* Timeout scenario*/
        clear(!use_cache);
        searchStatistics->inc_attr_unsigned(PLAJA::StatsUnsigned::NUM_TIMEOUT);
        searchStatistics->inc_attr_double(PLAJA::StatsDouble::NO_COVERAGE_TOTAL_TIME, seconds_per_query);
        safety_deciding_algorithm_stats.push_back({false, seconds_per_query});
        return false;
    }
    // INCREASE COVERAGE since we found an answer
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

bool LRTDPOracle::check_safe(const State& state) const {
    searchStatistics->inc_attr_unsigned(PLAJA::StatsUnsigned::NUM_ORACLE_QUERIES);
    const auto time = std::chrono::steady_clock::now();
    const bool safe = not no_safe_policy_under_substate(state.get_id_value(), {state.get_id_value()});
    /* Increase coverage if query processed in time. Otherwise return timeout. */
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


void LRTDPOracle::expandAndInitialize(const StateID_type state) const {
    /* Add all states reachable from this state using any applicable action to the value function. */
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


/* Helper functions for LRTDP */
void LRTDPOracle::runTrial(const StateID_type state_0) const {
	std::stack<StateID_type> trial_stack;
	std::set<StateID_type> trial_set;
    auto state = state_0;
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
          //  print_trial(trial_stack);
            solved.insert(state);
            break;
        }
        /* Expand state based on greedy action*/
    	auto label = get_greedy_action_label(state);
        update(state);
		auto successor_distribution = simulatorEnv->compute_successor_distribution(simulatorEnv->get_state(state), label);
        const auto current_state = simulatorEnv->sample(*successor_distribution);
        state = current_state->get_id_value();
    }

    /* Try labelling visited states in reverse order */
    while (not trial_stack.empty()) {
    	auto state = trial_stack.top(); trial_stack.pop();
        if (not check_solved(state)) break;
    }
    // print_valueFunc();
}
STMT_IF_DEBUG(
    void LRTDPOracle::print_trial(std::stack<StateID_type> trial_stack) const {
        std::cout << "Trial Stack is: ["  << std::endl;;
        while (not trial_stack.empty()) {
            auto state = trial_stack.top(); trial_stack.pop();
            std::cout << state << " "; simulatorEnv->get_state(state).dump(false);
        }
        std::cout << "]" << std::endl;
    }
)

bool LRTDPOracle::check_solved(const StateID_type state_0) const {
    // Return true if all states below state_0 are solved under the greedy policy.
	bool consistent = true; std::vector<StateID_type> open, closed;
    if (solved.find(state_0) == solved.end()) { open.push_back(state_0); }
    while (!open.empty()) {
        auto state = open.back(); open.pop_back();
		closed.push_back(state);
        if (residual(state)) { consistent = false; continue; } //check residual
        //expand state
        auto action = get_greedy_action_label(state);
        auto successors = simulatorEnv->compute_successors(simulatorEnv->get_state(state), action);
        for (auto succ : successors) {
            if (solved.find(succ) == solved.end() and std::find(open.begin(), open.end(), succ) == open.end() and std::find(closed.begin(), closed.end(), succ) == closed.end()) {
                if (is_leaf(succ)) { solved.insert(succ); continue; }
            	open.push_back(succ);
            }
        }
    }
    if (consistent) {
        // label relevant states
		while (not closed.empty()) {
            auto state = closed.back(); closed.pop_back(); solved.insert(state);
		}
    } else {
        // update states with residuals and ancestors
		while (not closed.empty()) {
            auto state = closed.back(); closed.pop_back();
            update(state);
		}
    }
    return consistent;
}

/* Value Function updates. We use V(s) = False if state is failed state and True otherwise. */

ActionLabel_type LRTDPOracle::get_greedy_action_label(const StateID_type& state) const {
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

/* Q(s, a) AND over all s -a-> s' V(s'). */
bool LRTDPOracle::q_val(const StateID_type state, const ActionLabel_type action_label) const {
    auto successors = simulatorEnv->compute_successors(simulatorEnv->get_state(state), action_label);
    for (auto succ : successors) {
        if (valueFunc.find(succ) == valueFunc.end()) {
            valueFunc[succ] = not check_avoid(simulatorEnv->get_state(succ));
        }
        if (!valueFunc[succ])   {
            return false;
        }
    }
    return true;
}

/* An update is an OR over all Q(s, a)*/
void LRTDPOracle::update(StateID_type state) const {
    if (is_leaf(state)) { return; }
    auto actions = simulatorEnv->extract_applicable_actions(simulatorEnv->get_state(state), true);
    for (auto action : actions) {
        if (q_val(state, action)) { valueFunc[state] = true; return; }
    }
    valueFunc[state] = false;
}

bool LRTDPOracle::residual(StateID_type state) const {
    if (is_leaf(state)) return false; // leafs cannot have residuals
	auto actions = simulatorEnv->extract_applicable_actions(simulatorEnv->get_state(state), true);
    auto val = false;
    for (auto action : actions) {
        if (q_val(state, action)) { val = true; }
    }
	return not (valueFunc[state] == val);
}

bool LRTDPOracle::is_leaf(StateID_type state) const {
    auto s = simulatorEnv->get_state(state);
    if (check_avoid(s)) valueFunc[state] = false;
    return check_avoid(s) or check_goal(s) or check_terminal(s);
}


LRTDPOracle::LRTDPOracle(const PLAJA::Configuration& config) : InfiniteOracle(config) { PLAJA_ASSERT(cost_objective_ == MAX_COST) }