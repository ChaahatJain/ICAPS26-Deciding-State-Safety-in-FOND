//
// This file is part of the PlaJA code base.
// Copyright (c) (2019 - 2021) Marcel Vinzent.
// See README.md in the top-level directory for licensing information.
//

#ifndef PLAJA_REPLAY_BUFFER_H
#define PLAJA_REPLAY_BUFFER_H

#include <vector>
#include <ATen/Tensor.h>
#include "../utils/default_constructors.h"
#include "using_policy_learning.h"

namespace REPLAY_BUFFER { struct Sample; }

class ReplayBuffer {

private:
    unsigned int batchSize;
    unsigned int maxBufferSize;

    class RandomNumberGenerator* rng;

    /* Buffer. */
    std::vector<REPLAY_BUFFER::Sample> buffer;
    unsigned int firstOut;
    std::vector<unsigned int> shuffleBuffer;

    // samples
    torch::Tensor statesSample;
    torch::Tensor nextStatesSample;
    torch::Tensor actionsSample;
    torch::Tensor nextStateApplicabilitySample;
    torch::Tensor rewardsSample;
    torch::Tensor doneSample;

    void shuffle();

public:
    ReplayBuffer(unsigned int batch_size, unsigned int max_buffer_size, unsigned int nn_state_size, long int num_actions);
    ~ReplayBuffer();
    DELETE_CONSTRUCTOR(ReplayBuffer)

    [[nodiscard]] std::size_t size() const; // inlining not possible when Sample struct undefined in header

    void add_sample(const USING_POLICY_LEARNING::NNState& nn_state, const USING_POLICY_LEARNING::NNState& next_nn_state,
                    USING_POLICY_LEARNING::NNAction_type action, std::list<USING_POLICY_LEARNING::NNAction_type>& next_state_applicability,
                    USING_POLICY_LEARNING::Reward_type reward, bool done);

    void sample();

    inline const torch::Tensor& states_sample() { return statesSample; }

    inline const torch::Tensor& next_states_sample() { return nextStatesSample; }

    inline const torch::Tensor& actions_sample() { return actionsSample; }

    inline const torch::Tensor& next_state_applicability_sample() { return nextStateApplicabilitySample; }

    inline const torch::Tensor& rewards_sample() { return rewardsSample; }

    inline const torch::Tensor& done_sample() { return doneSample; }

};

#endif //PLAJA_REPLAY_BUFFER_H
