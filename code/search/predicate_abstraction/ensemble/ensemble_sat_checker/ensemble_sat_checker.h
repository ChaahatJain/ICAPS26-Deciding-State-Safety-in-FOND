//
// This file is part of the PlaJA code base.
// Copyright (c) (2023 - 2024) Chaahat Jain.
//

#ifndef PLAJA_ENSEMBLE_SAT_CHECKER_H
#define PLAJA_ENSEMBLE_SAT_CHECKER_H

#include <list>
#include <vector>
#include <memory>
#include "../../../../include/ct_config_const.h"
#include "../../../../stats/forward_stats.h"
#include "../../../../assertions.h"
#include "../../../factories/forward_factories.h"
#include "../../../information/jani2ensemble/forward_jani_2_ensemble.h"
#include "../../../information/jani2ensemble/using_jani2ensemble.h"
#include "../../../using_search.h"
#include "../../../smt/base/forward_solution_check.h"
#include "../../../smt_ensemble/forward_smt_veritas.h"
#include "../../../states/forward_states.h"
#include "../../pa_states/forward_pa_states.h"
#include "../smt/forward_smt_ensemble_pa.h"
#include "../../sat_checker.h"
#include "../../../smt_ensemble/solver/solver.h"

class AdversarialAttack;

/** Ensemble-Sat checking for PPA. */
class EnsembleSatChecker : public SatChecker {
    friend struct EnsembleSatCheckerFactory; //
    friend class EnsembleSatCheckerTest;

protected:
    // construction data
    std::shared_ptr<ModelVeritasPA> modelVeritas;
    const Jani2Ensemble* jani2Ensemble;

    // checking modes
    std::unique_ptr<VERITAS_IN_PLAJA::Solver> solver;
    std::vector<veritas::Interval> box = {};
#ifdef USE_ADVERSARIAL_ATTACK
    std::unique_ptr<AdversarialAttack> adversarialAttack;
    bool attackSolutionGuess;
#endif

    // cache
    const ActionOpVeritasPA* set_action_op;


#ifdef ENABLE_QUERY_CACHING
    std::unique_ptr<QueryToJson> queryToJson;
#endif

 void save_solution_if(bool rlt) override;

 virtual bool check_() override; // internal auxiliary function to reduce code duplication

public:
    EnsembleSatChecker(const Jani2Ensemble& jani_2_ensemble, const PLAJA::Configuration& config);
    virtual ~EnsembleSatChecker() override;
    DELETE_CONSTRUCTOR(EnsembleSatChecker)

    void set_op_applicability_cache(std::shared_ptr<OpApplicability>& op_app) override;

    [[nodiscard]] const Jani2Ensemble& _jani_2_ensemble() const { return *jani2Ensemble; }

    void increment() override;

    void push() override;
    void pop() override;

#ifdef ENABLE_TERMINAL_STATE_SUPPORT
    void add_non_terminal_condition() override;
#else
    inline void add_non_terminal_condition() { PLAJA_ABORT }
#endif

    void set_operator(ActionOpID_type action_op_id) override;

    /**
     * Add constraints res. predicates of the source state.
     * In particular also location values if in usage.
     * @param source
     */
    void add_source_state(const AbstractState& source) override;

    /**
     * @return true if the guard is not trivially true (and thus the query has been modified).
     */
    bool add_guard() override;

    /**
     * @return true if guard is not trivially true (and thus the query has *not* become trivially unsat).
     */
    bool add_guard_negation() override;

    void add_update() override;
    void add_target_state(const AbstractState& target) override;

    /**
     * @return false if the query has become unsat.
     */
    bool add_output_interface() override; // internally prior to each check

    /**
     * Check whether the current input query is sat (true) or unsat (false).
     * @param unknown, value to return in case input query could not be solved.
     * @return
     */
    bool check_relaxed() override;

    bool has_solution_box() override { return true; } 

    std::vector<veritas::Interval> release_box() const { return box; }
    
    std::vector<veritas::Interval> extract_box() const { return solver->extract_box(); }

#ifdef ENABLE_STALLING_DETECTION
    void stalling_detection(const State& source);
#endif

#ifdef USE_ADVERSARIAL_ATTACK
    bool adversarial_attack();
#endif

};

namespace ENSEMBLE_SAT_CHECKER { extern std::unique_ptr<EnsembleSatChecker> construct_checker(const Jani2Ensemble& jani_2_ensemble, const PLAJA::Configuration& config); }

#endif //PLAJA_ENSEMBLE_SAT_CHECKER_H
