//
// This file is part of the PlaJA code base.
// Copyright (c) (2019 - 2022) Marcel Vinzent.
// See README.md in the top-level directory for licensing information.
//

#include "to_normalform.h"
#include "../ast/expression/non_standard/state_condition_expression.h"
#include "../ast/expression/unary_op_expression.h"
#include "extern/to_linear_expression.h"
#include "extern/ast_standardization.h"
#include "extern/ast_specialization.h"
#include "extern/visitor_base.h"
#include "normalform/push_down_negation.h"
#include "normalform/distribute_over.h"
#include "normalform/split_equality.h"
#include "normalform/split_on_binary_op.h"
#include "normalform/split_on_locs.h"
#include "linear_constraints_checker.h"

namespace TO_NORMALFORM {

    void negate(std::unique_ptr<Expression>& expr) {
        auto* negated_term = expr.release();
        expr = std::make_unique<UnaryOpExpression>(UnaryOpExpression::NOT);
        PLAJA_UTILS::cast_ref<UnaryOpExpression>(*expr).set_operand(std::unique_ptr<Expression>(negated_term));
        expr->determine_type();
    }

    void push_down_negation(std::unique_ptr<Expression>& expr) { PLAJA_VISITOR::visit_cast<Expression, PushDownNeg::push_down_negation>(expr); }

    void negate_and_push_down(std::unique_ptr<Expression>& expr) {
        TO_NORMALFORM::negate(expr);
        TO_NORMALFORM::push_down_negation(expr);
    }

    void distribute_or_over_and(std::unique_ptr<Expression>& expr) { PLAJA_VISITOR::visit_cast<Expression, DistributeOver::distribute_or_over_and>(expr); }

    void distribute_and_over_or(std::unique_ptr<Expression>& expr) { PLAJA_VISITOR::visit_cast<Expression, DistributeOver::distribute_and_over_or>(expr); }

    /******************************************************************************************************************/

    void to_cnf(std::unique_ptr<Expression>& expr) {
        TO_NORMALFORM::push_down_negation(expr);
        TO_NORMALFORM::distribute_or_over_and(expr);
    }

    void to_dnf(std::unique_ptr<Expression>& expr) {
        TO_NORMALFORM::push_down_negation(expr);
        TO_NORMALFORM::distribute_and_over_or(expr);
    }

    void negation_to_cnf(std::unique_ptr<Expression>& expr) {
        auto* negated_term = expr.release();
        expr = std::make_unique<UnaryOpExpression>(UnaryOpExpression::NOT);
        static_cast<UnaryOpExpression*>(expr.get())->set_operand(std::unique_ptr<Expression>(negated_term)); // NOLINT(cppcoreguidelines-pro-type-static-cast-downcast)
        expr->determine_type();
        TO_NORMALFORM::to_cnf(expr);
    }

    void negation_to_dnf(std::unique_ptr<Expression>& expr) {
        auto* negated_term = expr.release();
        expr = std::make_unique<UnaryOpExpression>(UnaryOpExpression::NOT);
        static_cast<UnaryOpExpression*>(expr.get())->set_operand(std::unique_ptr<Expression>(negated_term)); // NOLINT(cppcoreguidelines-pro-type-static-cast-downcast)
        expr->determine_type();
        TO_NORMALFORM::to_dnf(expr);
    }

    /******************************************************************************************************************/

    void split_equality(std::unique_ptr<Expression>& expr) { SplitEquality::split_equality(expr, SplitEquality::Both); }

    void split_equality_only(std::unique_ptr<Expression>& expr) { SplitEquality::split_equality(expr, SplitEquality::EqualityOnly); }

    void split_inequality_only(std::unique_ptr<Expression>& expr) { SplitEquality::split_equality(expr, SplitEquality::InequalityOnly); }

    /******************************************************************************************************************/

#ifndef NDEBUG

    /** Inefficient way to check multi-arity. Only use in debug.*/
    bool is_multi_ary(const Expression& expr) {

        auto splits = split_conjunction(expr.deepCopy_Exp(), false);

        if (splits.size() > 2) { return true; } // Con?

        if (splits.size() < 2) {

            splits = split_disjunction(std::move(splits.front()), false);
            if (splits.size() > 2) { return true; } // Dis?
        }

        return false;
    }

#endif

    void to_nary(std::unique_ptr<Expression>& expr) {
        ToNary::to_nary(expr, BinaryOpExpression::AND, true);
        ToNary::to_nary(expr, BinaryOpExpression::OR, true);
    }

