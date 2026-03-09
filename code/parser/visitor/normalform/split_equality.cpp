//
// This file is part of the PlaJA code base.
// Copyright (c) (2019 - 2023) Marcel Vinzent.
// See README.md in the top-level directory for licensing information.
//

#include "split_equality.h"
#include "../../ast/expression/special_cases/linear_expression.h"

SplitEquality::SplitEquality(Mode mode):
    mode(mode) {
}

SplitEquality::~SplitEquality() = default;

/*********************************************************************************************************************/

void SplitEquality::split_equality(std::unique_ptr<AstElement>& ast_elem, Mode mode) {
    SplitEquality split_equality(mode);
    ast_elem->accept(&split_equality);
    if (split_equality.replace_by_result) { ast_elem = split_equality.move_result<AstElement>(); }
}

void SplitEquality::split_equality(std::unique_ptr<Expression>& expr, Mode mode) {
    SplitEquality split_equality(mode);
    expr->accept(&split_equality);
    if (split_equality.replace_by_result) { expr = split_equality.move_result<Expression>(); }
}

/*********************************************************************************************************************/

void SplitEquality::visit(BinaryOpExpression* exp) {
    AstVisitor::visit(exp);

    switch (exp->get_op()) {

        case BinaryOpExpression::EQ: {

            if (mode == InequalityOnly) { break; }

            /* <=. */
            auto le_expr = exp->deepCopy();
            le_expr->set_op(BinaryOpExpression::LE);

            /* >=. */
            std::unique_ptr<BinaryOpExpression> ge_expr(new BinaryOpExpression(BinaryOpExpression::GE));
            ge_expr->set_left(exp->set_left());
            ge_expr->set_right(exp->set_right());
            ge_expr->determine_type();

            exp->set_op(BinaryOpExpression::AND);
            exp->set_left(std::move(le_expr));
            exp->set_right(std::move(ge_expr));
            break;
        }

        case BinaryOpExpression::NE: {

            if (mode == EqualityOnly) { break; }

            /* <. */
            auto lt_expr = exp->deepCopy();
            lt_expr->set_op(BinaryOpExpression::LT);

            /* >. */
            std::unique_ptr<BinaryOpExpression> gt_expr(new BinaryOpExpression(BinaryOpExpression::GT));
            gt_expr->set_left(exp->set_left());
            gt_expr->set_right(exp->set_right());
            gt_expr->determine_type();

            exp->set_op(BinaryOpExpression::OR);
            exp->set_left(std::move(lt_expr));
            exp->set_right(std::move(gt_expr));
            break;
        }

        default: { break; }
    }
}

void SplitEquality::visit(LinearExpression* exp) {

    if (exp->get_op() == BinaryOpExpression::EQ) {

        if (mode == InequalityOnly) { return; }

        std::unique_ptr<BinaryOpExpression> conjunction(new BinaryOpExpression(BinaryOpExpression::BinaryOp::AND));

        /* <=. */
        auto le_expr = exp->deep_copy();
        le_expr->set_op(BinaryOpExpression::LE);
        conjunction->set_left(std::move(le_expr));

        /* >= (move existing expr) */
        auto ge_expr = std::unique_ptr<LinearExpression>(new LinearExpression(BinaryOpExpression::GE));
        ge_expr->set_scalar(std::move(exp->scalar));
        ge_expr->addends = std::move(exp->addends);
        ge_expr->determine_type();
        conjunction->set_right(std::move(ge_expr));

        conjunction->determine_type();

        set_to_replace(std::move(conjunction));

    } else if (exp->get_op() == BinaryOpExpression::NE) {

        if (mode == EqualityOnly) { return; }

        std::unique_ptr<BinaryOpExpression> disjunction(new BinaryOpExpression(BinaryOpExpression::BinaryOp::OR));

        /* <. */
        auto lt_expr = exp->deep_copy();
        lt_expr->set_op(BinaryOpExpression::LT);
        lt_expr->to_nf();
        disjunction->set_left(std::move(lt_expr));

        /* >= (move existing expr) */
        auto gt_expr = std::unique_ptr<LinearExpression>(new LinearExpression(BinaryOpExpression::GT));
        gt_expr->set_scalar(std::move(exp->scalar));
        gt_expr->addends = std::move(exp->addends);
        gt_expr->determine_type();
        gt_expr->to_nf();
        disjunction->set_right(std::move(gt_expr));

        disjunction->determine_type();

        set_to_replace(std::move(disjunction));

    }

}