//
// This file is part of the PlaJA code base.
// Copyright (c) (2019 - 2021) Marcel Vinzent.
// See README.md in the top-level directory for licensing information.
//

#ifndef PLAJA_USING_POLICY_LEARNING_H
#define PLAJA_USING_POLICY_LEARNING_H

#include <vector>

namespace TORCH_IN_PLAJA { using Floating = float; }

namespace at { class Tensor; }

namespace torch {

    using at::Tensor;

    namespace optim { class Adam; }

    namespace nn { class Linear; }
}

namespace USING_POLICY_LEARNING {

    /* Static hyperparameters. */
    constexpr unsigned int bufferSize = unsigned(1e4); // Replay buffer size.
    constexpr unsigned int batchSize = 64; // Minibatch size.
    constexpr TORCH_IN_PLAJA::Floating gamma = 0.99; // Discount factor.
    constexpr TORCH_IN_PLAJA::Floating tau = 0.001; // For soft update of target parameters.
    constexpr TORCH_IN_PLAJA::Floating learningRate = 5e-4; // Learning rate.
    constexpr unsigned int updateEvery = 4; // How often to update the network.

    using NNInput_type = int;
    using NNAction_type = int64_t;
    using Reward_type = TORCH_IN_PLAJA::Floating;
    using NNState = std::vector<TORCH_IN_PLAJA::Floating>;

    constexpr NNAction_type noneAction = static_cast<NNAction_type>(-1);
}

#endif //PLAJA_USING_POLICY_LEARNING_H
