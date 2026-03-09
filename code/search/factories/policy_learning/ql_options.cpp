//
// This file is part of the PlaJA code base.
// Copyright (c) (2019 - 2022) Marcel Vinzent.
// See README.md in the top-level directory for licensing information.
//

#include "ql_options.h"

namespace PLAJA_OPTION {

    // Policy Learning
    const std::string evaluation_mode("evaluation-mode"); // NOLINT(cert-err58-cpp)
    const std::string load_nn("load-nn"); // NOLINT(cert-err58-cpp)
    const std::string load_ensemble("load-ensemble"); // NOLINT(cert-err58-cpp)
    const std::string save_backup("save-backup"); // NOLINT(cert-err58-cpp)
    const std::string save_agent_actions("save-agent-actions");
    const std::string save_policy_path("save-policy-path");
    const std::string postfix("postfix");
    const std::string save_nnet("save-nnet"); // NOLINT(cert-err58-cpp)
    const std::string reward_nn("reward-nn");
    const std::string reward_nn_layers("reward-nn-layers");
    const std::string reward_lambda("reward-lambda");
    const std::string num_episodes("num-episodes"); // NOLINT(cert-err58-cpp)
    const std::string max_start_states("max-start-states");
    const std::string max_length_episode("max-length-episode"); // NOLINT(cert-err58-cpp)
    const std::string terminate_cycles("terminate-cycles"); // NOLINT(cert-err58-cpp)
    const std::string epsilon_start("epsilon-start"); // NOLINT(cert-err58-cpp)
    const std::string epsilon_decay("epsilon-decay"); // NOLINT(cert-err58-cpp)
    const std::string epsilon_end("epsilon-end"); // NOLINT(cert-err58-cpp)
    const std::string grad_clip("grad-clip");
    const std::string l1_lambda("l1-lambda");
    const std::string l2_lambda("l2-lambda");
    const std::string group_lambda("group-lambda");
    const std::string kl_lambda("kl-lambda");
    const std::string goal_reward("goal-reward"); // NOLINT(cert-err58-cpp)
    const std::string avoid_reward("avoid-reward"); // NOLINT(cert-err58-cpp)
    const std::string terminal_reward("terminal-reward"); // NOLINT(cert-err58-cpp)
    const std::string stalling_reward("stalling-reward"); // NOLINT(cert-err58-cpp)
    const std::string unsafe_reward("unsafe-reward"); // NOLINT(cert-err58-cpp)
    const std::string groundStartPotential("ground-start-potential"); // NOLINT(cert-err58-cpp)
    const std::string groundDonePotential("ground-done-potential"); // NOLINT(cert-err58-cpp)
    const std::string minimal_score("minimal-score"); // NOLINT(cert-err58-cpp)
    const std::string slicingSize("slicing-size"); // NOLINT(cert-err58-cpp)

    /* Policy teacher. */
    const std::string teacherModel("teacher-model"); // NOLINT(cert-err58-cpp)
    const std::string teacherGreediness("teacher-greediness"); // NOLINT(cert-err58-cpp)
    const std::string teacherExplore("teacher-explorer"); // NOLINT(cert-err58-cpp)

}
