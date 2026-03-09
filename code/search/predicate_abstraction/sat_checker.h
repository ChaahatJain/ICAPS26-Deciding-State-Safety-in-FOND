//
// This file is part of the PlaJA code base (2019 - 2020).
// Generic SatChecker class
//

#ifndef PLAJA_SAT_CHECKER_H
#define PLAJA_SAT_CHECKER_H

#include <list>
#include <vector>
#include <memory>
#include "../../include/pa_config_const.h"
#include "../../stats/forward_stats.h"
#include "../../assertions.h"
#include "../factories/forward_factories.h"
#include "../information/jani2nnet/forward_jani_2_nnet.h"
#include "../information/jani2nnet/using_jani2nnet.h"
#include "../information/jani_2_interface.h"
#include "../using_search.h"
#include "../smt/base/forward_solution_check.h"
#include "../smt_nn/forward_smt_nn.h"
#include "../states/forward_states.h"
#include "pa_states/forward_pa_states.h"
#include "solution_checker_pa.h"

class AdversarialAttack;

/** Sat checking for PPA. */
class SatChecker {
    friend struct SatCheckerFactory; //
    friend class SatCheckerTest;

protected:
    // construction data
    const Jani2Interface* jani2Interface;
    const SolutionCheckerPa* solutionChecker;

    // checking modes
#ifdef USE_ADVERSARIAL_ATTACK
    std::unique_ptr<AdversarialAttack> adversarialAttack;
    bool attackSolutionGuess;
#endif

    // local data
    PLAJA::StatsBase* sharedStatistics;

    // cache
    ActionLabel_type set_action_label;
    OutputIndex_type set_output_index;
    ActionOpID_type set_action_op_id;
    UpdateIndex_type set_update_index;
    std::unique_ptr<const AbstractState> set_source_state;
    std::unique_ptr<const AbstractState> set_target_state;

    std::unique_ptr<SolutionCheckerInstance> lastSolution;

#ifdef ENABLE_QUERY_CACHING
    std::unique_ptr<QueryToJson> queryToJson;
#endif

    virtual void save_solution_if(bool rlt) = 0;

    virtual bool check_() = 0; // internal auxiliary function to reduce code duplication

public:
    SatChecker(const Jani2Interface& jani_2_interface, const PLAJA::Configuration& config);
    virtual ~SatChecker();
    DELETE_CONSTRUCTOR(SatChecker)

    inline void set_solution_checker(const SolutionCheckerPa* solution_checker) { solutionChecker = solution_checker; }

    virtual void set_op_applicability_cache(std::shared_ptr<OpApplicability>& op_app) = 0;

    [[nodiscard]] const Jani2Interface& _jani_2_interface() const { return *jani2Interface; }

    virtual void increment() = 0;

    virtual void push() = 0;
    virtual void pop() = 0;

    void set_label(ActionLabel_type action_label);
    void unset_label();
    virtual void set_operator(ActionOpID_type action_op_id) = 0;
    void unset_operator();
    void set_update(UpdateIndex_type update_index);
    void unset_update();

#ifdef ENABLE_TERMINAL_STATE_SUPPORT
    virtual void add_non_terminal_condition() = 0;
#else
    inline void add_non_terminal_condition() { PLAJA_ABORT }
#endif

    /**
     * Add constraints res. predicates of the source state.
     * In particular also location values if in usage.
     * @param source
     */
    virtual void add_source_state(const AbstractState& source) = 0;
    void unset_source_state();

    /**
     * @return true if the guard is not trivially true (and thus the query has been modified).
     */
    virtual bool add_guard() = 0;

    /**
     * @return true if guard is not trivially true (and thus the query has *not* become trivially unsat).
     */
    virtual bool add_guard_negation() = 0;

    virtual void add_update() = 0;
    virtual void add_target_state(const AbstractState& target) = 0;
    void unset_target_state();

    /**
     * @return false if the query has become unsat.
     */
    virtual bool add_output_interface() = 0; // internally prior to each check

    [[nodiscard]] bool is_action_selectable(ActionLabel_type action_label) const; // short-cut to jani2interface

    [[nodiscard]] inline bool has_solution() const { return lastSolution != nullptr; }

    virtual bool has_solution_box() = 0; 

    [[nodiscard]] const StateValues& get_last_solution() const;

    [[nodiscard]] std::unique_ptr<StateValues> release_solution();

    [[nodiscard]] const StateValues* get_target_solution() const;

    [[nodiscard]] std::unique_ptr<StateValues> release_target_solution();

    /**
     * Check whether the current input query is sat (true) or unsat (false).
     * @param unknown, value to return in case input query could not be solved.
     * @return
     */
    bool check();
    virtual bool check_relaxed() = 0;

#ifdef ENABLE_STALLING_DETECTION
    void stalling_detection(const State& source);
#endif

#ifdef USE_ADVERSARIAL_ATTACK
    bool adversarial_attack();
#endif

    void print_statistics();

};

namespace SAT_CHECKER { extern std::unique_ptr<SatChecker> construct_checker(const Jani2Interface& jani_2_interface, const PLAJA::Configuration& config); }

#endif //PLAJA_SAT_CHECKER_H
