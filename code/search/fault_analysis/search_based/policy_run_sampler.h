// This file is used to generate Test Cases (Unsafe runs). Given a start state, simulate a policy and report how the unsafe runs

// Components:
// * Fuzzing Algorithm: Given a state and an action, what is the next state to be taken
// * Statistics Handling

#ifndef PLAJA_POLICY_RUN_SAMPLER_H
#define PLAJA_POLICY_RUN_SAMPLER_H

#include "../../../stats/stats_base.h"

#include "../../fd_adaptions/state.h"
#include "../../using_search.h"

#include "../../non_prob_search/policy/policy_restriction.h"
#include "../../successor_generation/simulation_environment.h"
#include "policy_run_fuzzing.h"
#include "search_statistics_fault.h"

#include "../../predicate_abstraction/cegar/pa_cegar.h"

#include <unordered_map>
#include <optional>
#include <algorithm>
#include <cmath>
#include <numeric>
#include <stack>
#include <fstream>
#include <json.hpp>
#include "using_fault_analysis.h"

using json = nlohmann::json;

class PolicyRunSampler {

    private:
        int maxLengthEpisode = 1000;
        double rand_prob = 1;
        int sampling_lookahead = -1;
        bool prob_sampling = false;

    // Things needed:
    /*
    1. Policy
    2. Simulation Environment
    3. Fuzzing Algorithm
    4. Statistics
    5. A shield (if necessary)
    6. A verification algorithm (if build_pa available)
    */

        std::shared_ptr<PolicyRestriction> policy;
        std::shared_ptr<SimulationEnvironment> simulatorEnv;
        const ObjectiveExpression* objective;
        const Expression* avoid;
        std::unique_ptr<PolicyRunFuzzing> fuzzer; 
        std::shared_ptr<PLAJA::StatsBase> searchStatistics;
        mutable std::unordered_map<StateID_type, std::vector<ActionLabel_type>> policy_shield;

        #ifdef BUILD_PA
            std::unique_ptr<PACegar> pa_cegar;
            const PLAJA::Configuration& config;
            std::unique_ptr<PropertyInformation> sub_prop_info;
        #endif

        /* Termination Criteria */
        [[nodiscard]] bool check_avoid(const State& state) const;
        [[nodiscard]] static bool check_cycle(StateID_type state, std::vector<StateID_type> states) ;
        [[nodiscard]] bool check_goal(const State& state) const;
        [[nodiscard]] bool check_terminal(const State& state) const;

       

        // Given a state, action compute the successor (To be obtained through fuzzing algorithm)
        std::unique_ptr<State> compute_successor(const State& state, const ActionLabel_type action_label) const;
        StateID_type uniform_sampling(StateID_type state, const ActionLabel_type action) const;


        // Helper Function to change a std::vector<StateValues> to a Path
        PolicySampled::Path get_path_from_state_values(std::vector<StateValues> state_values) const;

         // Helper function to get all safe states reachable by a policy under s
        bool is_state_safe_under_policy(StateID_type s, std::set<StateID_type>& visiting, std::unordered_map<StateID_type, bool>& memo, std::set<StateID_type>& safe_states);

    public:
        explicit PolicyRunSampler(const PLAJA::Configuration& config, const std::shared_ptr<SimulationEnvironment>& env, const std::shared_ptr<PolicyRestriction>& pol, const ObjectiveExpression* goal, const Expression* reach, const std::shared_ptr<PLAJA::StatsBase>& stats, const std::shared_ptr<Bias::DistanceFunction> distance_function); // Constructor
        ~PolicyRunSampler() = default;
        DELETE_CONSTRUCTOR(PolicyRunSampler)
        
        // Load policy shield from a list of faults in a log file
        void load_policy_shield(std::string faults_path);

        // Load shield through an unordered map representing a set of faults
        void set_policy_shield(std::unordered_map<StateID_type, std::vector<ActionLabel_type>> faults);

        
        // Sample one path from the policy
        std::pair<PolicySampled::Path, PolicySampled::Stats> sample_path(StateID_type state_id);
        
        // Enumerate all policy paths
        PolicySampled::Stats enumerate_all_paths(StateID_type state_id, std::vector<StateID_type> path = {});

        // Compute policy envelope till first fail state is found
        bool search_policy_envelope(std::unique_ptr<State> state);

        // Search policy envelope for fail states for a vector of start states
        int search_envelope_for_fail_states(std::vector<StateID_type> startStates);
        
        // Compute the set of safe states in the policy envelope from a given state.
        std::set<StateID_type> get_safe_states_in_policy_envelope(StateID_type state);

        // Load an unsafe path given a log file with a specific structure.
        std::optional<PolicySampled::Path> get_unsafe_path_from_logs(const std::string& filename);

         // Given a state, get the policy action
        ActionLabel_type get_policy_action(const State& state) const;

        // Functions to return an unsafe path given a verification algorithm (if build_pa available)
        #ifdef BUILD_PA
            std::optional<PolicySampled::Path> run_verification(const Expression& start, const Expression& unsafety);
            void init_pa_cegar(const Expression& start);
        #endif

};


namespace PLAJA_OPTION {
    extern const std::string rand_prob;
    extern const std::string distance_func;
    extern const std::string use_prob_sampling;
    extern const std::string sampling_lookahead;
    extern const std::string max_steps_per_start;


    namespace POLICY_RUN_SAMPLER {
        extern void add_options(PLAJA::OptionParser& option_parser);
        extern void print_options();
    } // namespace POLICY_RUN_SAMPLER

}// namespace PLAJA_OPTION


#endif //PLAJA_POLICY_RUN_SAMPLER_H
