//
// This file is part of the PlaJA code base.
// Copyright (c) (2019 - 2022) Marcel Vinzent.
// See README.md in the top-level directory for licensing information.
//

#ifndef PLAJA_PREDICATES_STRUCTURE_H
#define PLAJA_PREDICATES_STRUCTURE_H

#include <list>
#include <memory>
#include "../../../parser/ast/expression/non_standard/expression_set.h"
#include "../../../parser/ast/expression/non_standard/predicates_expression.h"
#include "../../../stats/forward_stats.h"
#include "../../factories/forward_factories.h"
#include "../../using_search.h"
#include "../match_tree/forward_match_tree_pa.h"
#include "../optimization/forward_optimization_pa.h"
#include "../pa_states/forward_pa_states.h"
#include "../smt/forward_smt_pa.h"
#include "../using_predicate_abstraction.h"

struct PredicatesStructure {

private:
    std::shared_ptr<ModelZ3PA> modelZ3;
    PLAJA::StatsBase* stats;
    PredicatesExpression* predicates;
    ExpressionSet predicateSet;
    std::shared_ptr<PredicateRelations> predicateRelations;
    //
    std::shared_ptr<PAMatchTree> mt;

    // config:
    FIELD_IF_LAZY_PA(bool lazyPaState;)
    FIELD_IF_LAZY_PA(bool lazyPaApp;)
    FIELD_IF_LAZY_PA(bool lazyPaSel;)
    FIELD_IF_LAZY_PA(bool lazyPaTarget;)
    bool computeWp;
    bool addGuards; // whether to compute WP of *all* guards on the path
    bool addPredicates; // whether to compute WP of each predicate value (respectively its negation) on the path
    bool excludeEntailments; // exclude the guards and predicates above if entailed on the path (in this case: the guards/predicates are to be checked and added externally)
    bool reSub; // re-substitute wp-predicates, i.e., a predicate already substituted during wp computation is also substituted in the next step

    bool add_predicate_splits_aux(std::unique_ptr<Expression>&& predicate);

    [[nodiscard]] std::list<std::unique_ptr<Expression>> prepare_predicate(std::unique_ptr<Expression>&& predicate, const ExpressionSet& predicates_set, const PaStateBase* pa_state) const;

    PredicateIndex_type add_prepared_predicate(std::unique_ptr<Expression>&& predicate);

    // cache for propose split:
    mutable const StateBase* splitLb; // i.e, the interval in which a chosen split point must be contained, not the binary search interval. The latter is globally standardized on purpose.
    mutable const StateBase* splitUb;
    mutable PLAJA::integer interestLbInt;
    mutable PLAJA::integer interestUbInt;
    mutable PLAJA::floating interestLbFloat;
    mutable PLAJA::floating interestUbFloat;

    [[nodiscard]] bool propose_split(class BinaryOpExpression& bound, PLAJA::integer lower_bound, PLAJA::integer upper_bound, const ExpressionSet& predicates_set, const PaStateBase* pa_state) const;
    [[nodiscard]] bool propose_split(class BinaryOpExpression& bound, PLAJA::floating lower_bound, PLAJA::floating upper_bound, const ExpressionSet& predicates_set, const PaStateBase* pa_state) const;

public:
    PredicatesStructure(const PLAJA::Configuration& config, PredicatesExpression& predicates_);
    ~PredicatesStructure();
    DELETE_CONSTRUCTOR(PredicatesStructure)

    [[nodiscard]] inline const ModelZ3PA& get_model_z3() const {
        PLAJA_ASSERT(modelZ3.get())
        return *modelZ3;
    }

    [[nodiscard]] const ModelZ3PA& prepare_model_z3(StepIndex_type max_step) const;

    [[nodiscard]] inline PLAJA::StatsBase* share_stats() const { return stats; }

    FCT_IF_LAZY_PA([[nodiscard]] inline bool do_lazy_pa_state() const { return lazyPaState; })

    FCT_IF_LAZY_PA([[nodiscard]] inline bool do_lazy_pa_app() const { return lazyPaApp; })

    FCT_IF_LAZY_PA([[nodiscard]] inline bool do_lazy_pa_sel() const { return lazyPaSel; })

    FCT_IF_LAZY_PA([[nodiscard]] inline bool do_lazy_pa_target() const { return lazyPaTarget; })

    [[nodiscard]] inline bool do_compute_wp() const { return computeWp; }

    [[nodiscard]] inline bool do_add_guards() const { return addGuards; }

    [[nodiscard]] inline bool do_add_predicates() const { return addPredicates; }

    [[nodiscard]] inline bool do_exclude_entailments() const { return excludeEntailments; }

    [[nodiscard]] inline bool do_re_sub() const { return reSub; }

    [[nodiscard]] inline std::size_t size() const { return predicates->get_number_predicates(); };

    [[nodiscard]] inline const Expression* operator[](std::size_t index) const {
        PLAJA_ASSERT(index < size())
        return predicates->get_predicate(index);
    }

    [[nodiscard]] inline AstConstIterator<Expression> init_predicate_it() const { return static_cast<const PredicatesExpression*>(predicates)->predicatesIterator(); }

    [[nodiscard]] inline AstIterator<Expression> init_predicate_it() { return predicates->predicatesIterator(); }

    inline void reserve(std::size_t predicates_cap) { predicates->reserve(predicates_cap); }

    bool add_predicate(std::unique_ptr<Expression>&& predicate);

    std::list<const Expression*> add_predicate_and_retrieve(std::unique_ptr<Expression>&& predicate);

    [[nodiscard]] std::list<std::unique_ptr<Expression>> prepare_predicate(std::unique_ptr<Expression>&& predicate, const ExpressionSet* predicates_set, const PaStateBase* pa_state) const;

    void add_prepared_predicates(std::list<std::unique_ptr<Expression>>&& prepared_predicates, const PaStateBase* pa_state);

    [[nodiscard]] ExpressionSet collect_global_predicates() const;

    [[nodiscard]] ExpressionSet collect_predicates(const PaStateBase& pa_state) const;

    /** To prepare predicates externally. */
    [[nodiscard]] bool check_equivalence(const Expression& pred1, const Expression& pred2) const;

    [[nodiscard]] inline bool contains(const Expression& predicate) const { return predicateSet.count(&predicate); }

    STMT_IF_DEBUG([[nodiscard]] inline bool contains(const Expression* predicate) const { return contains(*predicate); })

    void set_interval_for_propose_split(const StateBase* lower_bounds = nullptr, const StateBase* upper_bounds = nullptr) const { // NOLINT(*-easily-swappable-parameters)
        splitLb = lower_bounds;
        splitUb = upper_bounds;
    }

    [[nodiscard]] std::unique_ptr<Expression> propose_split(const class LValueExpression& var, const ExpressionSet* predicates_set, const PaStateBase* pa_state) const;

    FCT_IF_DEBUG([[nodiscard]] bool match(const PaStateBase& pa_state, const ExpressionSet& predicates_set) const;)

    FCT_IF_DEBUG(void dump() const;)

};

#endif //PLAJA_PREDICATES_STRUCTURE_H
