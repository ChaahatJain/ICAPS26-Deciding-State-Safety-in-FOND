//
// This file is part of the PlaJA code base.
// Copyright (c) (2019 - 2020) Marcel Vinzent.
// See README.md in the top-level directory for licensing information.
//

#ifndef PLAJA_PREDICATE_RELATIONS_H
#define PLAJA_PREDICATE_RELATIONS_H

#include <list>
#include <memory>
#include <unordered_map>
#include <vector>
#include "../../../parser/ast/expression/forward_expression.h"
#include "../../../parser/ast/forward_ast.h"
#include "../../../stats/forward_stats.h"
#include "../../../assertions.h"
#include "../../factories/forward_factories.h"
#include "../using_predicate_abstraction.h"
#include "../pa_states/forward_pa_states.h"
#include "../smt/forward_smt_pa.h"

namespace PLAJA { class Configuration; }

class PredicateRelations final {
private:
    std::unique_ptr<ModelZ3PA> modelZ3Own;
    const ModelZ3PA* modelZ3;
    std::unique_ptr<Z3_IN_PLAJA::SMTSolver> solverOwn; // for backwards compatibility
    Z3_IN_PLAJA::SMTSolver* solver;

    /* Predicate-truth-value pairs entailed by the ground truth, i.e., the state of the solver fed into the machinery. */
    std::unordered_map<PredicateIndex_type, PA::EntailmentValueType> groundTruthEntailments;

    /* For each predicate-truth-value pair (A,true) the pairs it entails given ground truth: i.e.
     * the conjunction of A and B (value false) or A and \not B (value true) is unsat,
     * by predicate index
     * -- only computed for pairs for which the truth value of none is entailed by ground truth.
     */
    std::vector<std::list<std::pair<PredicateIndex_type, PA::EntailmentValueType>>> entailmentsPerPred;

    /* Sub-list with indexes lager than respective predicate index, see fix_predicate_state_asc_index. */
    std::vector<std::list<std::pair<PredicateIndex_type, PA::EntailmentValueType>>::const_iterator> entailmentsPerPredAscIndex;
    std::vector<std::list<std::pair<PredicateIndex_type, PA::EntailmentValueType>>> entailmentsPerNegPred;
    std::vector<std::list<std::pair<PredicateIndex_type, PA::EntailmentValueType>>::const_iterator> entailmentsPerNegPredAscIndex;

    /* For each predicate-truth-value pair the predicate-truth-values it is entailed by.*/
    std::vector<std::list<std::pair<PredicateIndex_type, PA::EntailmentValueType>>> entailmentsForPred;
    std::vector<std::list<std::pair<PredicateIndex_type, PA::EntailmentValueType>>> entailmentsForNegPred;

    FIELD_IF_DEBUG(bool emptyGroundTruth;) // has trivial ground?

    void compute_singleton_entailments(PredicateIndex_type index);
    void compute_binary_entailments(PredicateIndex_type index1, PredicateIndex_type index2);
    void compute_entailments_per_asc_index();

public:
    explicit PredicateRelations(const PLAJA::Configuration& config, const ModelZ3PA& model_z3, const Expression* ground_truth = nullptr);
    explicit PredicateRelations(const ModelZ3PA& model_z3, Z3_IN_PLAJA::SMTSolver& solver); // solver must contain "ground truth", typically src bounds of Z3 model.
    ~PredicateRelations();
    DELETE_CONSTRUCTOR(PredicateRelations)

    static std::unique_ptr<PredicateRelations> construct(const PLAJA::Configuration& config, const Expression* ground_truth);

    [[nodiscard]] inline PredicatesNumber_type get_size_predicates() const { return entailmentsPerPred.size(); }

    void increment(); // for incremental usage (within PA); external solver provided at construction must contain "ground truth"

