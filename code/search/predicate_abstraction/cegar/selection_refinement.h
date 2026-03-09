//
// This file is part of the PlaJA code base.
// Copyright (c) (2019 - 2022) Marcel Vinzent.
// See README.md in the top-level directory for licensing information.
//

#ifndef PLAJA_SELECTION_REFINEMENT_H
#define PLAJA_SELECTION_REFINEMENT_H

#include <list>
#include <memory>
#include "../../../parser/ast/expression/forward_expression.h"
#include "../../../stats/forward_stats.h"
#include "../../../utils/default_constructors.h"
#include "../../factories/configuration.h"
#include "../../information/jani_2_interface.h"
#include "../../non_prob_search/policy/forward_policy.h"
#include "../../using_search.h"
#include "../../states/forward_states.h"
#include "../pa_states/forward_pa_states.h"
#include "forward_cegar.h"
#include "../../../include/ct_config_const.h"
#ifdef USE_VERITAS
#include "interval.hpp"
#endif

namespace SELECTION_REFINEMENT { struct SplitVarsSelection; }

class SelectionRefinement {
    friend SELECTION_REFINEMENT::SplitVarsSelection;

public:
    enum Mode {
        ConcretizationExclusion,
        WsAll, // All inputs for which witness and concretization state disagree.
        WsMaxAbs, // Max absolute distance.
        WsMaxRel, // Max relative distance.
        WsMinAbs, // Max absolute distance.
        WsMinRel, // Max relative distance.
        WsVarSel, // Use variable selection.
        WsSensitiveOne, // Use most sensitive var only.
        WsSmtSearchC, // Smt-based binary search between witness and concretization to find selection switch point (involving all variables). Reuses functionality of variable selection.
        WsSmtSearchPa, // Smt-based binary search around witness (within pa state) to find decision boundary (involving all variables). Reuses functionality of variable selection.
        WsSmtSearch, // Smt-based binary search around witness (within state variable domain) to find decision boundary (involving all variables). Reuses functionality of variable selection.
        WsRandomOne, // Randomly choose one candidate.
        WsRandom, // Choose candidates randomly (at least one).
        All, // All (nn inputs) ... though only supported for BS & GlobalSplitPaState.
        WsInterval, // All inputs for which concretization is not inside witness interval
        IntervalExclusion, // Add predicates to exclude entire interval
    };

    enum SplitPoint {
        NearConcretization,
        NearWitness,
        Middle,
        Random,
        WsSearch, // Binary search between concretization and witness value to find selection switch point on modified witness (fixing all other vars).
        VarSelEps, // Relative split point on distance witness to concretization as perturbation bound computed by variable selection.
        GlobalSplitWs, // Perform binary search on variable domain, but split point must be contained in interval between witness and concretization.
        GlobalSplitPaState, // Perform binary search on variable domain, but split point must be contained in abstract state.
        BinarySplit, // Perform binary search on variable domain for the next split not in the predicate set yet (i.e., witness-agnostic baseline).
    };

    enum EntailmentMode {
        None,
        UseEntailment,
        EntailmentOnly,
    };

    // TODO utilize steepest descent to find decision boundary

private:
    const Jani2Interface* policyInterface;
    const Policy* policy;

    Mode mode;
    SplitPoint splitPoint;
    EntailmentMode entailmentMode; // how to treat entailed state-variable-values
    std::unique_ptr<VariableSelection> varSelection; //
    class RandomNumberGenerator* rng;

    /* Stats */
    PLAJA::StatsBase* stats;
    mutable double accumulatedDistanceWsStates;
    mutable unsigned wsStatsCounter;

    void update_ws_stats(const StateBase& witness, const StateBase& concretization) const;

    /* Auxiliary: */

    [[nodiscard]] inline bool has_random_mode() { return mode == Mode::WsRandomOne or mode == Mode::WsRandom; }

    [[nodiscard]] inline bool has_random_split_point() { return splitPoint == SplitPoint::Random; }

