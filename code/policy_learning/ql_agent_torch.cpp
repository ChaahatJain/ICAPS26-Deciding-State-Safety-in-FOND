//
// This file is part of the PlaJA code base.
// Copyright (c) (2019 - 2021) Marcel Vinzent.
// See README.md in the top-level directory for licensing information.
//

#include "ql_agent_torch.h"
#include "../exception/constructor_exception.h"
#include "../utils/utils.h"
#include "neural_networks/neural_network.h"
#include "replay_buffer.h"
#include <torch/nn/modules/linear.h>
#include <torch/optim/adam.h>
#include <torch/serialize.h>

QlAgentTorch::QlAgentTorch(const std::vector<unsigned int>& layers, double reductionThreshold, const std::vector<TORCH_IN_PLAJA::Floating>& lower_bounds, const std::vector<TORCH_IN_PLAJA::Floating>& upper_bounds, bool grad_clip, double l1_lambda, double l2_lambda, double group_lambda, double kl_lambda):
    updateCounter(0)
    , currentActionRanking(new torch::Tensor())
    , currentAction(USING_POLICY_LEARNING::noneAction)
    , nextStateApplicability()
    , currentReward(0)
    , currentDone(false)
    , grad_clip(grad_clip)
    , l1_lambda(l1_lambda)
    , l2_lambda(l2_lambda)
    , group_lambda(group_lambda)
    , kl_lambda(kl_lambda) {
    PLAJA_ASSERT(layers.size() >= 3) // input, output, at least one hidden layer
    const unsigned int input_size = layers[0];
    const unsigned int output_size = layers[layers.size() - 1];

    policyNN = std::make_shared<TORCH_IN_PLAJA::NeuralNetwork>(layers, reductionThreshold, lower_bounds, upper_bounds);
    targetNN = std::make_shared<TORCH_IN_PLAJA::NeuralNetwork>(layers, reductionThreshold, lower_bounds, upper_bounds);
    optimizer = std::make_unique<torch::optim::Adam>(policyNN->parameters(), USING_POLICY_LEARNING::learningRate); // TODO? non-constant LR, e.g., exponential decrease (see adjust_learning_rate)
    replayBuffer = std::make_unique<ReplayBuffer>(USING_POLICY_LEARNING::batchSize, USING_POLICY_LEARNING::bufferSize, input_size, output_size);

    currentStateTensor = std::make_unique<torch::Tensor>(torch::zeros(input_size));
    currentState = USING_POLICY_LEARNING::NNState(input_size);
    nextState = USING_POLICY_LEARNING::NNState(input_size);
}

QlAgentTorch::QlAgentTorch(int input_size, const std::vector<TORCH_IN_PLAJA::Floating>& lower_bounds, const std::vector<TORCH_IN_PLAJA::Floating>& upper_bounds):
    updateCounter(0)
    , currentActionRanking(new torch::Tensor())
    , currentAction(USING_POLICY_LEARNING::noneAction)
    , nextStateApplicability()
    , currentReward(0)
    , currentDone(false) {
    currentStateTensor = std::make_unique<torch::Tensor>(torch::zeros(input_size));
    currentState = USING_POLICY_LEARNING::NNState(input_size);
    nextState = USING_POLICY_LEARNING::NNState(input_size);
}

QlAgentTorch::~QlAgentTorch() = default;

//

void QlAgentTorch::set_current_state_tensor() {
    const std::size_t state_size = currentState.size();
    PLAJA_ASSERT(currentStateTensor->size(0) == state_size)
    for (long int state_index = 0; state_index < state_size; ++state_index) {
        (*currentStateTensor)[state_index] = currentState[state_index];
    }
}

USING_POLICY_LEARNING::NNAction_type QlAgentTorch::act() {
    return currentAction = policyNN->forward(*currentStateTensor).argmax().item<USING_POLICY_LEARNING::NNAction_type>();
}

USING_POLICY_LEARNING::NNAction_type QlAgentTorch::rank_actions() {
    *currentActionRanking = std::get<1>(policyNN->forward(*currentStateTensor).sort(0, true));
    PLAJA_ASSERT(currentStateTensor->size(0) > 0)
    return currentAction = (*currentActionRanking)[0].item<USING_POLICY_LEARNING::NNAction_type>();
}

