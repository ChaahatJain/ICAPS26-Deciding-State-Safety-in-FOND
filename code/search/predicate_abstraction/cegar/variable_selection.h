//
// This file is part of the PlaJA code base.
// Copyright (c) (2019 - 2023) Marcel Vinzent.
// See README.md in the top-level directory for licensing information.
//

#ifndef PLAJA_VARIABLE_SELECTION_H
#define PLAJA_VARIABLE_SELECTION_H

#include <list>
#include <memory>
#include <unordered_set>
#include "../../../parser/ast/expression/forward_expression.h"
#include "../../../stats/forward_stats.h"
#include "../../../utils/default_constructors.h"
#include "../../factories/forward_factories.h"
#include "../../smt_nn/forward_smt_nn.h"
#include "../../states/forward_states.h"
#include "../../using_search.h"
#include "../nn/smt/forward_smt_nn_pa.h"
#include "../pa_states/forward_pa_states.h"

namespace VARIABLE_SELECTION {
    struct PerturbationBoundMap;
    struct VarInfoVec;
    enum class PerturbationBoundType { Lower, Upper, Both };
}

class VariableSelection {
    friend class VariableSelectionTest;

private:
    std::shared_ptr<ModelMarabouPA> model;
    std::unique_ptr<MARABOU_IN_PLAJA::SMTSolver> solver;

    /* Cache. */
    std::unique_ptr<VARIABLE_SELECTION::VarInfoVec> varInfo;
    std::unique_ptr<VARIABLE_SELECTION::PerturbationBoundMap> lastPertBound;

    /* Config. */
    PLAJA::floating perturbationBoundUser; // relative on considered perturbation interval
    PLAJA::floating perturbationBoundPrecision; // for binary search
    bool ascTravOrd;
    bool perturbWitness;
    bool perturbTowardsRef;
    bool perturb1Norm;
    // Specific to decision boundary search (not variable selection).
    bool globalPerturbation;
    bool individualPerturbation;
    bool perDirectionPerturbation;

    /* Stats. */
    PLAJA::StatsBase* stats;
    double pertBoundSum;
    unsigned int pertBoundCounter;
    unsigned int relevantVars;
    unsigned int irrelevantVars;

    std::list<const LValueExpression*> compute_traversing_order();

    /** inf-Norm. */
    void add_perturbation_constraint(VariableIndex_type state_index, PLAJA::floating perturbation_bound);
    void add_perturbation_constraint_0_norm(PLAJA::floating perturbation_bound, const std::unordered_set<VariableIndex_type>& perturbed_vars);

    /** 1-Norm. */
    void add_perturbation_constraint_1_norm(PLAJA::floating perturbation_bound, const std::unordered_set<VariableIndex_type>& perturbed_vars);

    void constraint_non_irrelevant_vars(const StateBase& state, const std::unordered_set<VariableIndex_type>& irrelevant_vars);
    void compute_relevant_vars(const StateBase& state, ActionLabel_type action_label, PLAJA::floating perturbation_bound, const std::list<const LValueExpression*>& traversing_order, std::list<const LValueExpression*>& relevant_features, std::unordered_set<VariableIndex_type>& irrelevant_features);

    bool check_decision_boundary(ActionLabel_type action_label, const VARIABLE_SELECTION::PerturbationBoundMap& relative_perturbation);
    PLAJA::floating compute_decision_boundary_greedy(ActionLabel_type action_label);
    /** @return true if there exists a split point (i.e., not any value is possible). */
    bool compute_decision_boundary_greedy(VariableIndex_type state_index, ActionLabel_type action_label, VARIABLE_SELECTION::PerturbationBoundMap& perturbation_bound, VARIABLE_SELECTION::PerturbationBoundType type);

public:
    explicit VariableSelection(const PLAJA::Configuration& config);
    ~VariableSelection();
    DELETE_CONSTRUCTOR(VariableSelection)

    /* Flags for sanity checks. */

    [[nodiscard]] inline bool is_asc_trav_ord() const { return ascTravOrd; }

    [[nodiscard]] inline bool is_perturb_witness() const { return perturbWitness; }

    [[nodiscard]] inline bool is_perturb_towards_ref() const { return perturbTowardsRef; }

    [[nodiscard]] inline bool is_perturb_1_norm() const { return perturb1Norm; }

    /* Interface. */

    const LValueExpression* compute_most_sensitive_variable(const StateBase& witness, const StateBase& non_witness);

    std::list<const LValueExpression*> compute_relevant_vars(const StateBase& witness, const StateBase& non_witness, ActionLabel_type action_label);

    /** @return variables with an actual split point (i.e., not any value is possible). */
    std::list<const LValueExpression*> compute_decision_boundary(const StateBase& witness, const StateBase& non_witness, ActionLabel_type action_label);
    std::list<const LValueExpression*> compute_decision_boundary(const StateBase& witness, const PaStateBase* pa_state, ActionLabel_type action_label);

    [[nodiscard]] std::unique_ptr<Expression> get_last_lower_perturbation_bound(VariableIndex_type state_index) const;
    [[nodiscard]] std::unique_ptr<Expression> get_last_upper_perturbation_bound(VariableIndex_type state_index) const;

    // std::list<VariableIndex_type> compute_split_point(VariableIndex_type state_index, const StateBase* concretization, const StateBase* witness, ActionLabel_type action);
};

/**********************************************************************************************************************/

namespace PLAJA_OPTION::VARIABLE_SELECTION {
    extern void add_options(PLAJA::OptionParser& option_parser);
    extern void print_options();
}

#endif //PLAJA_VARIABLE_SELECTION_H