    /**/
    #ifdef USE_VERITAS
    [[nodiscard]] std::list<const LValueExpression*> witness_split_candidates(const std::vector<veritas::Interval> box_witness, const StateBase& concretization) const;
    void add_refinement_preds(SpuriousnessResult& result, const std::list<const LValueExpression*>& split_vars, ActionLabel_type action, const std::vector<veritas::Interval> box_witness, const StateBase& concretization) const;
    [[nodiscard]] std::unique_ptr<Expression> create_split(const LValueExpression& var, ActionLabel_type action, const std::vector<veritas::Interval> box_witness, const StateBase& concretization) const;
    [[nodiscard]] std::unique_ptr<Expression> create_lower_split(const LValueExpression& var, ActionLabel_type action, const std::vector<veritas::Interval> box_witness) const;
    [[nodiscard]] std::unique_ptr<Expression> create_upper_split(const LValueExpression& var, ActionLabel_type action, const std::vector<veritas::Interval> box_witness) const;
    void add_entailed_refinement_preds(SpuriousnessResult& result, const LValueExpression& var, const std::vector<veritas::Interval> box_witness) const;
    void add_entailed_refinement_preds(SpuriousnessResult& result, const LValueExpression& var, const StateBase& concretization, const std::vector<veritas::Interval> witness) const;
    #endif



    [[nodiscard]] std::list<const LValueExpression*> witness_split_candidates(const StateBase& witness, const StateBase& concretization) const;
    
    void add_refinement_preds(SpuriousnessResult& result, const std::list<const LValueExpression*>& split_vars, ActionLabel_type action, const StateBase& witness, const StateBase& concretization) const;
    
    [[nodiscard]] std::unique_ptr<Expression> create_split(const LValueExpression& var, ActionLabel_type action, const StateBase& witness, const StateBase& concretization) const;


    void add_entailed_refinement_preds(SpuriousnessResult& result, const LValueExpression& var, const StateBase& concretization) const;
    void add_entailed_refinement_preds(SpuriousnessResult& result, const LValueExpression& var, const StateBase& concretization, const StateBase& witness) const;

    /** Return true if refinement is completed. */
    std::pair<bool, std::unordered_set<VariableIndex_type>> add_entailed_refinement_preds(SpuriousnessResult& result, const StateBase& concretization) const;

    void witness_split_random_one(SpuriousnessResult& result, ActionLabel_type action, const StateBase& witness, const StateBase& concretization) const;
    void witness_split_random(SpuriousnessResult& result, ActionLabel_type action, const StateBase& witness, const StateBase& concretization) const;

    void perform_non_ws_refinement(SpuriousnessResult& result, const StateBase& concretization) const;

public:
    explicit SelectionRefinement(const PLAJA::Configuration& config, const Jani2Interface& policy_interface);
    ~SelectionRefinement();
    DELETE_CONSTRUCTOR(SelectionRefinement)

    [[nodiscard]] bool needs_witness() const;
    [[nodiscard]] bool needs_concretization() const;
    #ifdef USE_VERITAS
    [[nodiscard]] bool needs_interval_witness() const { return mode == SelectionRefinement::WsInterval or mode == SelectionRefinement::IntervalExclusion; }
    void add_refinement_preds(class SpuriousnessResult& result, ActionLabel_type action, std::vector<veritas::Interval> box_witness, const StateBase* concretization) const;
    #endif
    void add_refinement_preds(SpuriousnessResult& result, ActionLabel_type action, const StateBase* witness, const StateBase* concretization) const;

    [[nodiscard]] inline Mode get_mode() const { return mode; }

    [[nodiscard]] inline const Jani2Interface& get_interface() const { return *policyInterface; }

};

/**********************************************************************************************************************/

namespace PLAJA { class OptionParser; }

namespace PLAJA_OPTION::SELECTION_REFINEMENT {
    extern void add_options(PLAJA::OptionParser& option_parser);
    extern void print_options();
}

#endif //PLAJA_SELECTION_REFINEMENT_H
