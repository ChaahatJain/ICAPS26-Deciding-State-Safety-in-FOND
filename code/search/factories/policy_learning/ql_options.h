//
// This file is part of the PlaJA code base.
// Copyright (c) (2019 - 2022) Marcel Vinzent.
// See README.md in the top-level directory for licensing information.
//

#ifndef PLAJA_QL_AGENT_OPTIONS_H
#define PLAJA_QL_AGENT_OPTIONS_H

#include <string>
#include <cstdint>

namespace PLAJA_OPTION {

    // Policy Learning
    extern const std::string evaluation_mode;
    extern const std::string load_nn;
    extern const std::string load_ensemble;
    extern const std::string save_backup;
    extern const std::string save_policy_path;
    extern const std::string postfix;
    extern const std::string save_agent_actions;
    extern const std::string save_nnet;
    extern const std::string reward_nn;
    extern const std::string reward_nn_layers;
    extern const std::string num_episodes;
    extern const std::string max_start_states;
    extern const std::string max_length_episode;
    extern const std::string terminate_cycles;
    extern const std::string epsilon_start;
    extern const std::string epsilon_decay;
    extern const std::string epsilon_end;
    extern const std::string grad_clip;
    extern const std::string l1_lambda;
    extern const std::string l2_lambda;
    extern const std::string group_lambda;
    extern const std::string kl_lambda;
    extern const std::string reward_lambda;
    extern const std::string goal_reward;
    extern const std::string avoid_reward;
    extern const std::string stalling_reward;
    extern const std::string terminal_reward;
    extern const std::string unsafe_reward;
    extern const std::string groundStartPotential;
    extern const std::string groundDonePotential;
    extern const std::string minimal_score;
    extern const std::string slicingSize;

    /* Policy teacher. */
    extern const std::string teacherModel;
    extern const std::string teacherGreediness;
    extern const std::string teacherExplore;
}

namespace PLAJA_OPTION_DEFAULT {

    constexpr unsigned int numEpisodes = 20000;
    constexpr unsigned int maxStartStates = INT32_MAX;
    constexpr unsigned int maxLengthEpisode = 1000;
    constexpr double epsilonStart = 1;
    constexpr double epsilonDecay = 0.999;
    constexpr double epsilonEnd = 0.001;
    constexpr bool grad_clip = false;
    constexpr double l1_lambda = 0;
    constexpr double l2_lambda = 0;
    constexpr double group_lambda = 0;
    constexpr double kl_lambda = 0;
    constexpr double reward_lambda = 0.01;
    constexpr double goalReward = 200;
    constexpr double avoidReward = -100;
    constexpr double stallingReward = -100;
    constexpr double terminalReward = -100;
    constexpr double unsafeReward = 0;
    constexpr bool groundStartPotential = false;
    constexpr bool groundDonePotential = false;
    constexpr double minimalScore = -1000;
    constexpr unsigned int slicingSize = 100;

    constexpr double teacherGreediness = 0.5;
    constexpr bool teacherExplore(true);
}

#endif //PLAJA_QL_AGENT_OPTIONS_H