USING_POLICY_LEARNING::NNAction_type QlAgentTorch::retrieve_ranked_action(long int rank) {
    PLAJA_ASSERT(0 <= rank)
    PLAJA_ASSERT(rank < currentActionRanking->size(0))
    return currentAction = (*currentActionRanking)[rank].item<USING_POLICY_LEARNING::NNAction_type>();
}

void QlAgentTorch::step_learning(bool applicability_filtering) {
    replayBuffer->add_sample(currentState, nextState, currentAction, nextStateApplicability, currentReward, currentDone); // add the sample to the buffer
    ++updateCounter;
    updateCounter %= USING_POLICY_LEARNING::updateEvery; // set update counter

    // learn if requirements fulfilled
    if (updateCounter == 0 && replayBuffer->size() > USING_POLICY_LEARNING::batchSize) {
        replayBuffer->sample();
        learn(applicability_filtering);
    }
}

void QlAgentTorch::learn(bool applicability_filtering) {
    // Implementation of dqn algorithm
    torch::Tensor q_values_next_states;
    if (applicability_filtering) {
        auto values_next_states = targetNN->forward(replayBuffer->next_states_sample());
        auto min_value = std::get<0>(values_next_states.max(1)).min(); // suppress gradient required
        q_values_next_states = std::get<0>((values_next_states + ((min_value - 1 - values_next_states) * (1 - replayBuffer->next_state_applicability_sample()))).max(1)).detach(); // suppress gradient required;
    } else {
        q_values_next_states = std::get<0>(targetNN->forward(replayBuffer->next_states_sample()).max(1)).detach(); // suppress gradient required
    }

    auto targets = replayBuffer->rewards_sample() + (USING_POLICY_LEARNING::gamma * (q_values_next_states) * (1 - replayBuffer->done_sample()));
    auto q_values = policyNN->forward(replayBuffer->states_sample());

    const auto& actions_sample = replayBuffer->actions_sample();
    auto actions_view = actions_sample.view({ actions_sample.sizes()[0], 1 });
    auto predictions = torch::gather(q_values, 1, actions_view).view(targets.sizes()[0]);

    // loss calculation
    auto loss = torch::mse_loss(predictions, targets);

    // Gradient clipping
    if (grad_clip) {
        for (auto& param : policyNN->parameters()) {
            if (param.grad().defined()) {
                param.grad().data().clamp_(-1.0, 1.0);
            }
        }
    }

    // L1 regularization
    if (l1_lambda > 0) {
        torch::Tensor l1 = torch::tensor(0.0, loss.options());
        for (const auto& param : policyNN->parameters()) {
            if (param.requires_grad()) {
                l1 += param.abs().sum();
            }
        }
        loss = loss + l1_lambda * l1;
    }

    // L2 regularization
    if (l2_lambda > 0) {
        torch::Tensor l2 = torch::tensor(0.0, loss.options());
        for (const auto& param : policyNN->parameters()) {
            if (param.requires_grad()) {
                l2 += param.pow(2).sum();
            }
        }
        loss = loss + l2_lambda * l2;
    }

    // Group lasso
    if (group_lambda > 0) {
        torch::Tensor lasso = torch::tensor(0.0, loss.options());;
        for (const auto& module : policyNN->modules(false)) {
            if (auto* linear = dynamic_cast<torch::nn::LinearImpl*>(module.get())) {
                lasso += linear->weight.norm(2, 1).sum();
            }
        }
        loss = loss + group_lambda * lasso;
    }

    // KL divergence
    if (kl_lambda > 0) {
        auto states_sample = replayBuffer->states_sample();
        auto current_probs = torch::softmax(policyNN->forward(states_sample), 1);
        auto target_probs = torch::softmax(targetNN->forward(states_sample), 1);
        auto kl_div = (target_probs * (torch::log(target_probs + 1e-8) - torch::log(current_probs + 1e-8))).sum(1).mean();
        loss = loss + kl_lambda * kl_div;
    }

    // make backwards step (optimize)
    optimizer->zero_grad();
    loss.backward();
    // for (auto& param: policyNN->parameters()) { param.grad().data().clamp(-1, 1); } // TODO might be useful?
    optimizer->step();

    // perform a soft-update to the target network
    auto parameters = policyNN->parameters();
    auto parameters_target = targetNN->parameters();
    const std::size_t l = parameters.size();
    PLAJA_ASSERT(l == parameters_target.size())
    for (std::size_t i = 0; i < l; ++i) {
        parameters_target[i].data().copy_(USING_POLICY_LEARNING::tau * parameters[i].data() + (1.0 - USING_POLICY_LEARNING::tau) * parameters_target[i].data());
    }
}

