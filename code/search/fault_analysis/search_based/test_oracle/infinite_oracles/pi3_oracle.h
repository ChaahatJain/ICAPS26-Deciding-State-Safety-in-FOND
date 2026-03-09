//
// This file is part of the PlaJA code base.
// Copyright (c) (2023 - 2025) Chaahat Jain, Johannes Schmalz.
//

// Intuition:
// A state s is safe if all possible policy executions are safe from s.

#ifndef PLAJA_PI3_ORACLE_H
#define PLAJA_PI3_ORACLE_H

// #include "komihash.h"
// struct Komihash {
//     std::size_t operator()(unsigned long x) const noexcept {
//         return komihash(&x, sizeof(x), 0);
//     }
// };

#include <vector>
#include <unordered_set>
#include "../infinite_oracle.h"
#include <map>
#include <algorithm>
#include <stack>
#include <utility>
#include <optional>
#include <random>


enum class PI3Mode {
    INPUT_POLICY,
    ZERO,
    RANDOM
};


template <PI3Mode PolicyInit>
class PI3Oracle final: public InfiniteOracle {
public:
    enum class StateStatus {
        SAFE,
        UNSAFE,
        CHECKING,
        TIMEOUT,
    };


private:

    bool no_safe_policy_under_substate(const StateID_type state, const std::vector<StateID_type>& path) const {
        auto const x = pi(state);
        STMT_IF_DEBUG(std::cout << "rec calls " << rec_pi_calls_ << std::endl;)
        return x == StateStatus::UNSAFE;
    }

    //      **********************************
    //      ****  InfiniteOracle methods  ****
    //      **********************************

    bool check_if_fault(const State& state, const ActionLabel_type action) const override {
        // INCREASE QUERY COUNTER
        searchStatistics->inc_attr_unsigned(PLAJA::StatsUnsigned::NUM_ORACLE_QUERIES);
        auto successors = simulatorEnv->compute_successors(state, action);
        bool fault = false;
        auto time = std::chrono::steady_clock::now();
        for (auto t_id : successors) {
            auto target = simulatorEnv->get_state(t_id);
            bool is_fault = not no_safe_policy_under_substate(
                            state.get_id_value(),
                            {state.get_id_value()}) and
                        no_safe_policy_under_substate(
                            target.get_id_value(),
                            {target.get_id_value()});
            if (is_fault) {fault = is_fault; break; }
        }
        auto diff = std::chrono::duration_cast<std::chrono::duration<double>>(std::chrono::steady_clock::now() - time).count();
        if (time_limit_reached(std::chrono::steady_clock::now())) {
            clear(!use_cache);
            searchStatistics->inc_attr_unsigned(PLAJA::StatsUnsigned::NUM_TIMEOUT);
            searchStatistics->inc_attr_double(PLAJA::StatsDouble::NO_COVERAGE_TOTAL_TIME, seconds_per_query);
            safety_deciding_algorithm_stats.push_back({false, seconds_per_query});
            return false;
        }
        // INCREASE QUERY COVERAGE
        searchStatistics->inc_attr_unsigned(PLAJA::StatsUnsigned::COVERAGE);
        if (fault) {
            searchStatistics->inc_attr_double(PLAJA::StatsDouble::YES_COVERAGE_TOTAL_TIME, diff);
            searchStatistics->inc_attr_unsigned(PLAJA::StatsUnsigned::NUM_YES);
        } else {
            searchStatistics->inc_attr_double(PLAJA::StatsDouble::NO_COVERAGE_TOTAL_TIME, diff);
            searchStatistics->inc_attr_unsigned(PLAJA::StatsUnsigned::NUM_NO);
        }
        safety_deciding_algorithm_stats.push_back({fault, diff});
        clear(!use_cache);
        return fault;
    }

