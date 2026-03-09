#include "reduction_agent.h"
#include "../exception/plaja_exception.h"
#include "../parser/ast/expression/non_standard/objective_expression.h"
#include "../parser/ast/model.h"
#include "../search/factories/policy_learning/ql_options.h"
#include "../search/factories/policy_reduction/reduction_options.h"
#include "../search/factories/configuration.h"
#include "../search/fd_adaptions/timer.h"
#include "../search/fd_adaptions/state.h"
#include "../search/information/jani2nnet/jani_2_nnet.h"
#include "../search/information/property_information.h"
#include "../search/non_prob_search/initial_states_enumerator.h"
#include "../search/successor_generation/simulation_environment.h"
#include "../utils/rng.h"
#include "../globals.h"
#include "policy_teacher.h"
#include "ql_agent_torch.h"
#include "reduction_statistics.h"
#include "../option_parser/plaja_options.h"


namespace REDUCTION_AGENT {

    extern std::unique_ptr<SearchEngine> construct(const PLAJA::Configuration& config);

    std::unique_ptr<SearchEngine> construct(const PLAJA::Configuration& config) { return std::make_unique<ReductionAgent>(config); }

}

ReductionAgent::ReductionAgent(const PLAJA::Configuration& config):
    SearchEngine(config),
    numEpisodes(config.get_int_option(PLAJA_OPTION::num_episodes)),
    maxLengthEpisode(config.get_int_option(PLAJA_OPTION::max_length_episode)),
    numStartStates(config.get_int_option(PLAJA_OPTION::num_start_states)),
    reductionThreshold(config.get_double_option(PLAJA_OPTION::reduction_threshold)),
    frequency(config.get_double_option(PLAJA_OPTION::frequency)),
    compact(config.is_flag_set(PLAJA_OPTION::compact)),
    saveNNet(nullptr),
    savePolicyPath(config.has_value_option(PLAJA_OPTION::save_policy_path) ? new std::string(config.get_value_option_string(PLAJA_OPTION::save_policy_path)) : nullptr),
    postfix(config.has_value_option(PLAJA_OPTION::postfix) ? new std::string(config.get_value_option_string(PLAJA_OPTION::postfix)) : nullptr),
    saveBackup(config.has_value_option(PLAJA_OPTION::save_backup) ? new std::string(config.get_value_option_string(PLAJA_OPTION::save_backup)) : nullptr),
    saveIntermediateStats(config.is_flag_set(PLAJA_OPTION::save_intermediate_stats)),
    episodesCounter(0),
    subEpisodesCounter(0),
    goalCounter(0),
    reductionStats(new ReductionStatistics()),
    startEnumerator(propertyInfo->get_start() ? new InitialStatesEnumerator(config, *propertyInfo->get_start()) : nullptr),
    cycleDetected(false) {
    reductionStats->add_basic_stats(*searchStatistics);
    PLAJA_ASSERT(propertyInfo->get_learning_objective())
    PLAJA_ASSERT(propertyInfo->get_reach())
    objective = propertyInfo->get_learning_objective();
    avoid = propertyInfo->get_reach();
    jani2NNet = propertyInfo->get_nn_interface();
    PLAJA_ASSERT(jani2NNet)
    simulatorEnv = std::make_unique<SimulationEnvironment>(config, *model);
    PLAJA_GLOBAL::randomizeNonDetEval = true; // For simulation.
    if (config.is_flag_set(PLAJA_OPTION::save_nnet)) { saveNNet = &jani2NNet->get_policy_file(); }
    startStates.push_back(simulatorEnv->get_initial_state().get_id_value());
    std::unordered_set<StateID_type> start_states;
    start_states.insert(startStates.back());
    searchStatistics->inc_attr_unsigned(PLAJA::StatsUnsigned::START_STATES);
    if (startEnumerator) {
        startEnumerator->initialize();
        if (not startEnumerator->samples()) {
            for (const auto& start_state: startEnumerator->enumerate_states()) {
                const auto start_id = simulatorEnv->get_state(start_state).get_id_value();
                if (start_states.insert(start_id).second) {
                    startStates.push_back(start_id);
                    searchStatistics->inc_attr_unsigned(PLAJA::StatsUnsigned::START_STATES);
                }
            }
            startEnumerator = nullptr;
        } else if (startStates.size() < numStartStates) {
            while (startStates.size() < numStartStates) {
                auto start_sample = startEnumerator->sample_state();
                const auto start_state_id = simulatorEnv->get_state(*start_sample).get_id_value();
                startStates.push_back(start_state_id);
                searchStatistics->inc_attr_unsigned(PLAJA::StatsUnsigned::START_STATES);
            }
            startEnumerator = nullptr;
        } else {
            PLAJA_ASSERT(propertyInfo->get_start())
            if (propertyInfo->get_start()->evaluate_integer(simulatorEnv->get_initial_state())) { startStates.clear(); }
        }
    }
    std::vector<TORCH_IN_PLAJA::Floating> lower_bounds;
    std::vector<TORCH_IN_PLAJA::Floating> upper_bounds;
    {
        const auto& model_info = model->get_model_information();
        const auto l = jani2NNet->get_num_input_features();
        lower_bounds.reserve(l);
        upper_bounds.reserve(l);
        for (std::size_t i = 0; i < l; ++i) {
            const VariableIndex_type state_index = jani2NNet->get_input_feature_index(i);
            lower_bounds.push_back(PLAJA_UTILS::cast_numeric<TORCH_IN_PLAJA::Floating>(model_info.get_lower_bound(state_index)));
            upper_bounds.push_back(PLAJA_UTILS::cast_numeric<TORCH_IN_PLAJA::Floating>(model_info.get_upper_bound(state_index)));
        }
    }
    agent = std::make_unique<QlAgentTorch>(jani2NNet->get_layer_sizes(), reductionThreshold, lower_bounds, upper_bounds, false, 0, 0, 0, 0);
    agent->eval();
    const auto& nn_to_load = config.get_value_option_string(PLAJA_OPTION::load_nn);
    if (PLAJA_UTILS::file_exists(nn_to_load)) { load_nn(nn_to_load, nn_to_load); }
    else {
        std::cout << "Warning: specified invalid path to load existing NN ... trying to load NN as specified by loaded interface ..." << std::endl;
        load_nn(propertyInfo->get_nn_interface()->get_torch_file(), propertyInfo->get_nn_interface()->get_interface_file()); // property_info to not use "save_external" path
    }
}

