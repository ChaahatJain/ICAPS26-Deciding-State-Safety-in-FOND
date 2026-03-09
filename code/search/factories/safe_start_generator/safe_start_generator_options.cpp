//
// Created by Daniel Sherbakov in 2025.
//

#include "safe_start_generator_options.h"

namespace PLAJA_OPTION {

    // Safe Start Generator
    const std::string verification_method("method");
    const std::string approximate("approximate");
    const std::string approximation_type("approximation-type");
    const std::string use_testing("use-testing");
    const std::string testing_time("testing-time");
    const std::string terminate_on_cycles("terminate-on-cycles");
    const std::string log_path("log-path");
    const std::string policy_run_sampling("policy-run-sampling");
    const std::string use_probabilistic_sampling("use-prob-sampling");
    const std::string sampling_probability("p");
    const std::string max_run_length("max-run-length");
    const std::string alternate("alternate");
    const std::string iteration_stats("iter-stats");
} // namespace PLAJA_OPTION

namespace PLAJA_OPTION_INFO {
    // Safe Start Generator
    const std::string verification_method(
        "The verification method used to verify start and unsafety conditions.\n\tsupported: inv_str, scs");
    const std::string approximate(
        "Defines which set of unsafe sets to approximate. \n\t testing - only unsafe states identified by testing are "
        "approximated. \n\t both - approximate unsafe states of verification and testing.");
    const std::string approximation_type(
        "The type of approximation to be used on the set of unsafe states. Currently performs box approximation in "
        "two variations: \n\t over - Bounds set of states with a box\n\t under - Computes maximal bounded box in set "
        "of states.");
    const std::string use_testing("Use testing to to identify unsafe paths.");
    const std::string testing_time(
        "Time limit for testing. If no unsafe path is found within this time, testing stops.");
    const std::string terminate_on_cycles("If enabled will terminate on cycles.");
    const std::string log_path("Dumps policy execution path during testing.");
    const std::string policy_run_sampling(
        "Use policy run sampling to identify unsafe paths on non-deterministic state spaces.");
    const std::string use_probabilistic_sampling(
        "Use probabilistic sampling to sample successor states in policy-run sampling.\n\tBy default greedy selection "
        "is used.");
    const std::string sampling_probability("Probability of sampling a successor state in policy-run sampling.");
    const std::string max_run_length(
        "Maximum length of policy run sampling.\n\tBy default: determined by distance function ability to discriminate "
        "states.");
    const std::string alternate("If set testing and verification iterations will alternate regardless of testing result");
    const std::string iteration_stats("Save per iteration statistics to csv file.");
} // namespace PLAJA_OPTION_INFO

namespace PLAJA_OPTION_DEFAULT {
    // Safe Start Generator
    const int testing_time = 900;
    const double sampling_probability = 1;
    const int max_run_length = -1;
} // namespace PLAJA_OPTION_DEFAULT