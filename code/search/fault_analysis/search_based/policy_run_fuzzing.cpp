//
// Created by Daniel Sherbakov in 2025.
//

#include "policy_run_fuzzing.h"

#include <climits>
#include <cmath>
#include <numeric>

PolicyRunFuzzing::PolicyRunFuzzing(
    PolicyRestriction& policy,
    const Expression& unsafety_condition,
    const ObjectiveExpression& objective_condition,
    const SimulationEnvironment& simulation_env,
    const std::shared_ptr<Bias::DistanceFunction>& distance_to_avoid,
    const int max_policy_length,
    const bool use_probabilistic_sampling):
    policy(policy),
    avoid(unsafety_condition),
    objective(objective_condition),
    simulation_env(simulation_env),
    distance_to_avoid(distance_to_avoid),
    max_policy_length(max_policy_length),
    use_probabilistic_sampling(use_probabilistic_sampling) {}

/**
* Executes the policy on each successor and evaluates the leaves with a distance function.
* if a leaf of a successor state is identified s.t. it is the only leaf with minimum distance,
* the policy execution is stopped. Otherwise, the policy is executed on the leaves of the successor states.
* After policy execution, each successor is sampled from a discrete distribution based on the minimum distance of the leaves,
* or greeddily selected
*
* @return successor state sampled from probability distribution based on the distance of the leaves.
*/
std::unique_ptr<State> PolicyRunFuzzing::get_policy_run_successor(const std::vector<StateID_type>& successors) {
    // Timer timer(1800);// 30 minutes.
    bool discriminated = false; // unique min distance found.
    std::unordered_map<StateID_type, std::set<StateID_type>> successors_to_leaves;
    std::unordered_map<StateID_type, std::set<StateID_type>> successors_to_expanded_leaves;
    std::unordered_map<StateID_type, int> successors_to_distance;
    std::unordered_set<StateID_type> successors_with_terminal_leaves;

    for (auto s: successors) { successors_to_distance[s] = distance_to_avoid->evaluate(simulation_env.get_state(s)); }
    discriminated = unique_min_exists_fa(successors_to_distance);

    // initialize with self.
    for (auto s: successors) { successors_to_leaves[s] = {s}; }

    int num_steps = 0;
    // run policy one step at a time until unique min distance is found, no progress can be made, or time-limit reached.
    // !timer.is_expired() can be used to keep track of time limit if needed
    while (!discriminated) {
        std::unordered_map<StateID_type, std::set<StateID_type>> successors_to_leaves_tmp;
        for (auto s: successors) {
            if (successors_with_terminal_leaves.count(s) == 1) { continue; } // skip states with terminal leaves.
            // compute new leaves.
            std::vector<std::unique_ptr<State>> new_leaves;
            std::set<StateID_type> new_leaf_ids;

            for (StateID_type leaf_id: successors_to_leaves[s]) {
                auto leaf_states = simulate_policy(leaf_id); // single step
                successors_to_expanded_leaves[s].insert(leaf_id);
                for (auto& l: leaf_states) {
                    if (successors_to_expanded_leaves[s].count(l->get_id()) == 1) { continue; }
                    new_leaves.push_back(std::move(l));
                }
            }

            // map leaves to distances
            std::unordered_map<StateID_type, int> leaf_to_distance;
            int min_distance = INT_MAX;
            for (const auto& leaf: new_leaves) {
                if (check_goal(*leaf)) {
                    leaf_to_distance[leaf->get_id()] = INT_MAX;
                    continue;
                }
                const auto d = distance_to_avoid->evaluate(*leaf);
                if (d < min_distance) { min_distance = d; }
                leaf_to_distance[leaf->get_id()] = d;
            }

            // get minimum distance leaves. (prune leaves with distance greater than min_distance)
            std::set<StateID_type> minimal_leaves;
            for (const auto& leaf: new_leaves) {
                if (leaf_to_distance[leaf->get_id()] == min_distance) { minimal_leaves.insert(leaf->get_id()); }
            }

            successors_to_leaves_tmp[s] = minimal_leaves;
            successors_to_distance[s] = min_distance;
        }
        num_steps++;

        // update leaves.
        successors_to_leaves = successors_to_leaves_tmp;

        // check if unique minimal leaf found.
        if (unique_min_exists_fa(successors_to_distance)) {
            discriminated = true;
            break;
        }

        // if all states only have terminal leaves, break.
        if (successors_with_terminal_leaves.size() == successors.size()) { break; }

        // check if all leaves are terminal.
        for (const auto& [s, leaves]: successors_to_leaves) {
            bool all_leaves_terminal = std::all_of(leaves.begin(), leaves.end(), [this](const auto& state) {
                return check_terminal(simulation_env.get_state(state));
            });
            if (all_leaves_terminal) { successors_with_terminal_leaves.insert(s); }
        }

        // limit policy execution length.
        if (max_policy_length == num_steps) { break; }
    }

    // split states and distances
    std::vector<StateID_type> successor_ids;
    std::vector<int> distances;
    for (const auto& [s, d]: successors_to_distance) {
        successor_ids.push_back(s);
        distances.push_back(d);
    }

    // sample state based on distances or select state with min distance.
    return use_probabilistic_sampling ? sample_successor(successor_ids, distances)
                                      : greedy_selection(successor_ids, distances);
}

