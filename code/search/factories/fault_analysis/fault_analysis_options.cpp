//
// This file is part of the PlaJA code base.
// Copyright (c) (2023 - 2024) Chaahat Jain.
//

#include "fault_analysis_options.h"

namespace PLAJA_OPTION {

    const std::string unsafe_path("unsafe-path"); // NOLINT(cert-err58-cpp)
    const std::string region("region"); // NOLINT(cert-err58-cpp)
    const std::string reachable_path("reachable-path"); // NOLINT(cert-err58-cpp)
    const std::string reachable_prop("reachable-prop"); // NOLINT(cert-err58-cpp)
    const std::string oracle_use("oracle-use"); // NOLINT(cert-err58-cpp)
    
    const std::string num_starts("num-starts"); // NOLINT(cert-err58-cpp)
    const std::string num_paths_per_start("num-paths-per-start");

    /* Use verification to generate unsafe paths */
    const std::string use_cegar("use-cegar"); // NOLINT(cert-err58-cpp)
    const std::string ic3_logs("ic3-logs"); // NOLINT(cert-err58-cpp)

    /* Used during retraining to compute bug fraction and split start state set into training and evaluation sets. */
    const std::string compute_bug_fraction_exhaustively("compute-bug-fraction-exhaustively"); // NOLINT(cert-err58-cpp)
    const std::string compute_bug_fraction_approximately("compute-bug-fraction-approximately"); // NOLINT(cert-err58-cpp)
    const std::string retraining_train_test_split("retraining-train-test-split"); // NOLINT(cert-err58-cpp)
    const std::string evaluation_set("evaluation-set"); // NOLINT(cert-err58-cpp)
    const std::string compute_policy_performance("compute-policy-performance"); // NOLINT(cert-err58-cpp)
    const std::string compute_policy_performance_approximately("compute-policy-performance-approximately"); // NOLINT(cert-err58-cpp)
}