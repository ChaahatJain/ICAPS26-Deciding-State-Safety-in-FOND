#include "policy_run_sampler.h"

#include "../../../option_parser/option_parser.h"
#include "../../../option_parser/option_parser_aux.h"

#include "../../non_prob_search/policy/policy_restriction.h"

#include "../../../parser/ast/expression/forward_expression.h"
#include "../../../parser/ast/expression/non_standard/objective_expression.h"

#include "../../factories/configuration.h"
#include "../../factories/predicate_abstraction/search_engine_config_pa.h"

#include "../../information/property_information.h"

#include "../../fd_adaptions/timer.h"
#include "../../../globals.h"

// TODO: Create constructor and connect it with actual fault analysis engine.

namespace PLAJA_OPTION_DEFAULT {
    constexpr int max_steps_per_start(1000); // NOLINT(cert-err58-cpp)
    constexpr double rand_prob(0); // NOLINT(cert-err58-cpp)
    constexpr int sampling_lookahead(-1); // NOLINT(cert-err58-cpp)
} // namespace PLAJA_OPTION_DEFAULT

namespace PLAJA_OPTION {
    const std::string max_steps_per_start("max-steps-per-start"); // NOLINT(cert-err58-cpp)    
    /* Biased sampling */
    const std::string rand_prob("rand-prob"); // NOLINT(cert-err58-cpp)
    const std::string distance_func("distance-func"); // NOLINT(cert-err58-cpp)
    const std::string use_prob_sampling("use-prob-sampling"); // NOLINT(cert-err58-cpp)
    const std::string sampling_lookahead("sampling-lookahead"); // NOLINT(cert-err58-cpp)

    namespace POLICY_RUN_SAMPLER {
        void add_options(PLAJA::OptionParser& option_parser) {
            OPTION_PARSER::add_int_option(option_parser, PLAJA_OPTION::max_steps_per_start, PLAJA_OPTION_DEFAULT::max_steps_per_start);

        // Biased Sampling
            OPTION_PARSER::add_double_option(option_parser, PLAJA_OPTION::rand_prob, PLAJA_OPTION_DEFAULT::rand_prob);
            OPTION_PARSER::add_value_option(option_parser, PLAJA_OPTION::distance_func);
            OPTION_PARSER::add_flag(option_parser, PLAJA_OPTION::use_prob_sampling);
            OPTION_PARSER::add_int_option(option_parser, PLAJA_OPTION::sampling_lookahead, PLAJA_OPTION_DEFAULT::sampling_lookahead);
        }

        extern void print_options() {
        // Biased Sampling
            OPTION_PARSER::print_double_option(PLAJA_OPTION::rand_prob, PLAJA_OPTION_DEFAULT::rand_prob, "probability of choosing a random transition rather than controlled policy-run sampling.");
            OPTION_PARSER::print_option(PLAJA_OPTION::distance_func, "< avoid | wp >", "Distance functions to different conditions.");
            OPTION_PARSER::print_flag(PLAJA_OPTION::use_prob_sampling,"True if sample successors from a distribution based on distance to unsafety to enable exploration. False if greedy exploration based on lowest distance.");
            OPTION_PARSER::print_int_option(PLAJA_OPTION::sampling_lookahead, PLAJA_OPTION_DEFAULT::sampling_lookahead, "Specify how far ahead should be looked. -1 if search should continue until discrimination.");
        }
    }// namespace POLICY_RUN_SAMPLER
} // namespace PLAJA_OPTION


PolicyRunSampler::PolicyRunSampler(const PLAJA::Configuration& config, const std::shared_ptr<SimulationEnvironment>& env, const std::shared_ptr<PolicyRestriction>& pol, const ObjectiveExpression* goal, const Expression* reach, const std::shared_ptr<PLAJA::StatsBase>& stats, const std::shared_ptr<Bias::DistanceFunction> distance_function) :
maxLengthEpisode(config.get_int_option(PLAJA_OPTION::max_steps_per_start)),
rand_prob(config.get_double_option(PLAJA_OPTION::rand_prob)),
sampling_lookahead(config.get_int_option(PLAJA_OPTION::sampling_lookahead)),
prob_sampling(config.is_flag_set(PLAJA_OPTION::use_prob_sampling)),
config(config)
{
    simulatorEnv = env;
    policy = pol;
    objective = goal;
    avoid = reach;
    searchStatistics = stats;
    
    fuzzer = std::make_unique<PolicyRunFuzzing>(
        *policy,
        *avoid,
        *objective,
        *simulatorEnv,
        distance_function,
        maxLengthEpisode,
        prob_sampling
    );
}