// load and save

void QlAgentTorch::save(const std::string* pytorch_filename, const std::string* nnet_filename, const std::string* backup_path) const {
    if (pytorch_filename) { torch::save(policyNN, *pytorch_filename); }
    if (nnet_filename) { policyNN->to_nnet(*nnet_filename); }
    if (backup_path) {
        if (pytorch_filename) { torch::save(policyNN, PLAJA_UTILS::join_path(*backup_path, { PLAJA_UTILS::extract_filename(*pytorch_filename, true) })); }
        if (nnet_filename) { policyNN->to_nnet(PLAJA_UTILS::join_path(*backup_path, { PLAJA_UTILS::extract_filename(*nnet_filename, true) })); }
    }
}

void QlAgentTorch::load(const std::string& filename) {
    try {
        torch::load(policyNN, filename);
        torch::load(targetNN, filename);
    } catch (c10::Error& e) {
        throw ConstructorException("Parsing PyTorch model " + filename + " failed: " + e.what_without_backtrace());
    }
}

void QlAgentTorch::load_from_nnet(const std::string& filename) {
    std::cout << "Trying to load " << filename << " as NNet ..." << std::endl;
    policyNN->load_from_nnet(filename);
    targetNN->load_from_nnet(filename);
}

void QlAgentTorch::eval() {
    policyNN->eval(); // set to train mode, however using torch::nn::linear this should have no impact (?)
    targetNN->eval();
}

std::unordered_map<std::string, unsigned int> QlAgentTorch::reduce(double frequency, bool compact) {
    std::unordered_map<std::string, unsigned int> reducedNeurons;
    std::vector<std::string> layerDesc = policyNN->get_LayerDesc();
    for (const auto& desc : layerDesc) {
        reducedNeurons.emplace(desc, 0);
    }
    auto boundary = static_cast<unsigned int> (policyNN->get_totalPasses() * frequency);
    for (unsigned int i = 0; i < policyNN->get_zeroFrequencies().size(); ++i) {
        int j = 0;
        while (j < policyNN->get_zeroFrequencies()[i].size()) {
            auto neurons = policyNN->get_zeroFrequencies()[i];
            if (neurons[j] >= boundary) {
                if (!policyNN->isNeuronZeroed(i, j)) {
                    policyNN->setNeuronToZero(i, j, compact);
                    reducedNeurons[layerDesc[i]] += 1;
                }
            }
            j++;
        }
    }
    policyNN->resetTracking();
    return reducedNeurons;
}


#include <iostream>
#include <fstream>
#include <sstream>
#include "../exception/parser_exception.h"

std::vector<double> QlAgentTorch::act_ensemble(veritas::AddTree ensemble) {
    std::vector<veritas::FloatT> inputs;
    inputs.reserve(currentState.size());
    for (auto i: currentState) {
        inputs.push_back(i);
    }
    auto num_outputs = ensemble.num_leaf_values();
    std::vector<double> outputs;
    outputs.resize(num_outputs);
    veritas::data d {&inputs[0], 1, inputs.size(), inputs.size(), 1};
    veritas::data<double> outdata(outputs);
    ensemble.eval(d.row(0), outdata);
    // PLAJA_LOG("Actual state in evaluate is: "); state.dump(true);
    // std::cout << "Veritas num trees: " << veritas_tree << std::endl;
    for (int o = 0; o < num_outputs; ++o) {
        outputs[o] = outdata[o];
    }
    return outputs;
}

/***********************************************************************************************************************/

// TODO? * Maybe for decaying learning rate (cf. https://github.com/gandroz/rl-taxi/blob/main/pytorch/agent.py [September 2022])
/*
void QlAgentTorch::adjust_learning_rate(unsigned int episode) {
    auto lr_min = 0; // dummy
    auto delta = USING_POLICY_LEARNING::learningRate - lr_min;
    auto lr_decay_rate = 0; // dummy
    auto lr = lr_min + delta * std::exp(-episode / lr_decay_rate);
    for (auto& param: optimizer->param_groups()) {
        auto optimizer_opt = std::make_unique<torch::optim::OptimizerOptions>();
        optimizer_opt->set_lr(lr);
        param.set_options(std::move(optimizer_opt));
    }
}
 */
