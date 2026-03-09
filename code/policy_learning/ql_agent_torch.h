//
// This file is part of the PlaJA code base.
// Copyright (c) (2019 - 2021) Marcel Vinzent.
// See README.md in the top-level directory for licensing information.
//

#ifndef PLAJA_QL_AGENT_TORCH_H
#define PLAJA_QL_AGENT_TORCH_H

#include <memory>
#include <vector>
#include <list>
#include <unordered_map>
#include "../utils/default_constructors.h"
#include "forward_policy_learning.h"
#include "using_policy_learning.h"
#include "addtree.hpp"
#include "json_io.hpp"

/** Substructures of QLAgent that use torch. */
class QlAgentTorch {
    friend QlAgent;
    friend ReductionAgent;

private:
    // learning task infrastructure
    std::shared_ptr<TORCH_IN_PLAJA::NeuralNetwork> policyNN;
    std::shared_ptr<TORCH_IN_PLAJA::NeuralNetwork> targetNN;
    std::unique_ptr<torch::optim::Adam> optimizer;
    std::unique_ptr<ReplayBuffer> replayBuffer;
    unsigned int updateCounter;

    // for applicability filtering
    std::unique_ptr<torch::Tensor> currentActionRanking; // actions ordered DEC

    std::unique_ptr<torch::Tensor> currentStateTensor; // to evaluate the network
    USING_POLICY_LEARNING::NNState currentState; // set in main agent
    USING_POLICY_LEARNING::NNState nextState; // set in main agent
    USING_POLICY_LEARNING::NNAction_type currentAction;
    std::list<USING_POLICY_LEARNING::NNAction_type> nextStateApplicability;
    USING_POLICY_LEARNING::Reward_type currentReward;
    bool currentDone;

    // Regularization
    bool grad_clip = 0;
    double l1_lambda = 0;
    double l2_lambda = 0;
    double group_lambda = 0;
    double kl_lambda = 0;

    void learn(bool applicability_filtering);
    /* void adjust_learning_rate(unsigned int episode); */

public:
    // explicit QlAgentTorch(const std::vector<unsigned int>& layers, const std::vector<TORCH_IN_PLAJA::Floating>& lower_bounds, const std::vector<TORCH_IN_PLAJA::Floating>& upper_bounds);
    explicit QlAgentTorch(int input_size, const std::vector<TORCH_IN_PLAJA::Floating>& lower_bounds, const std::vector<TORCH_IN_PLAJA::Floating>& upper_bounds);
    explicit QlAgentTorch(const std::vector<unsigned int>& layers, double reductionThreshold, const std::vector<TORCH_IN_PLAJA::Floating>& lower_bounds, const std::vector<TORCH_IN_PLAJA::Floating>& upper_bounds, bool grad_clip, double l1_lambda, double l2_lambda, double group_lambda, double kl_lambda);
    ~QlAgentTorch();
    DELETE_CONSTRUCTOR(QlAgentTorch)

    [[nodiscard]] inline USING_POLICY_LEARNING::NNAction_type _current_action() const { return currentAction; }

    [[nodiscard]] inline USING_POLICY_LEARNING::Reward_type _current_reward() const { return currentReward; }

    [[nodiscard]] inline bool _done() const { return currentDone; }

    void set_current_state_tensor();

    inline void set_action(const USING_POLICY_LEARNING::NNAction_type action) { currentAction = action; };

    inline void add_unsafe_reward(const USING_POLICY_LEARNING::Reward_type unsafe_reward) {
        currentReward += unsafe_reward;
    }
    inline void set_not_done(const USING_POLICY_LEARNING::Reward_type undone_reward) {
        currentReward = undone_reward;
        currentDone = false;
    }

    inline void set_goal(const USING_POLICY_LEARNING::Reward_type goal_reward) {
        nextStateApplicability.clear();
        currentReward = goal_reward;
        currentDone = true;
    }

    inline void set_avoid(const USING_POLICY_LEARNING::Reward_type avoid_reward) {
        nextStateApplicability.clear();
        currentReward = avoid_reward;
        currentDone = true;
    }

    inline void set_stalling(const USING_POLICY_LEARNING::Reward_type stalling_reward) {
        nextState = currentState;
        nextStateApplicability.clear();
        currentReward = stalling_reward;
        currentDone = true;
    }

    inline void set_cycle(const USING_POLICY_LEARNING::Reward_type cycle_reward) {
        nextState = currentState;
        currentReward = cycle_reward;
        currentDone = true;
    }

    /**
     * For usage under "no-filtering".
     * Set current action according to argmax.
     * @return the action set.
     */
    USING_POLICY_LEARNING::NNAction_type act();
    /**
     * For usage under "applicability filtering", with the main agent maintaining the model and thus the applicability information.
     * Compute a vector of actions sorted (DEC) according NN output, i.e, their rank.
     * Set current action according to highest rank, i.e., rank 0.
     * @return highest ranked action.
     */
    USING_POLICY_LEARNING::NNAction_type rank_actions();
    /**
     * To be called after rank_actions().
     * Set current action according to given rank.
     * @param rank
     * @return action at given rank.
     */
    USING_POLICY_LEARNING::NNAction_type retrieve_ranked_action(long int rank); // propose action at given rank

    void step_learning(bool applicability_filtering);

    /**
     * Save policy NN in Pytorch (and optionally NNet) format.
     * @param pytorch_filename
     * @param nnet_filename
     * @param backup_path
     */
    void save(const std::string* pytorch_filename, const std::string* nnet_filename, const std::string* backup_path) const;
    void load(const std::string& filename);
    void load_from_nnet(const std::string& filename);
    void eval();
    std::unordered_map<std::string, unsigned int> reduce(double frequency, bool compact);
    std::vector<double> act_ensemble(veritas::AddTree ensemble);
};

#endif //PLAJA_QL_AGENT_TORCH_H