ReductionAgent::~ReductionAgent() = default;

void ReductionAgent::load_nn(const std::string& torch_file, const std::string& nnet_file) {
    try { agent->load(torch_file); } // try to load PyTorch model ...
    catch (PlajaException& e) {
        PLAJA_LOG(e.what())
        agent->load_from_nnet(nnet_file); // try to load NNet ...
    }
}

void ReductionAgent::save_copy(const std::string& prefix) {
    std::string file_name;
    if (postfix != nullptr) {
        file_name = prefix + PLAJA_UTILS::extract_filename(jani2NNet->get_torch_file(), false) + *postfix + PLAJA_UTILS::extract_extension(jani2NNet->get_torch_file());
    } else {
        file_name = prefix + PLAJA_UTILS::extract_filename(jani2NNet->get_torch_file(), true);
    }
    auto torch_file = PLAJA_UTILS::join_path(PLAJA_UTILS::extract_directory_path(jani2NNet->get_torch_file()), { file_name });
    if (savePolicyPath != nullptr) {
        torch_file =  PLAJA_UTILS::join_path(*savePolicyPath, { file_name });
    }
    if (saveNNet) {
        if (postfix != nullptr) {
            file_name = prefix + PLAJA_UTILS::extract_filename(jani2NNet->get_policy_file(), false) + *postfix + PLAJA_UTILS::extract_extension(jani2NNet->get_torch_file());
        } else {
            file_name = prefix + PLAJA_UTILS::extract_filename(jani2NNet->get_policy_file(), true);
        }
        auto nnet_file = PLAJA_UTILS::join_path(PLAJA_UTILS::extract_directory_path(jani2NNet->get_interface_file()), { file_name });
        if (savePolicyPath != nullptr) {
            nnet_file = PLAJA_UTILS::join_path(*savePolicyPath, { file_name });
        }
        agent->save(&torch_file, &nnet_file, saveBackup.get());
    } else {
        agent->save(&torch_file, nullptr, saveBackup.get());
    }
}

