//
// This file is part of the PlaJA code base.
// Copyright (c) (2019 - 2022) Marcel Vinzent.
// See README.md in the top-level directory for licensing information.
//

#include "predicates_structure.h"
#include "../../../parser/ast/expression/constant_value_expression.h"
#include "../../../parser/ast/expression/binary_op_expression.h"
#include "../../../parser/ast/expression/expression_utils.h"
#include "../../../parser/ast/expression/variable_expression.h"
#include "../../../parser/visitor/extern/to_normalform.h"
#include "../../../utils/floating_utils.h"
#include "../../factories/predicate_abstraction/pa_options.h"
#include "../../factories/configuration.h"
#include "../../information/model_information.h"
#include "../match_tree/pa_match_tree.h"
#include "../optimization/predicate_relations.h"
#include "../smt/model_z3_pa.h"
#include "../using_predicate_abstraction.h"

PredicatesStructure::PredicatesStructure(const PLAJA::Configuration& config, PredicatesExpression& predicates_):
    stats(config.get_sharable_ptr<PLAJA::StatsBase>(PLAJA::SharableKey::STATS))
    , predicates(&predicates_)
    , predicateRelations(nullptr)
    CONSTRUCT_IF_LAZY_PA(lazyPaState(config.get_bool_option(PLAJA_OPTION::lazy_pa_state)))
    CONSTRUCT_IF_LAZY_PA(lazyPaApp(config.get_bool_option(PLAJA_OPTION::lazy_pa_app)))
    CONSTRUCT_IF_LAZY_PA(lazyPaSel(config.get_bool_option(PLAJA_OPTION::lazy_pa_sel)))
    CONSTRUCT_IF_LAZY_PA(lazyPaTarget(config.get_bool_option(PLAJA_OPTION::lazy_pa_target)))
    , computeWp(config.get_bool_option(PLAJA_OPTION::compute_wp))
    , addGuards(config.is_flag_set(PLAJA_OPTION::add_guards))
    , addPredicates(config.is_flag_set(PLAJA_OPTION::add_predicates))
    , excludeEntailments(config.get_bool_option(PLAJA_OPTION::exclude_entailments))
    , reSub(config.is_flag_set(PLAJA_OPTION::re_sub))
    , splitLb(nullptr)
    , splitUb(nullptr)
    , interestLbInt(0)
    , interestUbInt(0)
    , interestLbFloat(0)
    , interestUbFloat(0) {

    PLAJA_ASSERT(config.has_sharable(PLAJA::SharableKey::MODEL_Z3))
    modelZ3 = std::static_pointer_cast<ModelZ3PA>(config.get_sharable<ModelZ3>(PLAJA::SharableKey::MODEL_Z3));
    PLAJA_ASSERT(predicates == modelZ3->_predicates())

    if (config.has_sharable(PLAJA::SharableKey::PREDICATE_RELATIONS)) {
        predicateRelations = config.get_sharable<PredicateRelations>(PLAJA::SharableKey::PREDICATE_RELATIONS);
    } else {
        predicateRelations = config.set_sharable(PLAJA::SharableKey::PREDICATE_RELATIONS, std::make_shared<PredicateRelations>(config, *modelZ3, nullptr));
    }

    PLAJA_ASSERT(predicateRelations->has_empty_ground_truth())

    if constexpr (PLAJA_GLOBAL::lazyPA) { mt = config.has_sharable(PLAJA::SharableKey::PA_MT) ? config.get_sharable<PAMatchTree>(PLAJA::SharableKey::PA_MT) : config.set_sharable<PAMatchTree>(PLAJA::SharableKey::PA_MT, std::make_shared<PAMatchTree>(config, *predicates)); }

}

PredicatesStructure::~PredicatesStructure() = default;

/**********************************************************************************************************************/

const ModelZ3PA& PredicatesStructure::prepare_model_z3(StepIndex_type max_step) const {
    PLAJA_ASSERT(modelZ3)
    modelZ3->generate_steps(max_step);
    return *modelZ3;
}

