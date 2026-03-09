//
// This file is part of the PlaJA code base.
// Copyright (c) (2023 - 2024) Chaahat Jain.
//

// Intuition:
// Given transition s -a-> s' on unsafe policy path, try alternate actions. Check if they are safe.
// If any alternate action leads to safety, then we have detected a fault.

#ifndef PLAJA_ORACLE_H
#define PLAJA_ORACLE_H

#include "../../../parser/ast/expression/forward_expression.h"
#include "../../../stats/stats_base.h"
#include "../../fd_adaptions/state.h"
#include "../../non_prob_search/policy/policy_restriction.h"
#include "../../successor_generation/simulation_environment.h"
#include "../../using_search.h"
#include "using_fault_analysis.h"
#include <vector>
#include <chrono>
#include <set>
#include <string>
#include <sstream>
#include "../../../search/smt/model/model_z3.h"
#include "../../../search/smt/bias_functions/distance_function.h"
#include <unordered_map>
#include "policy_run_fuzzing.h"
#include "policy_run_sampler.h"

// Given transition s -a-> s' on unsafe policy path, try alternate actions.
// s-a->s' is a fault if s-a'-> t and t is safe.

class Oracle {

public:
    explicit Oracle(const PLAJA::Configuration& config); // Constructor
    virtual ~Oracle() = default;
    enum Status {
        BUG,
        NOT_BUG,
        FAULT_ON_PATH,
        NO_FAULT_ON_PATH,
        GOAL,
        AVOID,
        TERMINAL,
        UNDONE
    };

    enum STATE_SAFETY {
        SAFE,
        UNSAFE,
        TIMEOUT,
        FAULT,
        NOT_FAULT
    };


    enum class SEARCH_DIRECTION {
        FORWARD,
        BACKWARD
    };

    enum CostObjective {
        MAX_COST,
        SUM_COST,
        ENVELOPE_COST
    };

    enum class ORACLE_MODES {
        INFINITE,
        DSMC,
        FINITE
    };

    enum class EXPERIMENT_MODE {
        START_STATE_BUG_CHECK, // run oracle on start state of unsafe path to determine if it is a bug
        ENTIRE_PATH_BUG_CHECK, // run oracle on all states of unsafe path to determine all bugs
        FAULT_LOCALIZATION, // run oracle to localize faults on unsafe path
        IGNORE, // oracle ignores unsafe path
    };

protected:
    /* Variable Hyperparameters. */
    SEARCH_DIRECTION search_direction_;
    unsigned int maxFaults;
    CostObjective cost_objective_;
    bool use_cache = false;
    bool cacheAcrossPaths = false;
    double seconds_per_query;
    EXPERIMENT_MODE experiment_mode;

    /* Search specification. */
    const ObjectiveExpression* objective;
    const Expression* avoid;
    std::shared_ptr<SimulationEnvironment> simulatorEnv;
    std::shared_ptr<PolicyRestriction> policy;
    std::shared_ptr<PLAJA::StatsBase> searchStatistics;

    /* Keeping track of current policy path */
    // FCT_IF_DEBUG(
        void print_policy_path(const std::vector<StateID_type> policy_path) const {
        std::cout << "################# Policy Path: ###########################" << std::endl;
        for (auto s: policy_path) {
            simulatorEnv->get_state(s).dump();
        }
        std::cout << "################ Policy Path finished #####################" << std::endl;
    };
    // )
    std::string get_policy_path_str(std::vector<StateID_type> policy_path) const {
        std::string pathString = "[";
        for (int i = 0;  i < policy_path.size(); i++) {
            pathString += simulatorEnv->get_state(policy_path[i]).save_as_str() + "~ ";
        }
        pathString += "]";
        return pathString;
    } 

/* Keeping track of performance of safety deciding algorithms */
    struct AlgorithmStats { bool result; double search_time; };
    mutable std::chrono::time_point<std::chrono::steady_clock> start_time;
    mutable std::vector<AlgorithmStats> safety_deciding_algorithm_stats;
    std::string get_safety_deciding_algorithm_stats_str() const {
        std::ostringstream oss;
        oss << "["; // Start with an opening bracket

        for (size_t i = 0; i < safety_deciding_algorithm_stats.size(); ++i) {
            oss << "(" << (safety_deciding_algorithm_stats[i].result ? "true" : "false") << ": " << safety_deciding_algorithm_stats[i].search_time << ")";
            if (i != safety_deciding_algorithm_stats.size() - 1) {
                oss << "~ "; // Add comma between elements
            }
        }

        oss << "]"; // Closing bracket
        return oss.str();
    }

