//
// This file is part of the PlaJA code base.
// Copyright (c) (2019 - 2024) Marcel Vinzent.
// See README.md in the top-level directory for licensing information.
//

#include <algorithm>
#include "predicate_dependency_graph.h"
#include "../../parser/visitor/extern/variables_of.h"

PredicateDependencyGraph::PredicateDependencyGraph(const ModelSmt& model):
    model(&model) {
    increment();
}

PredicateDependencyGraph::~PredicateDependencyGraph() = default;

void PredicateDependencyGraph::increment() {
    PLAJA_ASSERT(varsPerPred.size() <= model->get_number_predicates())

    const auto num_preds = model->get_number_predicates();
    varsPerPred.reserve(num_preds);

    for (PredicateIndex_type pred = varsPerPred.size(); pred < num_preds; ++pred) {

        const auto vars_of_pred_set = VARIABLES_OF::vars_of(model->get_predicate(pred), true);

        /* Per predicate mapping. */
        varsPerPred.emplace_back(vars_of_pred_set.cbegin(), vars_of_pred_set.cend());
        auto& vars_of_pred = varsPerPred.back();
        std::sort(vars_of_pred.begin(), vars_of_pred.end()); // Keep sorted, for convenience.

        /* Per variable mapping. */
        for (const VariableID_type var: vars_of_pred) {
            auto& preds_per_var = predsPerVar[var];
            PLAJA_ASSERT(not std::any_of(preds_per_var.cbegin(), preds_per_var.cend(), [pred](PredicateIndex_type other) { return pred == other; })) // Sanity: Each predicate is added once.
            preds_per_var.push_back(pred);
            std::sort(preds_per_var.begin(), preds_per_var.end()); // Keep sorted, for convenience.
        }

    }

}

/*********************************************************************************************************************/

void PredicateDependencyGraph::compute_dependent_preds_rec_var(VariableID_type var, std::unordered_set<PredicateIndex_type>& dependent_preds) const { // NOLINT(*-no-recursion)
    PLAJA_ASSERT(predsPerVar.count(var))
    for (const PredicateIndex_type pred: predsPerVar.at(var)) { compute_dependent_preds_rec_pred(pred, dependent_preds); }
}

void PredicateDependencyGraph::compute_dependent_preds_rec_pred(PredicateIndex_type pred, std::unordered_set<PredicateIndex_type>& dependent_preds) const { // NOLINT(*-no-recursion)
    if (not dependent_preds.insert(pred).second) { return; }
    for (const VariableID_type var: varsPerPred[pred]) { compute_dependent_preds_rec_var(var, dependent_preds); }
}

std::unordered_set<PredicateIndex_type> PredicateDependencyGraph::compute_dependent_predicates(PredicateIndex_type pred) const {
    std::unordered_set<PredicateIndex_type> dependent_preds;
    compute_dependent_preds_rec_pred(pred, dependent_preds);
    return dependent_preds;
}

/*********************************************************************************************************************/

void PredicateDependencyGraph::compute_var_closure_rec_var(VariableID_type var, std::unordered_set<VariableID_type>& closure) const { // NOLINT(*-no-recursion)
    PLAJA_ASSERT(predsPerVar.count(var))
    if (not closure.insert(var).second) { return; }
    for (const PredicateIndex_type pred: predsPerVar.at(var)) { compute_var_closure_rec_pred(pred, closure); }
}

void PredicateDependencyGraph::compute_var_closure_rec_pred(PredicateIndex_type pred, std::unordered_set<VariableID_type>& closure) const { // NOLINT(*-no-recursion)
    for (const VariableID_type var: varsPerPred[pred]) { compute_var_closure_rec_var(var, closure); }
}

std::unordered_set<VariableID_type> PredicateDependencyGraph::compute_variable_closure(PredicateIndex_type pred) const {
    std::unordered_set<VariableID_type> closure;
    compute_var_closure_rec_pred(pred, closure);
    return closure;
}