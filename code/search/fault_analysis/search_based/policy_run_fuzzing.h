//
// Created by Daniel Sherbakov in 2025.
//

#ifndef POLICY_RUN_FUZZING_H
#define POLICY_RUN_FUZZING_H

#include "../../../parser/ast/expression/non_standard/objective_expression.h"
#include "../../smt/bias_functions/distance_function.h"
#include "../../fd_adaptions/state.h"
#include "../../successor_generation/simulation_environment.h"
#include "../../non_prob_search/policy/policy_restriction.h"
#include "../../../utils/rng.h"

#include <memory>
#include <vector>

class PolicyRunFuzzing {

private:
    PolicyRestriction& policy;
    const Expression& avoid;
    const ObjectiveExpression& objective;
    const SimulationEnvironment& simulation_env;
    std::shared_ptr<Bias::DistanceFunction> distance_to_avoid;

    /* Hyper parameters */
    const int max_policy_length = -1;
    const bool use_probabilistic_sampling = false; // controls whether to sample from distribution or greedily.

public:
    PolicyRunFuzzing(
        PolicyRestriction& policy,
        const Expression& unsafety_condition,
        const ObjectiveExpression& objective_condition,
        const SimulationEnvironment& simulation_env,
        const std::shared_ptr<Bias::DistanceFunction>& distance_to_avoid,
        int max_policy_length,
        bool use_probabilistic_sampling
    );
    std::unique_ptr<State> get_policy_run_successor(const std::vector<StateID_type>& successors);

private:
    std::vector<std::unique_ptr<State>> simulate_policy(StateID_type& state);

    std::unique_ptr<State> sample_successor(
        const std::vector<StateID_type>& successors,
        const std::vector<int>& distances);

    std::unique_ptr<State> greedy_selection(
        const std::vector<StateID_type>& successors,
        const std::vector<int>& distances);

    std::unique_ptr<State> sample_from_distribution(
        const std::vector<StateID_type>& states,
        const std::vector<double>& probabilities);

    bool unique_min_exists_fa(std::unordered_map<StateID_type, int>& successors_to_distance);
    bool check_avoid(const State& state) const;
    bool check_goal(const State& state) const;
    bool check_terminal(const State& state) const;
};

#endif //POLICY_RUN_FUZZING_H
