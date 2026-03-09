//
// This file is part of the PlaJA code base.
// Copyright (c) (2019 - 2021) Marcel Vinzent.
// See README.md in the top-level directory for licensing information.
//


#include "ql_agent.h"
#include "../search/smt/bias_functions/distance_function.h"
#include "../exception/plaja_exception.h"
#include "../globals.h"
#include "../parser/ast/expression/non_standard/objective_expression.h"
#include "../parser/ast/model.h"
#include "../parser/visitor/to_normalform.h"
#include "../search/factories/policy_learning/ql_options.h"
#include "../search/factories/configuration.h"
#include "../search/fd_adaptions/state.h"
#include "../search/fd_adaptions/timer.h"
#include "../search/information/jani2nnet/jani_2_nnet.h"
#include "../search/information/jani_2_interface.h"
#include "../search/information/jani2ensemble/jani_2_ensemble.h"
#include "../search/information/model_information.h"
#include "../search/information/property_information.h"
#include "../search/non_prob_search/initial_states_enumerator.h"
#include "../search/smt/visitor/to_z3_visitor.h"
#include "../search/successor_generation/simulation_environment.h"
#include "../utils/rng.h"
#include "learning_statistics.h"
#include "policy_teacher.h"
#include "ql_agent_torch.h"
#include "slicing_window.h"

#include <cmath>
#include <numeric>
#include <torch/serialize.h>


namespace QL_AGENT {

    extern std::unique_ptr<SearchEngine> construct(const PLAJA::Configuration& config);

    std::unique_ptr<SearchEngine> construct(const PLAJA::Configuration& config) { return std::make_unique<QlAgent>(config); }

}// namespace QL_AGENT