bool PolicyRunSampler::check_avoid(const State& state) const { return avoid->evaluateInteger(&state); }

bool PolicyRunSampler::check_cycle(StateID_type state, std::vector<StateID_type> states) { return std::find(states.begin(), states.end(), state) != states.end(); }

bool PolicyRunSampler::check_goal(const State& state) const { 
    auto* goal = objective->get_goal();
    return goal and goal->evaluateInteger(&state); 
}

bool PolicyRunSampler::check_terminal(const State& state) const { 
    auto applicable_actions = simulatorEnv->extract_applicable_actions(state, true);
    return applicable_actions.empty();
}

void PolicyRunSampler::load_policy_shield(std::string faults_path) {
    std::ifstream in(faults_path);
    json j;
    in >> j;
    policy_shield.clear();
    for (const auto& fault_json : j) {
        // std::vector<double> state;
        // state.push_back(0);  // Always start with 0

        std::vector<PLAJA::integer> ints;
        std::vector<PLAJA::floating> floats;
        std::vector<ActionLabel_type> actions;
        ints.push_back(0);
        for (const auto& val : fault_json["state"]) {
            if (val.is_number_integer()) {
                ints.push_back(val.get<int>());
            } else if (val.is_number_float()) {
                floats.push_back(val.get<double>());
            }
        }

        for (const auto& act : fault_json["actions"]) {
            actions.push_back(act.get<int>());
        }

        StateValues state = StateValues(ints, floats);

        policy_shield[simulatorEnv->get_state(state).get_id_value()] = actions;
    }
}

void PolicyRunSampler::set_policy_shield(std::unordered_map<StateID_type, std::vector<ActionLabel_type>> faults) {
     for (auto& [state_id, actions] : faults) {
        auto& target_vec = policy_shield[state_id];  // creates an empty vector if not present
        target_vec.insert(target_vec.end(), std::make_move_iterator(actions.begin()), std::make_move_iterator(actions.end()));
    }
}

ActionLabel_type PolicyRunSampler::get_policy_action(const State& state) const {
    /* Function to output the policy action on a state. */
    if (policy_shield.size() > 0) {
        auto action_values = policy->action_values(state);
        auto applicable_actions = simulatorEnv->extract_applicable_actions(state, false);
        const auto& shielded_actions = policy_shield[state.get_id_value()];
        double max_value = -std::numeric_limits<double>::infinity();
        ActionLabel_type best_action = -1; // invalid action index
        for (ActionLabel_type a : applicable_actions) {
            /* When shielding, prune actions that already exist in policy_shield[state] */
            if (std::find(shielded_actions.begin(), shielded_actions.end(), a) == shielded_actions.end()) {
                double val = action_values[a];
                if (val > max_value) {
                    max_value = val;
                    best_action = a;
                }
            } 
        }
        PLAJA_ASSERT(best_action != -1)
        return best_action;
    }
    /* When not shielding, use the applicable action with the highest value. */
    return policy->get_action(state);
 }

std::unique_ptr<State> PolicyRunSampler::compute_successor(const State& state, const ActionLabel_type action_label) const {
/**
 * Wrapper for successor computation to switch between policy-run sampling and normal successor generation.
 * Policy-run sampling only takes affect w/ probability delta on non-deterministic actions.
 */
    auto successors = simulatorEnv->compute_successors(state, action_label);
    if (PLAJA_GLOBAL::rng->prob() >= rand_prob && successors.size() > 1) { // sample with prob on nd steps
        return fuzzer->get_policy_run_successor(successors);
    }
    return simulatorEnv->compute_successor_if_applicable(state, action_label);
}

