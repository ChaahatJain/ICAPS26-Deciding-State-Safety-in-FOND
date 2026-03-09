//
// This file is part of the PlaJA code base.
// Copyright (c) (2019 - 2021) Marcel Vinzent.
// See README.md in the top-level directory for licensing information.
//

#include <torch/types.h>
#include "replay_buffer.h"
#include "../utils/rng.h"
#include "../utils/utils.h"

namespace REPLAY_BUFFER {

    struct Sample {
        torch::Tensor nnState;
        torch::Tensor nextNnState;
        USING_POLICY_LEARNING::NNAction_type action;
        torch::Tensor nextStateApplicability;
        USING_POLICY_LEARNING::Reward_type reward;
        bool done;

        Sample(const USING_POLICY_LEARNING::NNState& nn_state, const USING_POLICY_LEARNING::NNState& next_nn_state, const USING_POLICY_LEARNING::NNAction_type action_, std::list<USING_POLICY_LEARNING::NNAction_type>& next_state_applicability, long int num_actions, USING_POLICY_LEARNING::Reward_type reward_, bool done_):
            nnState(torch::zeros(nn_state.size()))
            , nextNnState(torch::zeros(nn_state.size()))
            , action(action_)
            , nextStateApplicability(torch::zeros(num_actions, { torch::kI8 }))
            , reward(reward_)
            , done(done_) {

            PLAJA_ASSERT(nn_state.size() == next_nn_state.size())

            const auto state_size = PLAJA_UTILS::cast_numeric<long int>(nn_state.size());
            for (long int state_index = 0; state_index < state_size; ++state_index) {
                nnState[state_index] = nn_state[state_index];
                nextNnState[state_index] = next_nn_state[state_index];
            }

            for (const USING_POLICY_LEARNING::NNAction_type app_action: next_state_applicability) { nextStateApplicability[app_action] = 1; }

        }

    };

}

ReplayBuffer::ReplayBuffer(unsigned int batch_size, unsigned int max_buffer_size, unsigned int nn_state_size, long int num_actions):
    batchSize(batch_size)
    , maxBufferSize(max_buffer_size)
    , rng(PLAJA_GLOBAL::rng.get())
    , firstOut(0)
    , statesSample(torch::zeros({ batch_size, nn_state_size }))
    , nextStatesSample(torch::zeros({ batch_size, nn_state_size }))
    , actionsSample(torch::zeros({ batch_size }, { torch::kI64 }))
    , nextStateApplicabilitySample(torch::zeros({ batch_size, num_actions }))
    , rewardsSample(torch::zeros({ batch_size }))
    , doneSample(torch::zeros({ batch_size })) {

}

ReplayBuffer::~ReplayBuffer() = default;

/***********************************************************************************************************************/

void ReplayBuffer::shuffle() {
    PLAJA_ASSERT(shuffleBuffer.size() == buffer.size())

    const auto l = PLAJA_UTILS::cast_numeric<int>(shuffleBuffer.size());

    for (int i = 0; i < batchSize; ++i) {
        std::swap(shuffleBuffer[i], shuffleBuffer[rng->index(l - i) + i]);
    }

}

std::size_t ReplayBuffer::size() const { return buffer.size(); }

void ReplayBuffer::add_sample(const USING_POLICY_LEARNING::NNState& nn_state,
                              const USING_POLICY_LEARNING::NNState& next_nn_state,
                              USING_POLICY_LEARNING::NNAction_type action,
                              std::list<USING_POLICY_LEARNING::NNAction_type>& next_state_applicability,
                              USING_POLICY_LEARNING::Reward_type reward,
                              bool done) {

    STMT_IF_DEBUG(static bool hit_max_buffer_size(false);)
    if (buffer.size() >= maxBufferSize) {
        STMT_IF_DEBUG(hit_max_buffer_size = true;)

        buffer[firstOut] = { nn_state, next_nn_state, action, next_state_applicability, nextStateApplicabilitySample.size(1), reward, done };
        firstOut = (firstOut + 1) % maxBufferSize;

    } else {
        PLAJA_ASSERT(not hit_max_buffer_size)

        buffer.emplace_back(nn_state, next_nn_state, action, next_state_applicability, nextStateApplicabilitySample.size(1), reward, done);
        shuffleBuffer.push_back(buffer.size() - 1);
    }
}

void ReplayBuffer::sample() {
    PLAJA_ASSERT(not buffer.empty())

    // Construct buffer to shuffle the samples.
    // std::vector<const REPLAY_BUFFER::Sample*> shuffle_buffer;
    // shuffle_buffer.reserve(buffer.size());
    // for (const auto& sample: buffer) { shuffle_buffer.push_back(&sample); }

    shuffle();

    for (int i = 0; i < batchSize; ++i) {

        /* Add to samples. */
        const auto& sample = buffer[shuffleBuffer[i]];
        statesSample[i] = sample.nnState;
        nextStatesSample[i] = sample.nextNnState;
        actionsSample[i] = sample.action;
        nextStateApplicabilitySample[i] = sample.nextStateApplicability;
        rewardsSample[i] = sample.reward;
        doneSample[i] = sample.done;
    }

    // useful:
    // states_tensor = torch::from_blob(states.data(), {num_states, states_size}); //, options);
    // states_tensor = torch::cat({states_tensor, state_tensor_1.reshape({1,state_size}), state_tensor_2.reshape({1,state_size})}, 0);
    // torch::nn::utils::rnn::pad_sequence({state_tensor_1, ..., state_tensor_n}, 1);

}