/**********************************************************************************************************************/

inline bool PredicatesStructure::add_predicate_splits_aux(std::unique_ptr<Expression>&& predicate) {
    auto insert_rtl = predicateSet.insert(predicate.get());
    if (insert_rtl.second) {
        if (predicateRelations->check_if_useful(*predicate, nullptr)) {
            predicates->add_predicate(std::move(predicate));
            // also increment/update infrastructure to check usefulness:
            modelZ3->increment();
            predicateRelations->increment();
            //
            if constexpr (PLAJA_GLOBAL::lazyPA) { mt->add_global_predicate(predicates->get_number_predicates() - 1); }
            //
            return true;
        } else { predicateSet.erase(insert_rtl.first); } // remove again
    }
    return false;
}

bool PredicatesStructure::add_predicate(std::unique_ptr<Expression>&& predicate) {
    if (not predicate or predicate->is_constant()) { return false; } // optimization
    auto p_size_old = size();
    auto split_predicates = TO_NORMALFORM::normalize_and_split(std::move(predicate), true); // to preserve linearity
    for (auto& split_pred: split_predicates) {
        if (split_pred->is_constant()) { continue; }
        add_predicate_splits_aux(std::move(split_pred));
    }
    return size() > p_size_old;
}

std::list<const Expression*> PredicatesStructure::add_predicate_and_retrieve(std::unique_ptr<Expression>&& predicate) {
    auto pred_index = size();
    // add
    add_predicate(std::move(predicate));
    // retrieve
    std::list<const Expression*> added_splits;
    auto num_preds = size();
    for (; pred_index < num_preds; ++pred_index) { added_splits.push_back(predicates->get_predicate(pred_index)); }
    return added_splits;
}

/**********************************************************************************************************************/

std::list<std::unique_ptr<Expression>> PredicatesStructure::prepare_predicate(std::unique_ptr<Expression>&& predicate, const ExpressionSet& predicates_set, const PaStateBase* pa_state) const {
    PLAJA_ASSERT(predicate and not predicate->is_constant())

    PLAJA_ASSERT(not pa_state or match(*pa_state, predicates_set))

    auto split_predicates = TO_NORMALFORM::normalize_and_split(std::move(predicate), true); // to preserve linearity and *standardize*
    for (auto it = split_predicates.begin(); it != split_predicates.end();) {
        PLAJA_ASSERT(*it)
        const auto& pred = **it;

        if (pred.is_constant() or predicates_set.count(&pred)) {
            it = split_predicates.erase(it);
            continue;
        }

        if (PLAJA_GLOBAL::lazyPA and not pa_state ? not predicateRelations->check_if_useful(pred, mt->get_global_predicates()) : not predicateRelations->check_if_useful(pred, pa_state)) {
            it = split_predicates.erase(it);
            continue;
        }

        ++it;
    }

    return split_predicates;
}

std::list<std::unique_ptr<Expression>> PredicatesStructure::prepare_predicate(std::unique_ptr<Expression>&& predicate, const ExpressionSet* predicates_set, const PaStateBase* pa_state) const {
    PLAJA_ASSERT(PLAJA_GLOBAL::lazyPA or not pa_state) // NOLINT(cert-dcl03-c,misc-static-assert)
    PLAJA_ASSERT(PLAJA_GLOBAL::lazyPA == static_cast<bool>(predicates_set))
    PLAJA_ASSERT(predicates_set or not pa_state)

    if (not predicate or predicate->is_constant()) { return {}; } // optimization

    return prepare_predicate(std::move(predicate), predicates_set ? *predicates_set : predicateSet, pa_state);
}

