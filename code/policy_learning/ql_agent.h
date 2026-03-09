//
// This file is part of the PlaJA code base.
// Copyright (c) (2019 - 2021) Marcel Vinzent.
// See README.md in the top-level directory for licensing information.
//

#ifndef PLAJA_QL_AGENT_H
#define PLAJA_QL_AGENT_H

#include "../search/smt/bias_functions/distance_function.h"
#include "../search/smt/model/model_z3.h"
#include "../parser/ast/expression/forward_expression.h"
#include "../search/fd_adaptions/search_engine.h"
#include "../search/information/jani2nnet/forward_jani_2_nnet.h"
#include "../search/information/jani2nnet/using_jani2nnet.h"
#include "../search/information/jani_2_interface.h"
#include "../search/information/jani2ensemble/jani_2_ensemble.h"
#include "../search/states/forward_states.h"
#include "../search/successor_generation/forward_successor_generation.h"
#include "../search/using_search.h"
#include "./neural_networks/neural_network.h"
#include "forward_policy_learning.h"
#include "transition_set.h"
#include "using_policy_learning.h"
#include <unordered_set>
#include <vector>

#ifdef BUILD_FAULT_ANALYSIS
#include "../search/fault_analysis/search_based/oracle.h"
#endif

class QlAgent: public SearchEngine {

private:

    const bool evaluationMode;                              // Run simulation without learning the NN, to evaluate an already learned NN.  // NOLINT(*-avoid-const-or-ref-data-members)
    unsigned int numEpisodes;                               // Non-cost due to evaluation mode.
    unsigned int maxStartStates;
    const unsigned int maxLengthEpisode;                    // NOLINT(*-avoid-const-or-ref-data-members)

    /* Random exploration parameters */
    double epsilon;
    const double epsilonDecay;                              // NOLINT(*-avoid-const-or-ref-data-members)
    const double epsilonEnd;                                // NOLINT(*-avoid-const-or-ref-data-members)
    
    /* Regularizers */
    const bool grad_clip;
    const double l1_lambda;
    const double l2_lambda;
    const double group_lambda;
    const double kl_lambda;
    const double reward_lambda;
    
    /* Rewards */
    const USING_POLICY_LEARNING::Reward_type goalReward;    // NOLINT(*-avoid-const-or-ref-data-members)
    const USING_POLICY_LEARNING::Reward_type avoidReward;   // NOLINT(*-avoid-const-or-ref-data-members)
    const USING_POLICY_LEARNING::Reward_type stallingReward;// NOLINT(*-avoid-const-or-ref-data-members)
    const USING_POLICY_LEARNING::Reward_type terminalReward;// NOLINT(*-avoid-const-or-ref-data-members)
    #ifdef BUILD_FAULT_ANALYSIS
    const USING_POLICY_LEARNING::Reward_type unsafeReward; // NOLINT(*-avoid-const-or-ref-data-members)
    #endif
    const bool terminateCycles;                             // NOLINT(*-avoid-const-or-ref-data-members)
    
    /* Saving Policy variables */
    const std::string* saveNNet;
    const std::string* savePolicyPath;

    /* Neural Networks which give rewards */
    const std::string* rewardNNPath;
    const std::string* rewardNNLayers;
    const std::string* postfix;
    std::shared_ptr<TORCH_IN_PLAJA::NeuralNetwork> rewardNN;
    std::unique_ptr<std::string> saveBackup;// Where/whether to *also* save NN structure.

    /* Saving actions used by a network */
    std::unique_ptr<std::string> saveAgentActions;
    std::stringstream saveAgentActionsBuffer;
    std::unique_ptr<std::string> learningStatsFile;

    /* Dynamic learning parameters & stats. */
    unsigned int episodesCounter;
    double bestScore;
    std::unique_ptr<LearningStatistics> learningStats;// Additional stats.
    std::unique_ptr<SlicingWindow> scoreWindow;
    std::unique_ptr<SlicingWindow> episodeLength;

    /** **/
    bool treePolicy = false; // Do we use a tree ensemble policy

    /* Learning task specification. */
    std::unique_ptr<class InitialStatesEnumerator> startEnumerator;
    std::vector<StateID_type> startStates;
    const Jani2Interface* jani2Interface; // interface file to the policy
    const ObjectiveExpression* objective; // goal condition
    const Expression* avoid; // failure condition
    std::unordered_set<StateID_type> groundedPotential;// To check membership.
    bool groundDonePotential;

    /* Policy teacher. */
    std::unique_ptr<PolicyTeacher> policyTeacher;
    double teacherGreediness;

    /* Heart of the learning agent. */
    std::shared_ptr<SimulationEnvironment> simulatorEnv;//
    class RandomNumberGenerator* rng;                   //
    std::unique_ptr<QlAgentTorch> agent;

    /* Cache. */
    std::unique_ptr<State> currentState;
    std::unique_ptr<State> nextState;
    std::vector<ActionLabel_type> cachedApplicableActions;
    QL_AGENT::TransitionSet cachedTransitionsPerEpisode;
    bool teacherEpisode;
    bool cycleDetected;
    veritas::AddTree ensemble;

    std::unique_ptr<Expression> start; // start condition

    #ifdef BUILD_FAULT_ANALYSIS
    std::unique_ptr<Oracle> oracle; // oracle for fault analysis
    #endif
    /* Auxiliaries. */
    [[nodiscard]] bool check_goal(const State& state) const;
    [[nodiscard]] bool check_avoid(const State& state) const;
    [[nodiscard]] USING_POLICY_LEARNING::Reward_type compute_step_reward(bool done) const;
    
    /* Auxiliaries to interact in torch agent. */
    void set_current_state(const State& state);
    void set_next_state(const State& state);
    void set_next_state_applicability();
    void set_next_to_current_state();
    void set_not_done(const State& state);
    void set_goal(const State& state);
    void set_avoid(const State& state);
    void set_stalling(bool terminal);

    /* Action Decision Making*/
    ActionLabel_type act_epsilon_greedy(); // either random action or neural network policy
    ActionLabel_type act_tree_policy(); // use tree ensemble policy to decide next action

    std::unique_ptr<State> simulate(const State& state, ActionLabel_type action_label);// True if reached terminal state.
    std::unique_ptr<State> simulate_until_choice_point(const State& state);            // To reach first choice point starting from start state.
    State sample_start_state();                                                        // Sample a start state from which we start QLearning episode

    /* Loading and Saving Neural Network policies */
    void load_nn(const std::string& torch_file, const std::string& nnet_file);
    void save_copy(const std::string& prefix);

    void save_actions(); // To save the actions made by the action policy.
    
    SearchStatus initialize() override;
    SearchStatus step() override;

    // override:
    void print_statistics() const override;
    void stats_to_csv(std::ofstream& file) const override;
    void stat_names_to_csv(std::ofstream& file) const override;

public:
    explicit QlAgent(const PLAJA::Configuration& config);
    ~QlAgent() override;
    DELETE_CONSTRUCTOR(QlAgent)
};

#endif//PLAJA_QL_AGENT_H