StateID_type PolicyRunSampler::uniform_sampling(StateID_type state, const ActionLabel_type action) const {
    /* Uniformly choose a successor rather than based on transition probabilities */
    auto successors = simulatorEnv->compute_successors(simulatorEnv->get_state(state), action);
    auto index = PLAJA_GLOBAL::rng->index(successors.size());
    return successors[index];
}

std::pair<PolicySampled::Path, PolicySampled::Stats> PolicyRunSampler::sample_path(StateID_type state_id) {
    /**
     * Sample a single path from the start state. 
     * PolicySampled::Stats keeps track of which termination condition was triggered.
     */
    std::vector<StateID_type> path = {};
    std::vector<ActionLabel_type> actions = {};
    auto current_state_id = state_id;

    PolicySampled::Path final_path;
    PolicySampled::Stats ps;
    for (auto t = 0; t < maxLengthEpisode; ++t) {
        /* Get action and compute successor*/
        const ActionLabel_type action_label = get_policy_action(simulatorEnv->get_state(current_state_id));
        PLAJA_ASSERT(simulatorEnv->is_applicable(simulatorEnv->get_state(current_state_id), action_label));
        std::unique_ptr<State> succ_state;
        succ_state = compute_successor(simulatorEnv->get_state(current_state_id), action_label); // Fuzzing by distance function
        path.push_back(current_state_id);
        actions.push_back(action_label);
        /* Early Termination checks */
        if (check_goal(*succ_state)) {
            searchStatistics->inc_attr_unsigned(PLAJA::StatsUnsigned::DONE_GOAL);
            final_path = {path, actions};
            ps =  {1, 0, 0, 0, 1};
            return {final_path, ps};
        }
        if (check_terminal(*succ_state)) {
            searchStatistics->inc_attr_unsigned(PLAJA::StatsUnsigned::TERMINAL_STATES);
            final_path = {path, actions};
            ps = {0, 0, 0, 1, 1};
            return {final_path, ps};
        }
        if (check_cycle(succ_state->get_id_value(), path)) {
            searchStatistics->inc_attr_unsigned(PLAJA::StatsUnsigned::DETECTED_CYCLES);
            final_path = {path, actions};
            ps = {0, 0, 1, 0, 1};
            return {final_path, ps};
        }
        if (check_avoid(*succ_state)) {
            searchStatistics->inc_attr_unsigned(PLAJA::StatsUnsigned::DONE_AVOID);
            path.push_back(succ_state->get_id_value());
            final_path = {path, actions};
            ps = {0, 1, 0, 0, 1};
            return{final_path, ps};
        }
        current_state_id = succ_state->get_id_value(); // Extend policy path and continue.
    }
    searchStatistics->inc_attr_unsigned(PLAJA::StatsUnsigned::UNDONE);
    final_path = {path, actions};
    ps = {0, 0, 0, 1, 1};
    return {final_path, ps};
}


PolicySampled::Stats PolicyRunSampler::enumerate_all_paths(StateID_type state, std::vector<StateID_type> path) {
    /* Enumerate all policy runs under a given state. This is extremely expensive to do due to action outcome nondeterminism. */
    PolicySampled::Stats ps{0,0,0,0,0};
    /* Early Termination */
    if (check_goal(simulatorEnv->get_state(state))) return {1, 0, 0, 0, 1};
    if (check_avoid(simulatorEnv->get_state(state))) return {0, 1, 0, 0, 1};
    if (check_cycle(state, path)) return {0, 0, 1, 0, 1};
    if (check_terminal(simulatorEnv->get_state(state))) return {0, 0, 0, 1, 1};

    auto new_path = path;
    new_path.push_back(state);
    auto action = get_policy_action(simulatorEnv->get_state(state));
    auto successors = simulatorEnv->compute_successors(simulatorEnv->get_state(state), action);
    for (auto succ : successors) {
        auto succ_ps = enumerate_all_paths(succ, new_path); // Recurse on all successors
        ps.goal_count += succ_ps.goal_count;
        ps.avoid_count += succ_ps.avoid_count;
        ps.terminal_count += succ_ps.terminal_count;
        ps.cycle_count += succ_ps.cycle_count;
        ps.num_paths += succ_ps.num_paths;
    }

    return ps;
}