/**
 * Simulates policy for a single step. absorbing on goals and avoids.
 * @return successor states after policy execution or empty set if dead end.
 * If given state is terminal, returns the state itself.
 */
std::vector<std::unique_ptr<State>> PolicyRunFuzzing::simulate_policy(StateID_type& state) {
    // if terminal return state
    if (check_goal(simulation_env.get_state(state)) || check_avoid(simulation_env.get_state(state))) {
        std::vector<std::unique_ptr<State>> terminal_state;
        terminal_state.push_back(simulation_env.get_state(state).to_ptr());
        return terminal_state;
    }
    const State s = simulation_env.get_state(state);
    const auto action = policy.get_action(s);
    std::vector<StateID_type> successor_ids = simulation_env.compute_successors(s, action);
    std::vector<std::unique_ptr<State>> successor_states;
    for (const auto& s_id: successor_ids) { successor_states.push_back(simulation_env.get_state(s_id).to_ptr()); }
    return successor_states;
}

/**
 * Computes discrete probability distribution based on values.
 * @param values Random variable values.
 * @return discrete probability distribution of the values.
 */
template<typename T> std::vector<double> get_probability_distribution(const std::vector<T>& values) {
    std::vector<double> probabilities;
    if (values.empty()) { throw std::invalid_argument("Input values cannot be empty."); }
    const double sum = std::accumulate(values.begin(), values.end(), 0.0);
    if (sum == 0) { throw std::invalid_argument("Sum of values is zero."); }
    for (auto d: values) { probabilities.push_back(d / sum); }
    return probabilities;
}

/**
 * @param successors_to_distance map from successor states to their distance.
 * @return true if a unique state with minimum distance is found.
 */
bool PolicyRunFuzzing::unique_min_exists_fa(std::unordered_map<StateID_type, int>& successors_to_distance) {
    std::set<StateID_type> min_states;
    int min_distance = INT_MAX;
    for (const auto& [s, d]: successors_to_distance) {
        if (d < min_distance) {
            min_distance = d;
            min_states.clear();
            min_states.insert(s);
        } else if (d == min_distance) {
            min_states.insert(s);
        }
    }
    return min_states.size() == 1;
}

/**
* Selects the states with minimum distance.
* If multiple states have min distance than return uniformly random one.
*/
std::unique_ptr<State> PolicyRunFuzzing::greedy_selection(
    const std::vector<StateID_type>& successors,
    const std::vector<int>& distances) {
    // get successors with min distance and return uniformly random state.
    const int min_distance = *std::min_element(distances.begin(), distances.end());
    std::vector<StateID_type> minimal_states;
    for (size_t i = 0; i < distances.size(); ++i) {
        if (distances[i] == min_distance) { minimal_states.push_back(successors[i]); }
    }
    if (minimal_states.size() > 1) {
        return simulation_env.get_state(minimal_states[PLAJA_GLOBAL::rng->index(minimal_states.size())]).to_ptr();
    }
    return simulation_env.get_state(minimal_states[0]).to_ptr();
}

/**
    * Samples a successor state based on the distances of the states.
    * The lower the distance the higher the sampling probability.
    * Uses softmax to get probabilities of the states and samples a state from the distribution.
    * @param successors sample set of successor states.
    * @param distances values on which the softmax is applied.
    * @return state from the distribution.
 */
std::unique_ptr<State> PolicyRunFuzzing::sample_successor(
    const std::vector<StateID_type>& successors,
    const std::vector<int>& distances) {
    // if distances are the same return uniformly random state.
    if (std::adjacent_find(distances.begin(), distances.end(), std::not_equal_to<>()) == distances.end()) {
        return sample_from_distribution(successors, std::vector<double>(successors.size(), 1.0 / successors.size()));
    }

    // compute softmax values
    std::vector<double> exp_values;
    double alpha = 1.0;
    for (double d: distances) { exp_values.push_back(std::exp(-alpha * d)); }

    // compute softmax probabilities
    std::vector<double> softmax_probabilities;
    double sum_exp = std::accumulate(exp_values.begin(), exp_values.end(), 0.0);
    for (auto d: exp_values) { softmax_probabilities.push_back(d / sum_exp); }

    return sample_from_distribution(successors, softmax_probabilities);
}

/**
 * Performs inverse transform sampling to sample a state from a given distribution.
 *
 * @param states list of states to sample from.
 * @param probabilities list of probabilities corresponding to a discrete probability distribution of the states.
 * @return State from the distribution.
 */
std::unique_ptr<State> PolicyRunFuzzing::sample_from_distribution(
    const std::vector<StateID_type>& states,
    const std::vector<double>& probabilities) {
    const auto p = PLAJA_GLOBAL::rng->prob();
    double cumulative_probability = 0.0;
    for (size_t i = 0; i < probabilities.size(); ++i) {
        cumulative_probability += probabilities[i];
        if (p <= cumulative_probability) { return simulation_env.get_state(states[i]).to_ptr(); }
    }
    return simulation_env.get_state(states.back()).to_ptr();
}

bool PolicyRunFuzzing::check_avoid(const State& state) const { return avoid.evaluateInteger(&state); }

bool PolicyRunFuzzing::check_goal(const State& state) const {
    auto* goal = objective.get_goal();
    return goal and goal->evaluateInteger(&state);
}

bool PolicyRunFuzzing::check_terminal(const State& state) const {
    auto applicable_actions = simulation_env.extract_applicable_actions(state, true);
    return applicable_actions.empty();
}