PredicateIndex_type PredicatesStructure::add_prepared_predicate(std::unique_ptr<Expression>&& predicate) {
    PLAJA_ASSERT(predicate and not predicate->is_constant())
    PLAJA_ASSERT(TO_NORMALFORM::normalize_and_split(*predicate, true).size() == 1) // important: we use the expression (*not* the unique_ptr) reference in the assertion
    PLAJA_ASSERT(PLAJA_GLOBAL::lazyPA or not predicateSet.count(predicate.get()))

    if (predicateSet.insert(predicate.get()).second) {

        predicates->add_predicate(std::move(predicate));
        // also increment/update infrastructure to check usefulness:
        modelZ3->increment();
        predicateRelations->increment();

        return predicates->get_number_predicates() - 1;

    } else {

        const auto num_preds = predicates->get_number_predicates();

        for (PredicateIndex_type pred_index = 0; pred_index < num_preds; ++pred_index) {

            if (*predicate == *operator[](pred_index)) { return pred_index; }

        }

        PLAJA_ABORT

    }

}

void PredicatesStructure::add_prepared_predicates(std::list<std::unique_ptr<Expression>>&& prepared_predicates, const PaStateBase* pa_state) {
    PLAJA_ASSERT(PLAJA_GLOBAL::lazyPA or not pa_state) // NOLINT(cert-dcl03-c,misc-static-assert)

    std::vector<PredicateIndex_type> closure;
    if constexpr (PLAJA_GLOBAL::lazyPA) { closure.reserve(prepared_predicates.size()); }

    for (auto& predicate: prepared_predicates) {

        PLAJA_ASSERT(predicate and not predicate->is_constant()) // if (not predicate or predicate->is_constant()) { return; }

        PLAJA_ASSERT(not PLAJA_GLOBAL::lazyPA or pa_state or predicateRelations->check_if_useful(*predicate, mt->get_global_predicates()))
        PLAJA_ASSERT((PLAJA_GLOBAL::lazyPA and not pa_state) or predicateRelations->check_if_useful(*predicate, pa_state))

        if constexpr (PLAJA_GLOBAL::lazyPA) { closure.push_back(add_prepared_predicate(std::move(predicate))); }
        else { add_prepared_predicate(std::move(predicate)); }

    }

    if constexpr (PLAJA_GLOBAL::lazyPA) {

        PLAJA_ASSERT(closure.size() == prepared_predicates.size())

        closure.shrink_to_fit(); // we store this permanently so lets try to save some space

        if (pa_state) { mt->set_closure(*pa_state, std::move(closure)); }
        else { for (const PredicateIndex_type pred_index: closure) { mt->add_global_predicate(pred_index); } }

    }

}

/**********************************************************************************************************************/

ExpressionSet PredicatesStructure::collect_global_predicates() const {
    PLAJA_ASSERT_EXPECT(PLAJA_GLOBAL::lazyPA) // NOLINT(cert-dcl03-c,misc-static-assert)

    ExpressionSet predicates_set;

    for (const PredicateIndex_type pred_index: mt->get_global_predicates()) {
        if (PLAJA_GLOBAL::debug) { PLAJA_ASSERT(predicates_set.insert(operator[](pred_index)).second) }
        else { predicates_set.insert(operator[](pred_index)); }
    }

    return predicates_set;

}

ExpressionSet PredicatesStructure::collect_predicates(const PaStateBase& pa_state) const {
    PLAJA_ASSERT_EXPECT(PLAJA_GLOBAL::lazyPA) // NOLINT(cert-dcl03-c,misc-static-assert)

    ExpressionSet predicates_set;

    for (auto it = pa_state.init_pred_state_it(); !it.end(); ++it) {

        PLAJA_ASSERT(PLAJA_GLOBAL::lazyPA or it.is_set())

        if (PLAJA_GLOBAL::lazyPA and not it.is_set()) { continue; }

        if (PLAJA_GLOBAL::debug) { PLAJA_ASSERT(predicates_set.insert(operator[](it.predicate_index())).second) }
        else { predicates_set.insert(operator[](it.predicate_index())); }

    }

    return predicates_set;
}