    std::list<std::unique_ptr<Expression>> split_conjunction(std::unique_ptr<Expression>&& expr, bool nf) {
        if (nf) { TO_NORMALFORM::to_cnf(expr); }
        return SplitOnBinaryOp::split(std::move(expr), BinaryOpExpression::AND);
    }

    std::list<std::unique_ptr<Expression>> split_conjunction(const Expression& expr, bool nf) { return TO_NORMALFORM::split_conjunction(expr.deepCopy_Exp(), nf); }

    std::list<std::unique_ptr<Expression>> split_disjunction(std::unique_ptr<Expression>&& expr, bool nf) {
        if (nf) { TO_NORMALFORM::to_dnf(expr); }
        return SplitOnBinaryOp::split(std::move(expr), BinaryOpExpression::OR);
    }

    std::list<std::unique_ptr<Expression>> split_disjunction(const Expression& expr, bool nf) { return TO_NORMALFORM::split_disjunction(expr.deepCopy_Exp(), nf); }

    std::unique_ptr<Expression> construct_conjunction(std::list<std::unique_ptr<Expression>>&& sub_expressions) { return SplitOnBinaryOp::to_expr(std::move(sub_expressions), BinaryOpExpression::AND); }

    std::unique_ptr<Expression> construct_disjunction(std::list<std::unique_ptr<Expression>>&& sub_expressions) { return SplitOnBinaryOp::to_expr(std::move(sub_expressions), BinaryOpExpression::OR); }

    /******************************************************************************************************************/

    void normalize(std::unique_ptr<Expression>& expr) {
        // AST_STANDARDIZATION::standardize_ast(expr);
        TO_NORMALFORM::push_down_negation(expr);
        TO_NORMALFORM::split_inequality_only(expr);
        // AST_SPECIALIZATION::specialize_ast(expr);
    }

    std::unique_ptr<Expression> normalize(std::unique_ptr<Expression>&& expr) {
        std::unique_ptr<Expression>& expr_ref(expr);
        normalize(expr_ref);
        return std::move(expr);
    }

    std::unique_ptr<Expression> normalize(const Expression& expr) { return TO_NORMALFORM::normalize(expr.deepCopy_Exp()); }

    inline void predicate_to_nf(std::unique_ptr<Expression>& expr) {

        AST_STANDARDIZATION::standardize_ast(expr); // Standardize to keep tests as is.

        PLAJA_ASSERT(LinearConstraintsChecker::is_linear(*expr, LINEAR_CONSTRAINTS_CHECKER::Specification::set_special_and_predicate(LINEAR_CONSTRAINTS_CHECKER::SpecialAstRequest::FALSE, true)))

        if (LinearConstraintsChecker::is_linear_constraint(*expr, LINEAR_CONSTRAINTS_CHECKER::Specification::set_nf_and_predicate(LINEAR_CONSTRAINTS_CHECKER::NFRequest::FALSE, true))) {
            expr = TO_LINEAR_EXP::to_standard_predicate(*expr);
        }

    }

    std::list<std::unique_ptr<Expression>> normalize_and_split(std::unique_ptr<Expression>&& expr, bool as_predicate) {

        /* To CNF. */
        TO_NORMALFORM::push_down_negation(expr);

        /* Equality split. */
        TO_NORMALFORM::split_equality(expr);

        /* Conjunction split. */
        auto splits = TO_NORMALFORM::split_conjunction(std::move(expr), true);

        /* Disjunction split. */
        std::list<std::unique_ptr<Expression>> splits_tmp;
        std::swap(splits, splits_tmp);
        PLAJA_ASSERT(splits.empty())
        for (auto& split: splits_tmp) { splits.splice(splits.cend(), TO_NORMALFORM::split_disjunction(std::move(split), true)); }

        /* Normalize if predicate. */
        if (as_predicate) { for (auto& split: splits) { predicate_to_nf(split); } }

        return splits;
    }

    std::list<std::unique_ptr<Expression>> normalize_and_split(const Expression& expr, bool as_predicate) { return TO_NORMALFORM::normalize_and_split(expr.deepCopy_Exp(), as_predicate); }

    bool is_states_values(const Expression& expr) { return CheckForStatesValues::is_state_or_states_values(expr); }

    std::unique_ptr<StateConditionExpression> split_on_locations(std::unique_ptr<Expression>&& expr) { return SplitOnLocs::split(std::move(expr)); }

    /******************************************************************************************************************/

    void specialize(std::unique_ptr<Expression>& expr) { AST_SPECIALIZATION::specialize_ast(expr); }

    void standardize(std::unique_ptr<Expression>& expr) { AST_STANDARDIZATION::standardize_ast(expr); }

}