    // Policy shield support
    mutable std::unordered_map<StateID_type, std::vector<ActionLabel_type>> faults_found;
    ActionLabel_type get_policy_action(const State& state) const;

    /* Termination Criteria */
    [[nodiscard]] bool check_avoid(const State& state) const;
    [[nodiscard]] static bool check_cycle(StateID_type state, std::vector<StateID_type> states) ;
    [[nodiscard]] bool check_goal(const State& state) const;
    [[nodiscard]] bool check_terminal(const State& state) const;

    /* Calls to the Oracle */
    bool search_fault_on_path(PolicySampled::Path path);
    bool is_initial_state_bug(PolicySampled::Path path);
    bool are_states_on_path_bugs(PolicySampled::Path path);

    /* Calls to the safety deciding algorithms */
    virtual bool check_safe(const State& state) const = 0;
    [[nodiscard]] virtual bool check_if_fault(const State& state, const ActionLabel_type action) const = 0;
    virtual void clear(bool reset_all) const = 0;
    bool time_limit_reached(const std::chrono::time_point<std::chrono::steady_clock> current_time) const { return std::chrono::duration_cast<std::chrono::seconds>(current_time - start_time).count() >= seconds_per_query; }

public:
    [[nodiscard]] Status get_status(const State& state) const;

    Oracle::Status check(PolicySampled::Path path);

    Oracle::STATE_SAFETY is_state_safe(StateID_type state_id);
    Oracle::STATE_SAFETY is_state_action_fault(StateID_type state_id, ActionLabel_type action);

    // Oracle initializer
    void initialize_oracle(const std::shared_ptr<SimulationEnvironment>& env, const std::shared_ptr<PolicyRestriction>& pol, const ObjectiveExpression* goal, const Expression* reach, const std::shared_ptr<PLAJA::StatsBase>& stats) {
        simulatorEnv = env;
        policy = pol;
        objective = goal;
        avoid = reach;
        searchStatistics = stats;
    }

    /* */
    void set_policy(const std::shared_ptr<PolicyRestriction>& pol) { policy = pol; }
};

/* Factory. */
namespace FA_ORACLE {
    extern std::unique_ptr<Oracle> construct(const PLAJA::Configuration& config);
}

namespace PLAJA_OPTION {

    extern const std::string oracle_mode;
    extern const std::string max_depth;
    extern const std::string search_dir;
    extern const std::string num_faults_per_path;
    extern const std::string cost_objective;
    extern const std::string cache_fault_search;
    extern const std::string retain_fault_cache;
    extern const std::string seconds_per_query;
    extern const std::string fault_experiment_mode;

    namespace FA_ORACLE {
        const std::unordered_map<std::string, Oracle::EXPERIMENT_MODE> stringToExperiment {
            {"individualSearch", Oracle::EXPERIMENT_MODE::START_STATE_BUG_CHECK},
            {"individualAll", Oracle::EXPERIMENT_MODE::ENTIRE_PATH_BUG_CHECK},
            {"aggregate", Oracle::EXPERIMENT_MODE::FAULT_LOCALIZATION},
            {"ignore", Oracle::EXPERIMENT_MODE::IGNORE}
        };
        const std::unordered_map<std::string, Oracle::CostObjective> stringToObjective {
            // NOLINT(cert-err58-cpp)
            {"MaxCost", Oracle::CostObjective::MAX_COST},
            {"SumCost", Oracle::CostObjective::SUM_COST},
            {"EnvelopeCost", Oracle::CostObjective::ENVELOPE_COST},
        };

        const std::unordered_map<std::string, Oracle::ORACLE_MODES> stringToOracle {
            // NOLINT(cert-err58-cpp)
            {"finite", Oracle::ORACLE_MODES::FINITE},
            {"dsmc", Oracle::ORACLE_MODES::DSMC},
            {"infinite", Oracle::ORACLE_MODES::INFINITE},
        };

        const std::unordered_map<std::string, Oracle::SEARCH_DIRECTION> stringToSearch {
            // NOLINT(cert-err58-cpp)
            {"forward", Oracle::SEARCH_DIRECTION::FORWARD},
            {"backward", Oracle::SEARCH_DIRECTION::BACKWARD},
        };
        extern void add_options(PLAJA::OptionParser& option_parser);
        extern void print_options();
    }// namespace FA_ORACLE

}// namespace PLAJA_OPTION

#endif//PLAJA_ORACLE_H
