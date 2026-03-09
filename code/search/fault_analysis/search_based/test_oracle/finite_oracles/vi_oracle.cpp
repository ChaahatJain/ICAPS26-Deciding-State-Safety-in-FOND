//
// Created by Chaahat Jain on 04/11/24.
//
#include "vi_oracle.h"
#include "../../../../../option_parser/option_parser.h"
#include "../../../../../option_parser/option_parser_aux.h"
#include "../../../../../option_parser/plaja_options.h"
#include "../../../../factories/configuration.h"
#include "tarjan_scc.cpp"

bool VIOracle::check_if_fault(const State& state, const ActionLabel_type action) const {
    searchStatistics->inc_attr_unsigned(PLAJA::StatsUnsigned::NUM_ORACLE_QUERIES);
    /* Iterate over all successors */
    auto successors = simulatorEnv->compute_successors(state, action);
    bool fault = false;
    auto time = std::chrono::steady_clock::now();
    computeVI(state);
    for (auto t_id : successors) {
        auto target = simulatorEnv->get_state(t_id);
        computeVI(target);
        bool is_fault = valueFunc[state.get_id_value()] < policy_deviations + 1 and valueFunc[t_id] >= policy_deviations + 1;
        if (is_fault) {fault = is_fault; break; }
    }

    /* Increase coverage if answer found in less than time limit */
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

bool VIOracle::check_safe(const State& state) const {
    searchStatistics->inc_attr_unsigned(PLAJA::StatsUnsigned::NUM_ORACLE_QUERIES);
    const auto time = std::chrono::steady_clock::now();
    computeVI(state);
    const bool safe = valueFunc[state.get_id_value()] < policy_deviations + 1;

    /* Increase coverage if answer found in less than time limit */
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
void VIOracle::computeVI(const State& state) const {
    if (valueFunc.find(state.get_id_value()) != valueFunc.end()) { return; }
    auto sccs = compute_state_space(state); // compute the entire state space represented by SCCs
    for (const auto& scc : sccs) {
        computeVI(scc);
    }
}

void VIOracle::computeVI(const std::vector<StateID_type>& scc) const { 
    // Go through each SCC in reverse topological order
    for (auto state: scc) {
        if (check_avoid(simulatorEnv->get_state(state))) valueFunc[state] = policy_deviations + 1;
        else valueFunc[state] = 0;
    }

    /* Apply VI on the SCC until convergence */
    while (true) {
        auto current_time = std::chrono::steady_clock::now();
        if (time_limit_reached(current_time)) break;
        bool scc_fixpoint = true;
        for (const auto state: scc) {
            scc_fixpoint = scc_fixpoint and bellmann(simulatorEnv->get_state(state));
        }
        if (scc_fixpoint) {
            break;
        }
    }
}

std::vector<std::vector<StateID_type>> VIOracle::compute_state_space(const State& state) const {
    /* Use Tarjans algorithm to compute SCCs */
    auto tarjan = TarjanSCC(state.get_id_value(), simulatorEnv, objective, avoid, searchStatistics);
    return tarjan.computeSCCIterative(start_time, seconds_per_query);
}

/* Value Function Helper Functions*/

int VIOracle::get_maximum_value(const std::vector<StateID_type>& successors) const {
    int value = std::numeric_limits<int>::min();
    for (auto successor: successors) {
        PLAJA_ASSERT(valueFunc.find(successor) != valueFunc.end())
        value = std::max(value, valueFunc[successor]);
    }
    return value;
}

int VIOracle::get_value_under_action(const State& state, ActionLabel_type action) const {
    const auto successors = simulatorEnv->compute_successors(state, action);
    const int cost = get_cost(state, action);
    const int max_value = get_maximum_value(successors);
    return max_value + cost;
}

bool VIOracle::bellmann(const State& state) const {
    if (check_avoid(state)) {
        valueFunc[state.get_id_value()] = policy_deviations + 1; return true;
    }
    if (check_goal(state)) { valueFunc[state.get_id_value()] = 0; return true; }
    auto applicable_actions = simulatorEnv->extract_applicable_actions(state, false);
    if (applicable_actions.empty()) { valueFunc[state.get_id_value()] = 0; return true; }
    int min_value = std::numeric_limits<int>::max();
    for (const auto action: applicable_actions) {
        auto val = get_value_under_action(state, action);
        min_value = std::min(min_value, val);
    }
    bool fixpoint = std::min(policy_deviations + 1, valueFunc[state.get_id_value()]) == std::min(policy_deviations + 1, min_value);
    valueFunc[state.get_id_value()] = std::min(policy_deviations + 1, min_value);
    return fixpoint;
}
int VIOracle::get_cost(const State& state, ActionLabel_type action) const {
    const auto policy_action = get_policy_action(state);
    return policy_action == action ? 0 : 1;
}

VIOracle::VIOracle(const PLAJA::Configuration& config) : FiniteOracle(config) { PLAJA_ASSERT(cost_objective_ == MAX_COST) }