QlAgent::QlAgent(const PLAJA::Configuration& config):
    SearchEngine(config),
    evaluationMode(config.is_flag_set(PLAJA_OPTION::evaluation_mode)),
    numEpisodes(config.get_int_option(PLAJA_OPTION::num_episodes)),
    maxStartStates(config.get_int_option(PLAJA_OPTION::max_start_states)),
    maxLengthEpisode(config.get_int_option(PLAJA_OPTION::max_length_episode)),
    epsilon(evaluationMode ? 0 : config.get_double_option(PLAJA_OPTION::epsilon_start)),
    epsilonDecay(config.get_double_option(PLAJA_OPTION::epsilon_decay)),
    epsilonEnd(evaluationMode ? 0 : config.get_double_option(PLAJA_OPTION::epsilon_end)),
    grad_clip(config.get_bool_option(PLAJA_OPTION::grad_clip)),
    l1_lambda(config.get_double_option(PLAJA_OPTION::l1_lambda)),
    l2_lambda(config.get_double_option(PLAJA_OPTION::l2_lambda)),
    group_lambda(config.get_double_option(PLAJA_OPTION::group_lambda)),
    kl_lambda(config.get_double_option(PLAJA_OPTION::kl_lambda)),
    reward_lambda(config.get_double_option(PLAJA_OPTION::reward_lambda)),
    goalReward(PLAJA_UTILS::cast_numeric<USING_POLICY_LEARNING::Reward_type>(config.get_double_option(PLAJA_OPTION::goal_reward))),
    avoidReward(PLAJA_UTILS::cast_numeric<USING_POLICY_LEARNING::Reward_type>(config.get_double_option(PLAJA_OPTION::avoid_reward))),
    stallingReward(PLAJA_UTILS::cast_numeric<USING_POLICY_LEARNING::Reward_type>(config.get_double_option(PLAJA_OPTION::stalling_reward))),
    terminalReward(PLAJA_UTILS::cast_numeric<USING_POLICY_LEARNING::Reward_type>(config.get_double_option(PLAJA_OPTION::terminal_reward))),
    #ifdef BUILD_FAULT_ANALYSIS
    unsafeReward(PLAJA_UTILS::cast_numeric<USING_POLICY_LEARNING::Reward_type>(config.get_double_option(PLAJA_OPTION::unsafe_reward))),
    #endif
    terminateCycles(config.is_flag_set(PLAJA_OPTION::terminate_cycles)),
    saveNNet(nullptr),
    savePolicyPath(config.has_value_option(PLAJA_OPTION::save_policy_path) ? new std::string(config.get_value_option_string(PLAJA_OPTION::save_policy_path)) : nullptr),
    rewardNNPath(config.has_value_option(PLAJA_OPTION::reward_nn) ? new std::string(config.get_value_option_string(PLAJA_OPTION::reward_nn)) : nullptr),
    rewardNNLayers(config.has_value_option(PLAJA_OPTION::reward_nn_layers) ? new std::string(config.get_value_option_string(PLAJA_OPTION::reward_nn_layers)) : nullptr),
    postfix(config.has_value_option(PLAJA_OPTION::postfix) ? new std::string(config.get_value_option_string(PLAJA_OPTION::postfix)) : nullptr),
    saveBackup(config.has_value_option(PLAJA_OPTION::save_backup) ? new std::string(config.get_value_option_string(PLAJA_OPTION::save_backup)) : nullptr),
    saveAgentActions(config.has_value_option(PLAJA_OPTION::save_agent_actions) ? new std::string(config.get_value_option_string(PLAJA_OPTION::save_agent_actions)) : nullptr),
    episodesCounter(0), 
    bestScore(config.get_double_option(PLAJA_OPTION::minimal_score)),
    learningStats(new LearningStatistics()),
    scoreWindow(evaluationMode ? nullptr : new SlicingWindow(config.get_int_option(PLAJA_OPTION::slicingSize))),
    episodeLength(evaluationMode ? nullptr : new SlicingWindow(config.get_int_option(PLAJA_OPTION::slicingSize))),
    startEnumerator(propertyInfo->get_start() ? new InitialStatesEnumerator(config, *propertyInfo->get_start()) : nullptr),
    groundDonePotential(config.get_bool_option(PLAJA_OPTION::groundDonePotential)),
    policyTeacher(nullptr),
    teacherGreediness(0),
    rng(PLAJA_GLOBAL::rng.get()),
    teacherEpisode(false),
    cycleDetected(false),
    ensemble(config.has_value_option(PLAJA_OPTION::load_ensemble) ?
    ([&]() {
            auto filename = PLAJA_GLOBAL::optionParser->get_value_option_string(PLAJA_OPTION::load_ensemble);
            std::ifstream jsonFile(filename);
            std::string jsonString((std::istreambuf_iterator<char>(jsonFile)),
                                std::istreambuf_iterator<char>());
            std::istringstream iss(jsonString);
            return veritas::addtree_from_json<veritas::AddTree>(iss);
        }()) 
    : veritas::AddTree(0, veritas::AddTreeType::REGR)) {

    
    learningStats->add_basic_stats(*searchStatistics); // Statistics file
    
    /* Set property information like goal and failure condition*/
    PLAJA_ASSERT(propertyInfo->get_learning_objective())
    PLAJA_ASSERT(propertyInfo->get_reach())
    objective = propertyInfo->get_learning_objective();// objective is goal
    avoid = propertyInfo->get_reach();                 // reach is avoid, i.e., the safety property to check
    
    /* Load policy interface */
    jani2Interface = propertyInfo->get_interface();
    PLAJA_ASSERT(jani2Interface)
    simulatorEnv = std::make_shared<SimulationEnvironment>(config, *model); // create simulator
    PLAJA_GLOBAL::randomizeNonDetEval = true;// For simulation.
    for (OutputIndex_type index = 0; index < jani2Interface->get_num_output_features(); ++index) {
        learningStats->set_epsilon_greedy_selection(model->get_action_name(jani2Interface->get_output_label(index)));
    }
    if (config.is_flag_set(PLAJA_OPTION::save_nnet)) { saveNNet = &jani2Interface->get_policy_file(); }

    // start states
    // primary initial state from the model
    startStates.push_back(simulatorEnv->get_initial_state().get_id_value());
    std::unordered_set<StateID_type> start_states;
    start_states.insert(startStates.back());
    searchStatistics->inc_attr_unsigned(PLAJA::StatsUnsigned::START_STATES);
    /* How to get more start states */
    if (startEnumerator) {
        startEnumerator->initialize();
        if (not startEnumerator->samples()) { 
            /* Enumerate all start states in start condition */
            for (const auto& start_state: startEnumerator->enumerate_states()) {
                const auto start_id = simulatorEnv->get_state(start_state).get_id_value();
                if (start_states.insert(start_id).second) {
                    startStates.push_back(start_id); //
                    searchStatistics->inc_attr_unsigned(PLAJA::StatsUnsigned::START_STATES);
                }
            }
            startEnumerator = nullptr;
        } 
        else if (numEpisodes > maxStartStates) {
            /* Enumerate a fixed number of start states */
            while (startStates.size() < maxStartStates) {
                auto start_sample = startEnumerator->sample_state();
                const auto start_state_id = simulatorEnv->get_state(*start_sample).get_id_value();
                startStates.push_back(start_state_id);
                searchStatistics->inc_attr_unsigned(PLAJA::StatsUnsigned::START_STATES);
            }
            startEnumerator = nullptr;
        } 
        else {
            /* Clear start states for sampling during each step */
            PLAJA_ASSERT(propertyInfo->get_start())
            start = propertyInfo->get_start()->deepCopy_Exp();
            startStates.clear();
        }
    }

    if (evaluationMode) {
        scoreWindow = std::make_unique<SlicingWindow>(numEpisodes);
        episodeLength = std::make_unique<SlicingWindow>(numEpisodes);
    }

    // compute upper and lower bounds
    std::vector<TORCH_IN_PLAJA::Floating> lower_bounds;
    std::vector<TORCH_IN_PLAJA::Floating> upper_bounds;
    {
        const auto& model_info = model->get_model_information();
        const auto l = jani2Interface->get_num_input_features();
        lower_bounds.reserve(l);
        upper_bounds.reserve(l);
        for (std::size_t i = 0; i < l; ++i) {
            const VariableIndex_type state_index = jani2Interface->get_input_feature_index(i);
            lower_bounds.push_back(PLAJA_UTILS::cast_numeric<TORCH_IN_PLAJA::Floating>(model_info.get_lower_bound(state_index)));
            upper_bounds.push_back(PLAJA_UTILS::cast_numeric<TORCH_IN_PLAJA::Floating>(model_info.get_upper_bound(state_index)));
        }
    }

    // torch agent internal
    if (!config.has_value_option(PLAJA_OPTION::load_ensemble)) {
        auto jani2NNet = dynamic_cast<const Jani2NNet*>(jani2Interface);
        agent = std::make_unique<QlAgentTorch>(jani2NNet->get_layer_sizes(), 0.0, lower_bounds, upper_bounds, grad_clip, l1_lambda, l2_lambda, group_lambda, kl_lambda); // initialize for Neural network
    } else {
        agent = std::make_unique<QlAgentTorch>(jani2Interface->get_num_input_features(), lower_bounds, upper_bounds); // initialize for tree ensemble
    }

    // load existing model ...
    if (config.has_value_option(PLAJA_OPTION::load_nn)) {
        /* Load existing neural network model for learning */
        const auto& nn_to_load = config.get_value_option_string(PLAJA_OPTION::load_nn);
        if (PLAJA_UTILS::file_exists(nn_to_load)) {
            load_nn(nn_to_load, nn_to_load);
        } else {
            std::cout << "Warning: specified invalid path to load existing NN ... trying to load NN as specified by loaded interface ..." << std::endl;
            load_nn(propertyInfo->get_nn_interface()->get_torch_file(), propertyInfo->get_nn_interface()->get_interface_file());// property_info to not use "save_external" path
        }
    } else if (evaluationMode) { 
        if (config.has_value_option(PLAJA_OPTION::load_ensemble)) { // Load policies for evaluation
            /* Load existing tree ensemble model */
            std::cout << "Leafs: " << ensemble.num_leafs() << " Nodes: " << ensemble.num_nodes() << std::endl;
            treePolicy = true;
        } else {
            /* Load existing neural network model*/
            load_nn(propertyInfo->get_nn_interface()->get_torch_file(), propertyInfo->get_nn_interface()->get_interface_file());
        }
    }

    // if "load_nn" (from different position) or "save_nnet" is set, save also in eval mode (if file non-existing)
    if (evaluationMode) {
        if (not treePolicy) {
            auto jani2NNet = dynamic_cast<const Jani2NNet*>(jani2Interface);
            if (config.has_value_option(PLAJA_OPTION::load_nn) and not PLAJA_UTILS::file_exists(jani2NNet->get_torch_file())) { agent->save(&jani2NNet->get_torch_file(), nullptr, saveBackup.get()); }
            if (saveNNet) { agent->save(nullptr, &jani2Interface->get_interface_file(), saveBackup.get()); }
        }
    }

    if (evaluationMode) { 
        if (!treePolicy) {
            agent->eval();
        }
    } // set torch models to eval mode; however should have no impact

    /* Init teacher. */
    if (not evaluationMode) {
        PUSH_LAP(searchStatistics, PLAJA::StatsDouble::TimeForTeacher)
        policyTeacher = PolicyTeacher::construct_policy_teacher(config);
        if (policyTeacher) { teacherGreediness = std::min(1.0, config.get_double_option(PLAJA_OPTION::teacherGreediness)); }
        POP_LAP(searchStatistics, PLAJA::StatsDouble::TimeForTeacher)
    }

    /* A neural network predicts rewards as opposed to having a static reward structure */
    rewardNN = nullptr;
    if (rewardNNPath != nullptr && rewardNNLayers != nullptr) {
        try {
            std::vector<unsigned int> result;
            std::istringstream is(*rewardNNLayers);
            std::string temp;
            char delimiter = '_';
            while (std::getline(is, temp, delimiter)) {
                result.push_back(static_cast<unsigned int>(std::stoul(temp)));
            }

            rewardNN = std::make_shared<TORCH_IN_PLAJA::NeuralNetwork>(result, 0, lower_bounds, upper_bounds, true);
            torch::load(rewardNN, *rewardNNPath);

            const auto l = jani2Interface->get_num_input_features();
            torch::Tensor tensor = torch::zeros(l);
            rewardNN->forward(tensor).item<float>();
            PLAJA_LOG("Reward NN was successfully loaded.")
        } catch (...) {
            rewardNN = nullptr;
            PLAJA_LOG("Unable to load the reward neural network! Proceeding with only static reward values.")
        }
    }

    /* Construct Oracle for fault analysis */
    #ifdef BUILD_FAULT_ANALYSIS
    oracle = FA_ORACLE::construct(config);
    oracle->initialize_oracle(simulatorEnv, nullptr, objective, avoid, searchStatistics);
    #endif
}