    bool check_safe(const State& state) const override {
        searchStatistics->inc_attr_unsigned(PLAJA::StatsUnsigned::NUM_ORACLE_QUERIES);
        const auto time = std::chrono::steady_clock::now();
        const bool safe = not no_safe_policy_under_substate(state.get_id_value(), {state.get_id_value()});
        auto diff = std::chrono::duration_cast<std::chrono::duration<double>>(std::chrono::steady_clock::now() - time).count();
        if (time_limit_reached(std::chrono::steady_clock::now())) {
            clear(!use_cache);
            searchStatistics->inc_attr_unsigned(PLAJA::StatsUnsigned::NUM_TIMEOUT);
            searchStatistics->inc_attr_double(PLAJA::StatsDouble::NO_COVERAGE_TOTAL_TIME, seconds_per_query);
            safety_deciding_algorithm_stats.push_back({false, seconds_per_query});
            return false;
        }
        // INCREASE QUERY COVERAGE
        searchStatistics->inc_attr_unsigned(PLAJA::StatsUnsigned::COVERAGE);
        if (safe) {
            searchStatistics->inc_attr_double(PLAJA::StatsDouble::YES_COVERAGE_TOTAL_TIME, std::chrono::duration_cast<std::chrono::duration<double>>(std::chrono::steady_clock::now() - start_time).count());
            searchStatistics->inc_attr_unsigned(PLAJA::StatsUnsigned::NUM_YES);
        } else {
            searchStatistics->inc_attr_double(PLAJA::StatsDouble::NO_COVERAGE_TOTAL_TIME, std::chrono::duration_cast<std::chrono::duration<double>>(std::chrono::steady_clock::now() - start_time).count());
            searchStatistics->inc_attr_unsigned(PLAJA::StatsUnsigned::NUM_NO);
        }
        safety_deciding_algorithm_stats.push_back({safe, diff});
        STMT_IF_DEBUG(
            if (safe) {
                std::cout << "Safe policy found for state: "; state.dump(true);
            }
            if (!safe) {
            std::cout << "No safe policy found for state: "; state.dump(true);
            }
        )

        clear(!use_cache);
        return safe;
    }

    void clear(bool reset_all) const override {
        if (reset_all) {
            // std::cout << "--- CLEARING CACHE!" << std::endl;
            unsafe_.clear();
            safe_.clear();
            action_idx_.clear();
        }
    }

    //      **********************************
    //      **********  PI3 Variables *********
    //      **********************************

    mutable bool saw_unsafe_;
    mutable std::unordered_set<StateID_type> seen_;

    // action_idx_ tracks the current candidate policy of PI's search
    mutable std::unordered_map<StateID_type, size_t> action_idx_;

    mutable std::mt19937 rng_;

    // Debugging vars
    mutable size_t max_depth_seen_ = 0;
    mutable size_t rec_pi_calls_ = 0;

    //      **********************************
    //      **********  Cache  *********
    //      **********************************

    mutable std::unordered_set<StateID_type> unsafe_;
    mutable std::unordered_set<StateID_type> safe_;

    //      **********************************
    //      **********  PI3 Methods  *********
    //      **********************************

    StateStatus pi(const StateID_type state) const {
        size_t iters = 0;
        max_depth_seen_ = 0;
        rec_pi_calls_ = 0;
        while (true) {
            // std::cout << "pi iter " << iters << std::endl;

            if (time_limit_reached(std::chrono::steady_clock::now())) {
                return StateStatus::TIMEOUT;
            }

            seen_.clear();
            saw_unsafe_ = false;
            auto x = rec_pi(state, 0);
            if (!saw_unsafe_ or x == StateStatus::SAFE) {
                // std::cout << "SAFE!" << std::endl;
                STMT_IF_DEBUG(std::cout << "rec calls " << rec_pi_calls_ << std::endl;)
                // simulatorEnv->get_state(state).dump();

                // cache state as safe (in case of !saw_unsafe_)
                //
                // POTENTIAL OPTIMISATION: at this point we could mark all states in the policy envelope
                // as safe. BUT! TarjanSafe only marks the root SCC as safe in similar situations, so
                // to keep it comparable we only mark
                safe_.insert(state);

                return StateStatus::SAFE;
            }
            if (x == StateStatus::UNSAFE) {
                // std::cout << "UNSAFE!" << std::endl;
                STMT_IF_DEBUG(std::cout << "rec calls " << rec_pi_calls_ << std::endl;)
                // simulatorEnv->get_state(state).dump();
                return StateStatus::UNSAFE;
            }

            ++iters;
        }
    }