inline bool ReductionAgent::check_goal(const State& state) const {
    auto* goal = objective->get_goal();
    return goal and goal->evaluateInteger(&state);
}

inline bool ReductionAgent::check_avoid(const State& state) const {
    PLAJA_ASSERT(avoid)
    return avoid->evaluateInteger(&state);
}

inline void ReductionAgent::set_current_state(const State& state) {
    currentState = state.to_ptr();
    USING_POLICY_LEARNING::NNState& current_nn_state = agent->currentState;
    const auto l = current_nn_state.size();
    PLAJA_ASSERT(l == jani2NNet->get_num_input_features())
    for (std::size_t i = 0; i < l; ++i) {
        current_nn_state[i] = PLAJA_UTILS::cast_numeric<TORCH_IN_PLAJA::Floating>(state[jani2NNet->get_input_feature_index(i)]);
    }
    agent->set_current_state_tensor();
}

inline void ReductionAgent::set_next_state(const State& state) {
    nextState = state.to_ptr();
    USING_POLICY_LEARNING::NNState& next_nn_state = agent->nextState;
    const auto l = next_nn_state.size();
    PLAJA_ASSERT(l == jani2NNet->get_num_input_features())
    for (std::size_t i = 0; i < l; ++i) {
        next_nn_state[i] = PLAJA_UTILS::cast_numeric<TORCH_IN_PLAJA::Floating>(state[jani2NNet->get_input_feature_index(i)]);
    }
}

inline void ReductionAgent::set_next_state_applicability() {
    // next state applicability
    if (jani2NNet->_do_applicability_filtering()) {
        std::list<USING_POLICY_LEARNING::NNAction_type>& next_state_app = agent->nextStateApplicability;
        next_state_app.clear();
        for (const ActionLabel_type action_label: cachedApplicableActions) {
            PLAJA_ASSERT(jani2NNet->is_learned(action_label))
            next_state_app.push_back(jani2NNet->get_output_index(action_label));
        }
    }
}

inline void ReductionAgent::set_not_done(const State& state) {
    set_next_state(state);
    set_next_state_applicability();
    agent->set_not_done(0);
}

inline void ReductionAgent::set_stalling(bool terminal) {
    nextState = nullptr;
    agent->set_stalling(0);
}

inline void ReductionAgent::set_goal(const State& state) {
    set_next_state(state);
    agent->set_goal(0);
}

inline void ReductionAgent::set_avoid(const State& state) {
    set_next_state(state);
    agent->set_avoid(0);
}

inline void ReductionAgent::set_next_to_current_state() {
    currentState = std::move(nextState);
    std::swap(agent->currentState, agent->nextState);
    agent->set_current_state_tensor();
}

ActionLabel_type ReductionAgent::act() {
    PLAJA_ASSERT(not cachedApplicableActions.empty())
    searchStatistics->inc_attr_unsigned(PLAJA::StatsUnsigned::POLICY_ACTS);
    if (jani2NNet->_do_applicability_filtering()) {
        USING_POLICY_LEARNING::NNAction_type current_nn_action = agent->rank_actions();
        ActionLabel_type current_action_label = jani2NNet->get_output_label(current_nn_action);
        if (simulatorEnv->is_cached_applicable(current_action_label)) {
            return current_action_label;
        }
        searchStatistics->inc_attr_unsigned(PLAJA::StatsUnsigned::SUB_RANK_ACTS);
        const auto num_ranks = jani2NNet->get_num_output_features();
        for (long int rank = 1; rank < num_ranks; ++rank) {
            current_nn_action = agent->retrieve_ranked_action(rank);
            current_action_label = jani2NNet->get_output_label(current_nn_action);
            if (simulatorEnv->is_cached_applicable(current_action_label)) {
                return current_action_label;
            }
        }
        PLAJA_ASSERT(cachedApplicableActions.empty())
        PLAJA_ABORT
    }
    auto current_action_label = jani2NNet->get_output_label(agent->act());
    return current_action_label;
}