    [[nodiscard]] inline const std::list<std::pair<PredicateIndex_type, PA::EntailmentValueType>>& entailments_for(PredicateIndex_type predicate, PA::TruthValueType value) const {
        PLAJA_ASSERT(not PA::is_unknown(value))
        PLAJA_ASSERT(predicate < entailmentsPerPred.size())
        return value ? entailmentsPerPred[predicate] : entailmentsPerNegPred[predicate];
    }

    /**
     * Is predicate entailed by ground truth ?
     * @param predicate_index
     * @return
     */
    [[nodiscard]] inline bool is_entailed(PredicateIndex_type predicate_index) const { return groundTruthEntailments.count(predicate_index); }

    [[nodiscard]] inline bool is_any_entailed() const { return !groundTruthEntailments.empty(); }

    /** Is the truth value of the predicate entailed by the state. */
    [[nodiscard]] bool is_entailed(PredicateIndex_type predicate_index, PA::TruthValueType truth_value, const PaStateBase& pa_state) const;

    [[nodiscard]] inline bool is_entailed(PredicateIndex_type predicate_index, const PaStateBase& pa_state) const { return is_entailed(predicate_index) or is_entailed(predicate_index, true, pa_state) or is_entailed(predicate_index, false, pa_state); }

    [[nodiscard]] bool is_equivalent_to(PredicateIndex_type pred_index1, PredicateIndex_type pred_index2) const;
    [[nodiscard]] bool is_equivalent_to_other(PredicateIndex_type pred_index) const;
    [[nodiscard]] bool is_equivalent_to_other_asc(PredicateIndex_type pred_index) const;
    [[nodiscard]] bool check_equivalence() const;

    /** Fix ground truth entailments. */
    void fix_predicate_state(PredicateState& predicate_state) const;

    /**
     * Fix predicate state with respect to set predicate using entailments of predicate relations.
     * @param pred_index with respect to which to fix.
     */
    void fix_predicate_state(PredicateState& predicate_state, PredicateIndex_type predicate_index) const;

    /** Fix entailments with respect to defined predicates. */
    void fix_predicate_state_all(PredicateState& predicate_state) const;

    /**
     * Works as fix_predicate_state only fixing predicates of a larger index.
     * Idea: predicates are fixed/set in ASC index.
     * @param predicate_state
     * @param predicate_index
     */
    void fix_predicate_state_asc_index(PredicateState& predicate_state, PredicateIndex_type predicate_index) const;

    inline void fix_predicate_state_asc_if_non_lazy(PredicateState& predicate_state, PredicateIndex_type predicate_index) const {
        if constexpr (PLAJA_GLOBAL::lazyPA) { fix_predicate_state(predicate_state, predicate_index); }
        else { fix_predicate_state_asc_index(predicate_state, predicate_index); }
    }

private:
    FCT_IF_DEBUG(void check_recursive(PredicateState& predicate_state, PredicateIndex_type original_index, const std::list<PredicateIndex_type>& newly_set, bool asc_index) const;)
public:
    FCT_IF_DEBUG(void check_all_entailments(const PredicateState& predicate_state) const;) // check whether all entailed predicates are marked entailed

    /**
     * Check if predicate is useful wrt. existing predicate.
     * @param pa_state if provided then check usefulness wrt. provided predicate-truth-values.
     */
    [[nodiscard]] bool check_if_useful(const Expression& predicate, const PaStateBase* pa_state) const;
    [[nodiscard]] bool check_if_useful(const Expression& predicate, const PaStateBase& pa_state) const;
    [[nodiscard]] bool check_if_useful(const Expression& predicate, const std::list<PredicateIndex_type>& preds_already_used) const;

    [[nodiscard]] bool check_equivalence(const Expression& pred1, const Expression& pred2) const;

    FCT_IF_DEBUG([[nodiscard]] inline bool has_empty_ground_truth() const { return emptyGroundTruth; })

};

#endif //PLAJA_PREDICATE_RELATIONS_H