bool PredicatesStructure::check_equivalence(const Expression& pred1, const Expression& pred2) const { return predicateRelations->check_equivalence(pred1, pred2); }

/**********************************************************************************************************************/

#ifndef NDEBUG

bool PredicatesStructure::match(const PaStateBase& pa_state, const ExpressionSet& predicates_set) const {

    PredicatesNumber_type num_preds = 0;

    for (auto it = pa_state.init_pred_state_it(); !it.end(); ++it) {

        PLAJA_ASSERT(PLAJA_GLOBAL::lazyPA or it.is_set())

        if (not it.is_set()) { continue; }

        if (not predicates_set.count(operator[](it.predicate_index()))) { return false; }

        ++num_preds;

    }

    if (predicates_set.size() != num_preds) { return false; }

    return true;
}

#endif

STMT_IF_DEBUG(void PredicatesStructure::dump() const { predicates->dump(); })

/**********************************************************************************************************************/

bool PredicatesStructure::propose_split(class BinaryOpExpression& bound, PLAJA::integer lower_bound, PLAJA::integer upper_bound, const ExpressionSet& predicates_set, const PaStateBase* pa_state) const { // NOLINT(misc-no-recursion)

    std::list<std::pair<PLAJA::integer, PLAJA::integer>> bounds { { lower_bound, upper_bound } };

    while (not bounds.empty()) {

        const auto lb = bounds.front().first;
        const auto ub = bounds.front().second;
        bounds.pop_front();

        if (lb > interestUbInt or ub < interestLbInt) { continue; } // interval not of interest

        if (ub == lb) { // terminal node
            bound.set_right(PLAJA_EXPRESSION::make_int(ub));
            if (not predicates_set.count(&bound) and predicateRelations->check_if_useful(bound, pa_state)) { return true; }
            continue;
        }

        auto candidate = lb + (ub - lb) / 2;
        PLAJA_ASSERT(lb <= candidate and candidate < ub)

        if (interestLbInt <= candidate and candidate <= interestUbInt) {

            bound.set_right(PLAJA_EXPRESSION::make_int(candidate));

            if (not predicates_set.count(&bound) and predicateRelations->check_if_useful(bound, pa_state)) { return true; }

        }

        // coarse-to-fine (with left-to-right as tie-breaking): ([0, 0.5], [0.5, 1]), ([0, 0.25], [0.25, 0.5], [0.5, 0.75], [0.75, 1]), ...
        if (lb < candidate) { bounds.emplace_back(lb, candidate - 1); }
        bounds.emplace_back(candidate + 1, ub);
        // left-to-right: [0, 0.5], [0, 0.25], [0, 0.125], ..., [0.5, 1], [0.5, 0.75], ...
        // bounds.emplace_front(candidate + 1, ub);
        // if (lb < candidate) { bounds.emplace_front(lb, candidate - 1); } // next one

    }

    return false;

}

#ifndef NDEBUG

#include "../../smt_nn/using_marabou.h"

#endif