std::unique_ptr<State> ReductionAgent::simulate(const State& state, ActionLabel_type action_label) {
    auto successor_state = simulatorEnv->compute_successor_if_applicable(state, action_label);
    searchStatistics->inc_attr_unsigned(PLAJA::StatsUnsigned::ACTION_LABELS);
    searchStatistics->inc_attr_unsigned(PLAJA::StatsUnsigned::EXPANDED_STATES);
    if (not successor_state) {
        searchStatistics->inc_attr_unsigned(PLAJA::StatsUnsigned::DONE_STALLING);
        set_stalling(false);
        return successor_state;
    } else {
        searchStatistics->inc_attr_unsigned(PLAJA::StatsUnsigned::GENERATED_STATES);
        searchStatistics->inc_attr_unsigned(PLAJA::StatsUnsigned::ACTION_OPS);
        searchStatistics->inc_attr_unsigned(PLAJA::StatsUnsigned::TRANSITIONS);
    }
    if (check_goal(*successor_state)) {
        searchStatistics->inc_attr_unsigned(PLAJA::StatsUnsigned::GOAL_STATES);
        searchStatistics->inc_attr_unsigned(PLAJA::StatsUnsigned::DONE_GOAL);
        goalCounter++;
        set_goal(*successor_state);
        return successor_state;
    }
    if (check_avoid(*successor_state)) {
        searchStatistics->inc_attr_unsigned(PLAJA::StatsUnsigned::DONE_AVOID);
        set_avoid(*successor_state);
        return successor_state;
    }
    cachedApplicableActions = simulatorEnv->extract_applicable_actions(*successor_state, true);
    while (cachedApplicableActions.size() <= 1) {
        if (cachedApplicableActions.empty()) {
            searchStatistics->inc_attr_unsigned(PLAJA::StatsUnsigned::TERMINAL_STATES);
            searchStatistics->inc_attr_unsigned(PLAJA::StatsUnsigned::DONE_STALLING);
            set_stalling(true);
            return successor_state;
        }
        const ActionLabel_type next_action = cachedApplicableActions[0];
        if (jani2NNet->is_learned(next_action)) { break; }
        PLAJA_ASSERT(next_action == ACTION::silentAction)
        successor_state = simulatorEnv->compute_successor(*successor_state, next_action);
        searchStatistics->inc_attr_unsigned(PLAJA::StatsUnsigned::GENERATED_STATES);
        searchStatistics->inc_attr_unsigned(PLAJA::StatsUnsigned::ACTION_OPS);
        searchStatistics->inc_attr_unsigned(PLAJA::StatsUnsigned::TRANSITIONS);
        if (check_goal(*successor_state)) {
            searchStatistics->inc_attr_unsigned(PLAJA::StatsUnsigned::GOAL_STATES);
            searchStatistics->inc_attr_unsigned(PLAJA::StatsUnsigned::DONE_GOAL);
            set_goal(*successor_state);
            return successor_state;
        }
        if (check_avoid(*successor_state)) {
            searchStatistics->inc_attr_unsigned(PLAJA::StatsUnsigned::DONE_AVOID);
            set_avoid(*successor_state);
            return successor_state;
        }
        cachedApplicableActions = simulatorEnv->extract_applicable_actions(*successor_state, true);
    }
    set_not_done(*successor_state);
    return successor_state;
}

