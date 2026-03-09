//
// Created by Daniel Sherbakov in 2025.
//

#ifndef SAFE_START_GENERATOR_OPTIONS_H
#define SAFE_START_GENERATOR_OPTIONS_H

#include <string>

namespace PLAJA_OPTION {
    extern const std::string verification_method;
    extern const std::string alternate;
    extern const std::string use_testing;
    extern const std::string approximate;
    extern const std::string approximation_type;
    extern const std::string testing_time;
    extern const std::string terminate_on_cycles;
    extern const std::string log_path;
    extern const std::string policy_run_sampling;
    extern const std::string use_probabilistic_sampling;
    extern const std::string sampling_probability;
    extern const std::string max_run_length;
    extern const std::string iteration_stats;
}

namespace PLAJA_OPTION_INFO {
    extern const std::string verification_method;
    extern const std::string alternate;
    extern const std::string use_testing;
    extern const std::string approximate;
    extern const std::string approximation_type;
    extern const std::string testing_time;
    extern const std::string terminate_on_cycles;
    extern const std::string log_path;
    extern const std::string policy_run_sampling;
    extern const std::string use_probabilistic_sampling;
    extern const std::string sampling_probability;
    extern const std::string max_run_length;
    extern const std::string iteration_stats;
}

namespace PLAJA_OPTION_DEFAULT {
    extern const int testing_time;
    extern const double sampling_probability;
    extern const int max_run_length;
} // namespace PLAJA_OPTION_DEFAULT


#endif //SAFE_START_GENERATOR_OPTIONS_H