bool PolicyRunSampler::search_policy_envelope(std::unique_ptr<State> state) {
    /* Compute the policy envelope until the first failed state is found. */

    // Initialize depth first search
    std::stack<StateID_type> stateStack;
    std::set<StateID_type> visited;
    stateStack.push(state->get_id_value());
    bool found_unsafe_execution = false;

    while (!(stateStack.empty() || found_unsafe_execution)) {
        auto s = stateStack.top();
        stateStack.pop();
        visited.insert(s); // Add state to visited set

        /* Early Termination Criteria */
        if (check_avoid(simulatorEnv->get_state(s))) {
            found_unsafe_execution = true;
            continue;
        }
        if (check_goal(simulatorEnv->get_state(s))) {
            continue;
        }
        if (check_terminal(simulatorEnv->get_state(s))) {
            continue;
        }

        // Expand state to visit all non-deterministic successors.
        auto action = get_policy_action(simulatorEnv->get_state(s));
        auto successors = simulatorEnv->compute_successors(simulatorEnv->get_state(s), action);
        for (auto succ : successors) {
            if (visited.count(succ) > 0) { // skip all states that have already been visited
                continue;
            }
            stateStack.push(succ);
        }
    }

    // found_unsafe_execution true when unsafe path is found from state
    if (found_unsafe_execution) {
        searchStatistics->inc_attr_unsigned(PLAJA::StatsUnsigned::DONE_AVOID, 1);
    } else {
        searchStatistics->inc_attr_unsigned(PLAJA::StatsUnsigned::DONE_GOAL, 1);
    }
    return found_unsafe_execution;
}

int PolicyRunSampler::search_envelope_for_fail_states(std::vector<StateID_type> startStates) {
    int num_unsafe_states = 0;

    // Cache: state -> can reach unsafe?
    std::unordered_map<StateID_type, bool> unsafe_cache;

    for (auto start : startStates) {

        // If already computed, reuse result
        if (unsafe_cache.count(start)) {
            if (unsafe_cache[start])
                num_unsafe_states++;
            continue;
        }

        std::stack<StateID_type> stateStack;
        std::unordered_set<StateID_type> local_visited;

        stateStack.push(start);
        bool found_unsafe_execution = false;

        while (!stateStack.empty()) {

            auto s = stateStack.top();
            stateStack.pop();

            if (local_visited.count(s))
                continue; // cycles are safe

            local_visited.insert(s);

            // If already classified globally, reuse result
            if (unsafe_cache.count(s)) {
                if (unsafe_cache[s]) {
                    found_unsafe_execution = true; // proved this state is unsafe before, no need to continue searching this start state
                    break;
                }
                continue; // proved this state is safe before
            }

            auto state = simulatorEnv->get_state(s);

            /* Early termination checks */
            if (check_avoid(state)) {
                unsafe_cache[s] = true;
                found_unsafe_execution = true;
                break;
            }

            if (check_goal(state) || check_terminal(state)) {
                unsafe_cache[s] = false;
                continue;
            }

            auto action = get_policy_action(state);
            auto successors = simulatorEnv->compute_successors(state, action);

            for (auto succ : successors) {
                stateStack.push(succ);
            }
        }

        // Cache result for all visited states
        for (auto v : local_visited) {
            unsafe_cache[v] = found_unsafe_execution;
        }

        if (found_unsafe_execution)
            num_unsafe_states++;
    }

    return num_unsafe_states;
}

bool PolicyRunSampler::is_state_safe_under_policy(
    StateID_type s,
    std::set<StateID_type>& visiting,
    std::unordered_map<StateID_type, bool>& memo,
    std::set<StateID_type>& safe_states)
{
    // Memoized result
    if (memo.count(s)) return memo[s];

    auto state = simulatorEnv->get_state(s);

    // Unsafe state
    if (check_avoid(state)) {
        memo[s] = false;
        return false;
    }

    // Goal or terminal: safe but do not collect
    if (check_goal(state) || check_terminal(state)) {
        memo[s] = true;
        return true;
    }

    // Cycle detection (treat cycles as safe unless proven otherwise)
    if (visiting.count(s)) return true;

    visiting.insert(s);

    auto action = get_policy_action(state);
    auto successors = simulatorEnv->compute_successors(state, action);

    for (auto succ : successors) {
        if (!is_state_safe_under_policy(succ, visiting, memo, safe_states)) {
            memo[s] = false;
            visiting.erase(s);
            return false;
        }
    }

    visiting.erase(s);

    // If all successors are safe → this state is safe
    memo[s] = true;
    safe_states.insert(s);

    return true;
}