std::unique_ptr<State> ReductionAgent::simulate_until_choice_point(const State& state) {
    std::unique_ptr<State> successor_state = state.to_ptr();
    cachedApplicableActions = simulatorEnv->extract_applicable_actions(*successor_state, true);
    while (cachedApplicableActions.size() <= 1) {
        if (cachedApplicableActions.empty()) {
            searchStatistics->inc_attr_unsigned(PLAJA::StatsUnsigned::TERMINAL_STATES);
            return nullptr;
        }
        const ActionLabel_type next_action = cachedApplicableActions[0];
        if (jani2NNet->is_learned(next_action)) { break; }
        PLAJA_ASSERT(next_action == ACTION::silentAction)
        successor_state = simulatorEnv->compute_successor(*successor_state, next_action);
        searchStatistics->inc_attr_unsigned(PLAJA::StatsUnsigned::GENERATED_STATES);
        searchStatistics->inc_attr_unsigned(PLAJA::StatsUnsigned::ACTION_OPS);
        searchStatistics->inc_attr_unsigned(PLAJA::StatsUnsigned::TRANSITIONS);
        if (check_goal(*successor_state)) {
            searchStatistics->inc_attr_unsigned(PLAJA::StatsUnsigned::GOAL_STATES);
            return nullptr;
        }
        if (check_avoid(*successor_state)) { return nullptr; }
        cachedApplicableActions = simulatorEnv->extract_applicable_actions(*successor_state, true);
    }
    return successor_state;
}

State ReductionAgent::sample_start_state() {
    searchStatistics->inc_attr_unsigned(PLAJA::StatsUnsigned::GENERATED_STATES);
    const StateID_type start_id = startStates[subEpisodesCounter % startStates.size()];
    return simulatorEnv->get_state(start_id);

}

SearchEngine::SearchStatus ReductionAgent::initialize() { return SearchStatus::IN_PROGRESS; }

SearchEngine::SearchStatus ReductionAgent::step() {
    if (episodesCounter >= numEpisodes) {
        save_copy("final_");
        return SearchStatus::SOLVED;
    } else if (saveIntermediateStats) {
        searchStatistics->reset();
        reductionStats->reset();
    }
    for (unsigned int i = 0; i < startStates.size(); i++) {
        std::unique_ptr<State> current_state = simulate_until_choice_point(sample_start_state());
        if (not current_state) {
            static bool print_warning(true);
            PLAJA_LOG_IF(print_warning, PLAJA_UTILS::to_red_string("Terminal start state."))
            print_warning = false;
            continue;
        } else {
            ++subEpisodesCounter;
        }
        cycleDetected = false;
        cachedTransitionsPerSubEpisode.clear();
        agent->set_not_done(0);
        set_current_state(*current_state);
        auto j = 0;
        for (; j < maxLengthEpisode and not cycleDetected and not agent->_done(); ++j) {
            PLAJA_ASSERT(current_state)
            const ActionLabel_type action_label = act();
            current_state = simulate(*current_state, action_label);
            if (!cachedTransitionsPerSubEpisode.insert({ currentState->get_id_value(), action_label, nextState ? nextState->get_id_value() : STATE::none }).second) {
                cycleDetected = true;
            }
            set_next_to_current_state();
        }
        if (not agent->_done()) { searchStatistics->inc_attr_unsigned(PLAJA::StatsUnsigned::UNDONE); }
        if (cycleDetected) { searchStatistics->inc_attr_unsigned(PLAJA::StatsUnsigned::DETECTED_CYCLES); }
    }
    subEpisodesCounter = 0;
    episodesCounter++;
    for (const auto& [layer, neurons] : agent->reduce(frequency, compact)) {
        reductionStats->set_reduced_neurons_map(layer);
        reductionStats->inc_layer_reduced_neurons(layer, neurons);
    }
    searchStatistics->set_attr_double(PLAJA::StatsDouble::TRAIN_GOAL_FRACTION, ((double) goalCounter) / ((double) startStates.size()));
    goalCounter = 0;
    searchStatistics->inc_attr_unsigned(PLAJA::StatsUnsigned::EPISODES);
    if (saveIntermediateStats) {
        trigger_intermediate_stats();
    }
    return SearchStatus::IN_PROGRESS;
}

void ReductionAgent::print_statistics() const {
    searchStatistics->print_statistics();
    reductionStats->print_statistics();
}

void ReductionAgent::stats_to_csv(std::ofstream& file) const {
    searchStatistics->set_attr_unsigned(PLAJA::StatsUnsigned::EPISODES, episodesCounter);
    searchStatistics->stats_to_csv(file);
    reductionStats->stats_to_csv(file);
}

void ReductionAgent::stat_names_to_csv(std::ofstream& file) const {
    searchStatistics->stat_names_to_csv(file);
    reductionStats->stat_names_to_csv(file);
}