    StateStatus rec_pi(const StateID_type state, size_t const depth) const {
        ++rec_pi_calls_;

        if (time_limit_reached(std::chrono::steady_clock::now())) {
            return StateStatus::TIMEOUT;
        }


        // Base cases from cache
        if (unsafe_.find(state) != unsafe_.end()) {
            saw_unsafe_ = true;
            return StateStatus::UNSAFE;
        }
        if (safe_.find(state) != safe_.end()) {
            return StateStatus::SAFE;
        }

        // CAREFUL: this needs to be after the other checks
        // --> if a state is safe or unsafe that's more informative than seen
        //
        // Q: What does "seen" mean?
        //
        // A: It means that we have encountered a state we've seen before, i.e., we have a cycle. That
        // is interpreted as "tentatively safe" in this algorithm.
        if (seen_.find(state) != seen_.end()) {
            return StateStatus::CHECKING;
        }

        // Base cases
        auto s = simulatorEnv->get_state(state);
        if (check_goal(s) or check_terminal(s)) {
            safe_.insert(state);
            return StateStatus::SAFE;
        }
        else if (check_avoid(s)) {
            saw_unsafe_ = true;
            unsafe_.insert(state);
            return StateStatus::UNSAFE;
        }


        // Mark as seen
        seen_.insert(state);

        if (depth > max_depth_seen_) {
            max_depth_seen_ = depth;
            // std::cout << "depth = " << depth << std::endl;
        }





        // INITIALISE ACTIONS

        std::vector<ActionLabel_type> const app_actions =
            simulatorEnv->extract_applicable_actions(s, true);

        std::vector<ActionLabel_type> actions;


        // Default action order
        if constexpr (PolicyInit == PI3Mode::ZERO) {
            actions = app_actions;
        }

        // Randomise action order
        //
        // Important: the action order must always be the same for the same state
        // --> this is because the implementation tracks the candidate action by index, so if if we
        //     revisit a state and shuffle differently everything breaks
        else if constexpr (PolicyInit == PI3Mode::RANDOM) {
            actions = app_actions;
            rng_.seed(state);
            std::shuffle(actions.begin(), actions.end(), rng_);
        }

        // Sort actions according to policy
        //
        // CAREFUL: these actions may NOT BE APPLICABLE, have to check later
        else if constexpr (PolicyInit == PI3Mode::INPUT_POLICY) {
            // Based on ql_agent.cpp
            //
            // Ranking from
            // https://stackoverflow.com/questions/1577475/c-sorting-and-keeping-track-of-indexes

            auto const action_values = policy->action_values(s);

            // initialise default index order 0, 1, 2,...
            //
            // NOTE: using int rather than size_t to capture ActionLabel_type directly
            std::vector<ActionLabel_type> idxs(action_values.size());
            std::iota(idxs.begin(), idxs.end(), 0);

            // sort idx according to action_values
            std::stable_sort(idxs.begin(), idxs.end(), [&](ActionLabel_type i1, ActionLabel_type i2)
                {return action_values[i1] > action_values[i2];});

            // !!! WARNING !!!
            // should use jani2Interface_->get_output_label(ranked_action_idx); to map idxs onto action
            // labels. BUT! get_output_label is the id function in this case
            actions = idxs;

            // remove all inapplicable actions
            auto is_not_applicable = [&](ActionLabel_type const test_a) {
                auto const it = std::find(app_actions.begin(), app_actions.end(), test_a);
                return it == app_actions.end();
            };
            actions.erase(std::remove_if(actions.begin(), actions.end(), is_not_applicable),
                        actions.end());
        }

        if (actions.empty()) {
            std::cout << "!!! STRANGE EDGE CASE: no applicable actions... returning UNSAFE !!!"
                    << std::endl;
            return StateStatus::UNSAFE;
        }

        // NOTE: the first call to action_idx_[state] populates it with 0

        PLAJA_ASSERT(action_idx_[state] < actions.size());
        for (size_t a_idx = action_idx_[state]; a_idx < actions.size(); ++a_idx) {
            ActionLabel_type curr_action = actions[a_idx];

            bool all_succs_safe = true;
            auto successors = simulatorEnv->compute_successors(s, curr_action);
            for (auto succ : successors) {
                auto x = rec_pi(succ, depth+1);
                if (x == StateStatus::UNSAFE) {
                    // This action is unsafe, so go pick a better one
                    goto end_of_loop;
                }
                all_succs_safe &= (x == StateStatus::SAFE);
            }

            if (all_succs_safe) {
                safe_.insert(state);
                action_idx_[state] = a_idx;
                return StateStatus::SAFE;
            }

            action_idx_[state] = a_idx;
            return StateStatus::CHECKING;

            end_of_loop:;
        }

        // If we ran out of actions (because all of them are unsafe)
        //
        // then this state is unsafe
        unsafe_.insert(state);
        return StateStatus::UNSAFE;

    }

public:
    explicit PI3Oracle(const PLAJA::Configuration& config)
        : InfiniteOracle(config)
    {
        PLAJA_ASSERT(cost_objective_ == MAX_COST);
    }

    ~PI3Oracle() override = default;
    DELETE_CONSTRUCTOR(PI3Oracle)
};


using PI3OracleInputPolicy = PI3Oracle<PI3Mode::INPUT_POLICY>;
using PI3OracleZero = PI3Oracle<PI3Mode::ZERO>;
using PI3OracleRandom = PI3Oracle<PI3Mode::RANDOM>;


#endif //PLAJA_PI3_ORACLE_H