std::set<StateID_type> PolicyRunSampler::get_safe_states_in_policy_envelope(StateID_type start) {
    /* Compute policy envelope till first fail state is found */
    {
    std::set<StateID_type> visiting;
    std::unordered_map<StateID_type, bool> memo;
    std::set<StateID_type> safe_states;

    is_state_safe_under_policy(start, visiting, memo, safe_states);

    return safe_states;
}
}



std::optional<PolicySampled::Path> PolicyRunSampler::get_unsafe_path_from_logs(const std::string& filename) {
    std::ifstream in(filename);
    json j;
    in >> j;
    std::vector<StateValues> unsafe_path;
    PLAJA_ASSERT(j.contains("path") && j["path"].is_array())
    for (const auto& state_json : j["path"]) {

        std::vector<PLAJA::integer> ints;
        std::vector<PLAJA::floating> floats;
        ints.push_back(0); // For the automaton

        for (const auto& val : state_json) {
            if (val.is_number_integer()) {
                ints.push_back(val.get<int>());
            } else if (val.is_number_float()) {
                floats.push_back(val.get<double>());
            }
        }

        StateValues state = StateValues(ints, floats);
        unsafe_path.push_back(state);
    }
    std::optional<PolicySampled::Path> path = get_path_from_state_values(unsafe_path);
    return path;
}

#ifdef BUILD_PA
std::optional<PolicySampled::Path> PolicyRunSampler::run_verification(const Expression& start, const Expression& unsafety) {
    PUSH_LAP(searchStatistics, PLAJA::StatsDouble::TOTAL_VERIFICATION_TIME);
        pa_cegar->search();
        POP_LAP(searchStatistics, PLAJA::StatsDouble::TOTAL_VERIFICATION_TIME);
        if (pa_cegar->is_safe) {
            return std::nullopt;
        }
        if (pa_cegar->get_status() == SearchEngine::FINISHED) {
            PLAJA_LOG("Extracting unsafe path ... ")
            std::vector<StateValues> unsafe_path = pa_cegar->getUnsafePath();
            std::optional<PolicySampled::Path> path = get_path_from_state_values(unsafe_path);
            return path;
        }
        PLAJA_LOG(PLAJA_UTILS::to_red_string("PA CEGAR TERMINATED WITHOUT SOLVING"))
    PLAJA_ABORT
    return std::nullopt;
}

void PolicyRunSampler::init_pa_cegar(const Expression& start) {
        // MODEL_Z3 initialized in InitialStateEnumerator and shared however pa_cegar requires a MODELZ3PA model
        // therefore the config is copied and we delete the model z3 from the shared objects.
        auto subconfig(config);
        const auto model = PLAJA_GLOBAL::currentModel;
        sub_prop_info = PropertyInformation::analyse_property(*model->get_property(1), *model);
        sub_prop_info->set_start(&start);
        subconfig.delete_sharable(PLAJA::SharableKey::MODEL_Z3);
        subconfig.delete_sharable(PLAJA::SharableKey::PROP_INFO);
        subconfig.set_sharable_const(PLAJA::SharableKey::PROP_INFO,sub_prop_info.get());
        pa_cegar = std::make_unique<PACegar>(PLAJA_UTILS::cast_ref<SearchEngineConfigPA>(subconfig));
}
#endif

PolicySampled::Path PolicyRunSampler::get_path_from_state_values(std::vector<StateValues> state_values) const {
    std::vector<StateID_type> states;
    std::vector<ActionLabel_type> actions;
    for (auto s : state_values) {
        states.push_back(simulatorEnv->get_state(s).get_id());
        actions.push_back(get_policy_action(simulatorEnv->get_state(s)));
    }
    return {states, actions};
}