bool PredicatesStructure::propose_split(class BinaryOpExpression& bound, PLAJA::floating lower_bound, PLAJA::floating upper_bound, const ExpressionSet& predicates_set, const PaStateBase* pa_state) const {

    constexpr auto precision = PREDICATE::predicateSplitPrecision;
    STMT_IF_DEBUG(static_assert(precision / MARABOU_IN_PLAJA::strictnessPrecision >= 10)); // NOLINT(*-avoid-magic-numbers)

    std::list<std::pair<PLAJA::floating, PLAJA::floating>> bounds { { lower_bound, upper_bound } };

    while (not bounds.empty()) {

        const auto lb = bounds.front().first;
        const auto ub = bounds.front().second;
        bounds.pop_front();

        if (PLAJA_FLOATS::gt(lb, interestUbFloat) or PLAJA_FLOATS::lt(ub, interestLbFloat)) { continue; } // interval not of interest

        if (PLAJA_FLOATS::equal(lb, ub, precision)) { continue; } // terminal node

        auto candidate = (lb + ub) / 2;

        PLAJA_ASSERT(PLAJA_FLOATS::lt(lb, candidate, MARABOU_IN_PLAJA::strictnessPrecision) and PLAJA_FLOATS::lt(candidate, ub, MARABOU_IN_PLAJA::strictnessPrecision))

        if (PLAJA_FLOATS::lte(interestLbFloat, candidate) and PLAJA_FLOATS::lte(candidate, interestUbFloat)) {

            bound.set_right(PLAJA_EXPRESSION::make_real(candidate));

            if (not predicates_set.count(&bound) and predicateRelations->check_if_useful(bound, pa_state)) { return true; }

        }

        // coarse-to-fine (with left-to-right as tie-breaking):
        bounds.emplace_back(lb, candidate);
        bounds.emplace_back(candidate, ub);
        // left-to-right:
        // bounds.emplace_front(candidate, ub);
        // bounds.emplace_front(lb, candidate); // next one

    }

    return false;

}

std::unique_ptr<Expression> PredicatesStructure::propose_split(const LValueExpression& var, const ExpressionSet* predicates_set, const PaStateBase* pa_state) const {
    PLAJA_ASSERT(PLAJA_GLOBAL::lazyPA == static_cast<bool>(predicates_set))
    PLAJA_ASSERT(PLAJA_GLOBAL::lazyPA or not pa_state) // NOLINT(cert-dcl03-c,misc-static-assert)

    const auto& model_info = modelZ3->get_model_info();
    const auto state_index = var.get_variable_index();

    if (model_info.is_floating(var.get_variable_index())) {

        const auto lower_bound = model_info.get_lower_bound_float(state_index);
        const auto upper_bound = model_info.get_upper_bound_float(state_index);

        interestLbFloat = splitLb ? splitLb->get_float(state_index) : lower_bound;
        interestUbFloat = splitUb ? splitUb->get_float(state_index) : upper_bound;

        auto bound = BinaryOpExpression::construct_bound(var.deep_copy_lval_exp(), PLAJA_EXPRESSION::make_real(lower_bound), BinaryOpExpression::GE);

        if (propose_split(PLAJA_UTILS::cast_ref<BinaryOpExpression>(*bound), lower_bound, upper_bound, predicates_set ? *predicates_set : predicateSet, pa_state)) {

            bound->determine_type();
            return bound;

        } else {

            // upper bound
            PLAJA_UTILS::cast_ref<BinaryOpExpression>(*bound).set_right(PLAJA_EXPRESSION::make_real(upper_bound));
            if (not(predicates_set ? *predicates_set : predicateSet).count(bound.get()) and predicateRelations->check_if_useful(*bound, pa_state)) { return bound; }

            // lower bound
            auto lb_expr = BinaryOpExpression::construct_bound(var.deep_copy_lval_exp(), PLAJA_EXPRESSION::make_real(lower_bound), BinaryOpExpression::GT);
            if (not(predicates_set ? *predicates_set : predicateSet).count(lb_expr.get()) and predicateRelations->check_if_useful(*lb_expr, pa_state)) { return lb_expr; }

            return nullptr;

        }

    } else {

        const auto lower_bound = model_info.get_lower_bound_int(state_index);
        const auto upper_bound = model_info.get_upper_bound_int(state_index);

        interestLbInt = splitLb ? splitLb->get_int(state_index) : lower_bound;
        interestUbInt = splitUb ? splitUb->get_int(state_index) : upper_bound;

        auto bound = BinaryOpExpression::construct_bound(var.deep_copy_lval_exp(), lower_bound, BinaryOpExpression::GE);

        if (propose_split(PLAJA_UTILS::cast_ref<BinaryOpExpression>(*bound), lower_bound, upper_bound, predicates_set ? *predicates_set : predicateSet, pa_state)) {

            bound->determine_type();
            return bound;

        } else {
            return nullptr;
        }

    }

}
