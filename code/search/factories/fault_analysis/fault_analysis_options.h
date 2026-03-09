//
// This file is part of the PlaJA code base.
// Copyright (c) (2023 - 2024) Chaahat Jain.
//

#ifndef PLAJA_FAULT_ANALYSIS_OPTIONS_H
#define PLAJA_FAULT_ANALYSIS_OPTIONS_H

#include <string>

namespace PLAJA_OPTION {

    extern const std::string unsafe_path;
    extern const std::string region;
    extern const std::string reachable_path;
    extern const std::string reachable_prop;
    extern const std::string oracle_use;

    extern const std::string num_starts;
    extern const std::string num_paths_per_start;

    extern const std::string use_cegar;
    extern const std::string ic3_logs;
    
    extern const std::string retraining_train_test_split;
    extern const std::string evaluation_set;
    extern const std::string check_policy;
    extern const std::string paths_to_sample;
}

namespace PLAJA_OPTION_DEFAULT {
    extern const std::string region;

    constexpr bool reachable_prop(false);
    constexpr int num_starts(10000);
    constexpr int num_paths_per_start(1);

    constexpr bool use_cegar(false);
    constexpr double retraining_train_test_split(1.0);
    constexpr bool evaluation_set(false);
}


#endif //PLAJA_FAULT_ANALYSIS_OPTIONS_H