QlAgent::~QlAgent() = default;

// auxiliaries:

void QlAgent::load_nn(const std::string& torch_file, const std::string& nnet_file) {
    try {
        agent->load(torch_file);
    }// try to load PyTorch model ...
    catch (PlajaException& e) {
        PLAJA_LOG(e.what())
        agent->load_from_nnet(nnet_file);// try to load NNet ...
    }
}

void QlAgent::save_copy(const std::string& prefix) {
    std::string file_name;
    auto jani2NNet = dynamic_cast<const Jani2NNet*>(jani2Interface);
    PLAJA_ASSERT(jani2NNet)
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

void QlAgent::save_actions() {
    if (saveAgentActions != nullptr) {
        std::ofstream eval_out_file(saveAgentActions->c_str(), std::ios::app);
        if (eval_out_file.fail()) { throw PlajaException(PLAJA_EXCEPTION::fileNotFoundString + saveAgentActions->c_str()); }
        auto data = saveAgentActionsBuffer.str();
        data.pop_back();
        eval_out_file << "{\"sample\":[" << data << "]}" << std::endl;
        eval_out_file.close();
        saveAgentActionsBuffer.clear();
    }
}

//

inline bool QlAgent::check_goal(const State& state) const {
    auto* goal = objective->get_goal();
    return goal and goal->evaluateInteger(&state);
}

inline bool QlAgent::check_avoid(const State& state) const {
    PLAJA_ASSERT(avoid)
    return avoid->evaluateInteger(&state);
}

inline USING_POLICY_LEARNING::Reward_type QlAgent::compute_step_reward(bool done) const {
    const bool is_terminal_or_stalling = not(nextState);// reward for stalling?

    USING_POLICY_LEARNING::Reward_type reward_sum = 0;

    // reward for step
    if (not is_terminal_or_stalling) {// there is no step reward when *stalling*
        reward_sum += static_cast<USING_POLICY_LEARNING::Reward_type>(objective->compute_step_reward(currentState.get(), nextState.get()));
    }

    const auto* goal_potential = evaluationMode ? nullptr : objective->get_goal_potential();// no goal potential in eval mode
    if (goal_potential) {
        const bool ground_current_potential = groundedPotential.count(currentState->get_id_value());
        const bool ground_next_potential = is_terminal_or_stalling or (done and groundDonePotential) or groundedPotential.count(nextState->get_id_value());
        const auto potential_current_state = ground_current_potential ? 0 : goal_potential->evaluateFloating(currentState.get());
        const auto potential_next_state = ground_next_potential ? 0 : goal_potential->evaluateFloating(nextState.get());
        reward_sum += static_cast<USING_POLICY_LEARNING::Reward_type>(potential_next_state - potential_current_state);
    }

    if (rewardNN != nullptr) {
        torch::Tensor tensor = torch::zeros(currentState->size());
        for (int i = 0; i < (currentState->size()); ++i) {
            tensor[i] = currentState->to_state()[i];
        }
        reward_sum += reward_lambda * rewardNN->forward(tensor).item<double>();
    }

    return reward_sum;
}

// auxiliaries to interact in torch agent:

inline void QlAgent::set_current_state(const State& state) {
    currentState = state.to_ptr();
    USING_POLICY_LEARNING::NNState& current_nn_state = agent->currentState;
    const auto l = current_nn_state.size();
    PLAJA_ASSERT(l == jani2Interface->get_num_input_features())
    for (std::size_t i = 0; i < l; ++i) {
        current_nn_state[i] = PLAJA_UTILS::cast_numeric<TORCH_IN_PLAJA::Floating>(state[jani2Interface->get_input_feature_index(i)]);
    }
    agent->set_current_state_tensor();
}

inline void QlAgent::set_next_state(const State& state) {
    nextState = state.to_ptr();
    USING_POLICY_LEARNING::NNState& next_nn_state = agent->nextState;
    const auto l = next_nn_state.size();
    PLAJA_ASSERT(l == jani2Interface->get_num_input_features())
    for (std::size_t i = 0; i < l; ++i) {
        next_nn_state[i] = PLAJA_UTILS::cast_numeric<TORCH_IN_PLAJA::Floating>(state[jani2Interface->get_input_feature_index(i)]);
    }
}

inline void QlAgent::set_next_state_applicability() {
    // next state applicability
    if (jani2Interface->_do_applicability_filtering()) {
        std::list<USING_POLICY_LEARNING::NNAction_type>& next_state_app = agent->nextStateApplicability;
        next_state_app.clear();
        for (const ActionLabel_type action_label: cachedApplicableActions) {
            PLAJA_ASSERT(jani2Interface->is_learned(action_label))
            next_state_app.push_back(jani2Interface->get_output_index(action_label));
        }
    }
}

/* Propagate successor state to agent */
inline void QlAgent::set_next_to_current_state() {
    currentState = std::move(nextState);
    std::swap(agent->currentState, agent->nextState);
    agent->set_current_state_tensor();
}

/* Propagate step rewards to agent as required! */
inline void QlAgent::set_not_done(const State& state) {
    set_next_state(state);
    set_next_state_applicability();
    agent->set_not_done(compute_step_reward(false));
}

inline void QlAgent::set_stalling(bool terminal) {
    nextState = nullptr;
    agent->set_stalling(compute_step_reward(true) + (terminal ? terminalReward : stallingReward));
}

inline void QlAgent::set_goal(const State& state) {
    set_next_state(state);
    agent->set_goal(compute_step_reward(true) + goalReward);
}

inline void QlAgent::set_avoid(const State& state) {
    set_next_state(state);
    agent->set_avoid(compute_step_reward(true) + avoidReward);
}

//
size_t get_ranked_action(const std::vector<double>& output, size_t rank) {
    PLAJA_ASSERT(rank < output.size())

    // Create a copy of the vector and sort it in descending order
    std::vector<double> sortedNumbers = output;
    std::sort(sortedNumbers.rbegin(), sortedNumbers.rend());

    // Find the nth highest number in the sorted vector
    double nthHighest = sortedNumbers[rank];

    // Find the index of the nth highest number in the original vector
    for (size_t i = 0; i < output.size(); ++i) {
        if (output[i] == nthHighest) {
            return i;
        }
    }
    PLAJA_ASSERT(false)// If the nth highest number is not found (which should not happen if n is valid)
    return 0;
}

//
ActionLabel_type QlAgent::act_tree_policy() {
    PLAJA_ASSERT(!cachedApplicableActions.empty())
    auto current_tree_actions = agent->act_ensemble(ensemble);
    auto maxIt = std::max_element(current_tree_actions.begin(), current_tree_actions.end());
    auto current_tree_action = std::distance(current_tree_actions.begin(), maxIt);
    ActionLabel_type current_action_label = jani2Interface->get_output_label(current_tree_action);
    searchStatistics->inc_attr_unsigned(PLAJA::StatsUnsigned::POLICY_ACTS);

    if (jani2Interface->_do_applicability_filtering()) {
        if (simulatorEnv->is_cached_applicable(current_action_label)) { return current_action_label; }
        // currentState->dump(true);
        searchStatistics->inc_attr_unsigned(PLAJA::StatsUnsigned::SUB_RANK_ACTS);
        std::size_t num_ranks = jani2Interface->get_num_output_features();
        for (long int rank = 1; rank < num_ranks; ++rank) {
            current_tree_action = get_ranked_action(current_tree_actions, rank);
            current_action_label = jani2Interface->get_output_label(current_tree_action);
            if (simulatorEnv->is_cached_applicable(current_action_label)) {
                // currentState->dump(true);
                return current_action_label;
            }
        }
        return jani2Interface->get_output_label(get_ranked_action(current_tree_actions, 0));// By default, return best action
    }

    /* No Filtering. */
    return current_action_label;
}

ActionLabel_type QlAgent::act_epsilon_greedy() {
    PLAJA_ASSERT(not cachedApplicableActions.empty()) // stalling due to terminal state is expected to be handled in preceding simulation step

    searchStatistics->inc_attr_unsigned(PLAJA::StatsUnsigned::POLICY_ACTS);

    if (teacherEpisode) {
        /* [Imitation]: Follow a teacher policy */
        searchStatistics->inc_attr_unsigned(PLAJA::StatsUnsigned::TeacherActs);
        PUSH_LAP(searchStatistics, PLAJA::StatsDouble::TimeForTeacher)
        const auto teacher_action = policyTeacher->get_action(*currentState);
        POP_LAP(searchStatistics, PLAJA::StatsDouble::TimeForTeacher)

        if (teacher_action != ACTION::noneLabel) {
            PLAJA_ASSERT(jani2Interface->is_learned(teacher_action))
            agent->set_action(jani2Interface->get_output_index(teacher_action));// Update action within agent.
            return teacher_action;
        }

        teacherEpisode = false;// Teacher failed once, so abort teacher usage for this episode.
        searchStatistics->inc_attr_unsigned(PLAJA::StatsUnsigned::TeacherActsUnsuccessful);
    }

    if (rng->prob() < epsilon) {
        // /* [Exploration]: Randomly sample an applicable action*/
        searchStatistics->inc_attr_unsigned(PLAJA::StatsUnsigned::EPSILON_GREEDY_ACTS);
        const auto& applicable_actions = cachedApplicableActions;
        if (applicable_actions.empty()) { return jani2Interface->get_output_label(agent->act()); } // (selecting inapplicable action)
        const ActionLabel_type random_action = applicable_actions[rng->index(applicable_actions.size())];
        PLAJA_ASSERT(jani2Interface->is_learned(random_action))
        agent->set_action(jani2Interface->get_output_index(random_action)); // update action within agent
        return random_action;
    }

    /* [Exploitation]: Follow the currently best learned policy */
    if (jani2Interface->_do_applicability_filtering()) {    
         /* Get best possible action according to policy that is still applicable */

        USING_POLICY_LEARNING::NNAction_type current_nn_action = agent->rank_actions();// rank actions
        ActionLabel_type current_action_label = jani2Interface->get_output_label(current_nn_action);
        if (simulatorEnv->is_cached_applicable(current_action_label)) { 
            // Highest action preferred by policy is applicable!
            return current_action_label;
        }

        searchStatistics->inc_attr_unsigned(PLAJA::StatsUnsigned::SUB_RANK_ACTS);

        /* Go down policy list to get best applicable action */
        const auto num_ranks = jani2Interface->get_num_output_features();
        for (long int rank = 1; rank < num_ranks; ++rank) {
            current_nn_action = agent->retrieve_ranked_action(rank);// also updates current action on agent side
            current_action_label = jani2Interface->get_output_label(current_nn_action);
            if (simulatorEnv->is_cached_applicable(current_action_label)) {
                return current_action_label;
            }
        }

        // No applicable action, fall back to no-filtering (selecting inapplicable action) ...
        PLAJA_ASSERT(cachedApplicableActions.empty())
        PLAJA_ABORT// Currently not expected.
    }

    /* No Filtering. */
    auto current_action_label = jani2Interface->get_output_label(agent->act());
    // std::cout << "Selecting action: " << current_action_label << " for current state: "; currentState->dump();
    return current_action_label;
}

std::unique_ptr<State> QlAgent::simulate(const State& state, ActionLabel_type action_label) {
    /* Take a state and action, return a possible next state. */

    auto successor_state = simulatorEnv->compute_successor_if_applicable(state, action_label); // Compute possible successor
    learningStats->inc_epsilon_greedy_selection(model->get_action_name(action_label));
    searchStatistics->inc_attr_unsigned(PLAJA::StatsUnsigned::ACTION_LABELS);
    searchStatistics->inc_attr_unsigned(PLAJA::StatsUnsigned::EXPANDED_STATES);

    // Termination Checks
    if (not successor_state) {
        /* [T1] Action was inapplicable */
        searchStatistics->inc_attr_unsigned(PLAJA::StatsUnsigned::DONE_STALLING);
        set_stalling(false);
        return successor_state;
    } else {
        // Action applicable so update statistics
        searchStatistics->inc_attr_unsigned(PLAJA::StatsUnsigned::GENERATED_STATES);
        searchStatistics->inc_attr_unsigned(PLAJA::StatsUnsigned::ACTION_OPS);
        searchStatistics->inc_attr_unsigned(PLAJA::StatsUnsigned::TRANSITIONS);
    }

    if (check_goal(*successor_state)) {
        /* [T2] We reached a state in the goal condition */
        searchStatistics->inc_attr_unsigned(PLAJA::StatsUnsigned::GOAL_STATES);
        searchStatistics->inc_attr_unsigned(PLAJA::StatsUnsigned::DONE_GOAL);
        set_goal(*successor_state);
        return successor_state;
    }

    if (check_avoid(*successor_state)) {
        /* [T3] We reached a state in the failure condition. */
        searchStatistics->inc_attr_unsigned(PLAJA::StatsUnsigned::DONE_AVOID);
        set_avoid(*successor_state);
        return successor_state;
    }

    // simulate until next choice point
    cachedApplicableActions = simulatorEnv->extract_applicable_actions(*successor_state, true);
    while (cachedApplicableActions.size() <= 1) {

        if (cachedApplicableActions.empty()) {
            searchStatistics->inc_attr_unsigned(PLAJA::StatsUnsigned::TERMINAL_STATES);
            searchStatistics->inc_attr_unsigned(PLAJA::StatsUnsigned::DONE_STALLING);
            set_stalling(true);
            return successor_state;
        }

        // step:
        const ActionLabel_type next_action = cachedApplicableActions[0];
        if (jani2Interface->is_learned(next_action)) { break; }// may happen on ipc benchmarks
        PLAJA_ASSERT(next_action == ACTION::silentAction)

        successor_state = simulatorEnv->compute_successor(*successor_state, next_action);
        searchStatistics->inc_attr_unsigned(PLAJA::StatsUnsigned::GENERATED_STATES);
        searchStatistics->inc_attr_unsigned(PLAJA::StatsUnsigned::ACTION_OPS);
        searchStatistics->inc_attr_unsigned(PLAJA::StatsUnsigned::TRANSITIONS);
        /* Termination Checks again */
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

        // applicable actions for next iteration
        cachedApplicableActions = simulatorEnv->extract_applicable_actions(*successor_state, true);
    }

    // Mark episode as not completed
    set_not_done(*successor_state);
    #ifdef BUILD_FAULT_ANALYSIS
        if(unsafeReward != 0) {
        std::cout << "Inside oracle call" << std::endl;
        Oracle::STATE_SAFETY current_state_safety = oracle->is_state_safe(state.get_id_value());
        if (current_state_safety == Oracle::STATE_SAFETY::UNSAFE) {
            agent->add_unsafe_reward(unsafeReward);
        }
    }
    #endif
    return successor_state;
}

std::unique_ptr<State> QlAgent::simulate_until_choice_point(const State& state) {
    /* Simulate a start state until choice point, i.e. more than one applicable action. */
    
    std::unique_ptr<State> successor_state = state.to_ptr();
    // simulate until next choice point
    cachedApplicableActions = simulatorEnv->extract_applicable_actions(*successor_state, true);
    while (cachedApplicableActions.size() <= 1) {

        if (cachedApplicableActions.empty()) {
            searchStatistics->inc_attr_unsigned(PLAJA::StatsUnsigned::TERMINAL_STATES);
            // successor_state->dump(true);
            return nullptr;
        }

        // step:
        const ActionLabel_type next_action = cachedApplicableActions[0];
        if (jani2Interface->is_learned(next_action)) { break; }// may happen on ipc benchmarks
        PLAJA_ASSERT(next_action == ACTION::silentAction)

        successor_state = simulatorEnv->compute_successor(*successor_state, next_action);
        searchStatistics->inc_attr_unsigned(PLAJA::StatsUnsigned::GENERATED_STATES);
        searchStatistics->inc_attr_unsigned(PLAJA::StatsUnsigned::ACTION_OPS);
        searchStatistics->inc_attr_unsigned(PLAJA::StatsUnsigned::TRANSITIONS);

        // termination checks
        if (check_goal(*successor_state)) {
            searchStatistics->inc_attr_unsigned(PLAJA::StatsUnsigned::GOAL_STATES);
            return nullptr;
        }
        if (check_avoid(*successor_state)) {
            return nullptr;
        }

        // applicable actions for next iteration
        cachedApplicableActions = simulatorEnv->extract_applicable_actions(*successor_state, true);
    }

    return successor_state;
}

State QlAgent::sample_start_state() {
    /* Sample a start state to be used in the episode */
    searchStatistics->inc_attr_unsigned(PLAJA::StatsUnsigned::GENERATED_STATES);

    if (startEnumerator and (startStates.empty() or rng->prob() >= 1.0 / scoreWindow->slicing_size())) {
        // Randomly sample a start state from the start condition.
        auto start_sample = startEnumerator->sample_state();
        PLAJA_ASSERT(start_sample)
        if (!start_sample) {
            PLAJA_LOG("Warning: No start state found.")
        }
        auto start_state = simulatorEnv->get_state(*start_sample);
        // check if this state was already sampled.
        return simulatorEnv->get_state(*start_sample);
    }

    /* If you have fixed set of start states, then pick one. */
    PLAJA_ASSERT_EXPECT(not startEnumerator or startStates.size() <= 1)// Latter should only contain primary state (iff not subsumed by start).
    const StateID_type start_id = evaluationMode ? startStates[episodesCounter % startStates.size()] : rng->index<StateID_type>(startStates.size());
    auto start_state = simulatorEnv->get_state(start_id);
    return simulatorEnv->get_state(start_id);
}

//

SearchEngine::SearchStatus QlAgent::initialize() { return SearchStatus::IN_PROGRESS; }

SearchEngine::SearchStatus QlAgent::step() {
    if (episodesCounter >= numEpisodes) { // Termination condition: Episodes have finished
        save_actions(); // Save agent actions. Internally does nothing if not set in config.
        return SearchStatus::SOLVED;
    }
    // Run an episode: Sample start state, Simulate policy, learn and then update scores and best policy

    // Step 1: Sample start and execute it until atleast two actions are applicable
    std::unique_ptr<State> current_state = simulate_until_choice_point(sample_start_state());

    if (not current_state) { // drawn start state leads to termination without any action decision
        static bool print_warning(true);
        PLAJA_LOG_IF(print_warning, PLAJA_UTILS::to_red_string("Terminal start state."))
        print_warning = false;
        return SearchStatus::IN_PROGRESS; // move to next episode
    } 
    else { // found choice point 
        ++episodesCounter;
        searchStatistics->inc_attr_unsigned(PLAJA::StatsUnsigned::EPISODES);
    }

    teacherEpisode = rng->prob() < teacherGreediness;

    /* Initialize episode */
    cycleDetected = false;
    cachedTransitionsPerEpisode.clear();
    double score = 0; // initial

    // set current state in torch agent
    PLAJA_ASSERT(agent)
    agent->set_not_done(0);
    set_current_state(*current_state);

    auto episode_length = 0;
    for (; episode_length < maxLengthEpisode and not(terminateCycles and cycleDetected) and not agent->_done(); ++episode_length) {
        PLAJA_ASSERT(current_state)
        /* Step 2. Simulate policy*/
        const ActionLabel_type action_label = treePolicy ? act_tree_policy() : act_epsilon_greedy(); // Retrieve action label from policy or epsilon greedy sampling
        
        if (saveAgentActions != nullptr) { // Save Agent Actions if needed
            saveAgentActionsBuffer << "{\"state\":" << current_state->to_str() << PLAJA_UTILS::commaString;
            saveAgentActionsBuffer << "\"label\":" << action_label << "}" << PLAJA_UTILS::commaString;
        }

        current_state = simulate(*current_state, action_label); // Simulate the retrieved action to get the next state

        /* Cache state, action for cycle detection */
        if (cachedTransitionsPerEpisode.insert({currentState->get_id_value(), action_label, nextState ? nextState->get_id_value() : STATE::none}).second) {
            if (not evaluationMode) { 
                /* Step 3. Learn based on rewards */
                agent->step_learning(jani2Interface->_do_applicability_filtering());
            }
        } else {
            cycleDetected = true;
        }
        set_next_to_current_state(); // Set next state internally in torch 

        /* Punish unsafe behaviour using fault analysis */
        
        //
        score += agent->_current_reward() * std::pow(USING_POLICY_LEARNING::gamma, episode_length); // Priority to latest rewards. Reaching the goal earlier is better than reaching it later.

    }

    /* Statistics Handling */
    episodeLength->add(static_cast<double>(episode_length));
    if (not agent->_done()) { searchStatistics->inc_attr_unsigned(PLAJA::StatsUnsigned::UNDONE); }
    if (cycleDetected) { searchStatistics->inc_attr_unsigned(PLAJA::StatsUnsigned::DETECTED_CYCLES); }
    // update epsilon
    scoreWindow->add(score * (1 - epsilon)); // adjust score wrt. epsilon, i.e., the larger epsilon the less "valuable" is the score
    if (not teacherEpisode) {
        epsilon = std::max(epsilonEnd, epsilonDecay * epsilon);
        searchStatistics->set_attr_double(PLAJA::StatsDouble::EPSILON, epsilon);
    }

    /* Step 4: Score Handling and Update policy */
    if (episodesCounter % scoreWindow->slicing_size() == 0 or episodesCounter >= numEpisodes) {
        PLAJA_LOG("Updating stats and backups...");

        // mean score
        auto mean_score = scoreWindow->average();
        auto average_length = episodeLength->average();
        searchStatistics->set_attr_double(PLAJA::StatsDouble::LAST_SCORE, mean_score);
        searchStatistics->set_attr_double(PLAJA::StatsDouble::LAST_AVERAGE_LENGTH, average_length);

        /* Saving policies based on best score achieved till now. */
        if (mean_score >= bestScore) {

            searchStatistics->set_attr_double(PLAJA::StatsDouble::BEST_SCORE, bestScore = mean_score);// update best score
            searchStatistics->set_attr_double(PLAJA::StatsDouble::AVERAGE_LENGTH_BEST_SCORE, average_length);
            if (not evaluationMode) {
                auto jani2NNet = dynamic_cast<const Jani2NNet*>(jani2Interface); 
                agent->save(&jani2NNet->get_torch_file(), saveNNet, saveBackup.get());
            }// save NN

        } else if (not evaluationMode and (episodesCounter >= numEpisodes or is_almost_expired())) { // Final backup.
            save_copy("final_");
        } else if (not evaluationMode and episodesCounter % (scoreWindow->slicing_size() * 10) == 0) { // Tmp backup.
            save_copy("tmp_");
        }

        PLAJA_LOG(PLAJA_UTILS::string_f("Episode: %u, average length: %f, mean score: %f, best score: %f, epsilon: %f.", episodesCounter, average_length, mean_score, bestScore, epsilon))
        if (episodesCounter < numEpisodes) { trigger_intermediate_stats(); }// For learning curve.

        PLAJA_LOG("... update done.\n")
    }

    return SearchStatus::IN_PROGRESS;
}

// override:

void QlAgent::print_statistics() const {
    searchStatistics->print_statistics();
    learningStats->print_statistics();
}

void QlAgent::stats_to_csv(std::ofstream& file) const {
    searchStatistics->set_attr_unsigned(PLAJA::StatsUnsigned::EPISODES, episodesCounter);// work-around to order stats properly
    searchStatistics->stats_to_csv(file);
    learningStats->stats_to_csv(file);
}

void QlAgent::stat_names_to_csv(std::ofstream& file) const {
    searchStatistics->stat_names_to_csv(file);
    learningStats->stat_names_to_csv(file);
}