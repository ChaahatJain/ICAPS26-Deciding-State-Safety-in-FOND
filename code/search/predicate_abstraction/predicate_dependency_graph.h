//
// This file is part of the PlaJA code base.
// Copyright (c) (2019 - 2024) Marcel Vinzent.
// See README.md in the top-level directory for licensing information.
//

#ifndef PLAJA_PREDICATE_DEPENDENCY_GRAPH_H
#define PLAJA_PREDICATE_DEPENDENCY_GRAPH_H

#include <vector>
#include <unordered_map>
#include <unordered_set>
#include "../../utils/default_constructors.h"
#include "using_predicate_abstraction.h"
#include "../smt/base/model_smt.h"

class PredicateDependencyGraph final {
    friend class PredicateDependencyGraphTest;

private:
    const ModelSmt* model;

    std::unordered_map<VariableID_type, std::vector<PredicateIndex_type>> predsPerVar;
    std::vector<std::vector<VariableID_type>> varsPerPred;

    void compute_dependent_preds_rec_var(VariableID_type var, std::unordered_set<PredicateIndex_type>& dependent_preds) const;
    void compute_dependent_preds_rec_pred(PredicateIndex_type pred, std::unordered_set<PredicateIndex_type>& dependent_preds) const;

    void compute_var_closure_rec_var(VariableID_type var, std::unordered_set<VariableID_type>& closure) const;
    void compute_var_closure_rec_pred(PredicateIndex_type pred, std::unordered_set<VariableID_type>& closure) const;

public:
    explicit PredicateDependencyGraph(const ModelSmt& model);
    ~PredicateDependencyGraph();
    DELETE_CONSTRUCTOR(PredicateDependencyGraph)

    void increment();

    /* Interface. */

    /**
     * @param rlt may be non-empty if closed under its member
     */
    inline void compute_dependent_predicates(std::unordered_set<PredicateIndex_type>& rlt, PredicateIndex_type pred) const { compute_dependent_preds_rec_pred(pred, rlt); }

    [[nodiscard]] std::unordered_set<PredicateIndex_type> compute_dependent_predicates(PredicateIndex_type pred) const;

    /**
     * @param rlt may be non-empty if closed under its member
     */
    inline void compute_variable_closure(std::unordered_set<VariableID_type>& rlt, PredicateIndex_type pred) const { compute_var_closure_rec_pred(pred, rlt); }

    [[nodiscard]] std::unordered_set<VariableID_type> compute_variable_closure(PredicateIndex_type pred) const;

};

#endif //PLAJA_PREDICATE_DEPENDENCY_GRAPH_H
