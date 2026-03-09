//
// This file is part of the PlaJA code base.
// Copyright (c) (2019 - 2023) Marcel Vinzent.
// See README.md in the top-level directory for licensing information.
//

#include "push_down_negation.h"
#include "../../ast/expression/non_standard/state_condition_expression.h"
#include "../../ast/expression/special_cases/nary_expression.h"
#include "../../ast/expression/special_cases/linear_expression.h"
#include "../../ast/expression/bool_value_expression.h"
#include "../../ast/expression/unary_op_expression.h"
#include "../../ast/expression/expression_utils.h"
#include "../../ast/type/declaration_type.h"
#include "../extern/visitor_base.h"
#include "../ast_element_visitor_const.h"

class IsNegatable: public AstElementVisitorConst {

// TODO Check on binary/unary expr should include whether operator is negatable.
//  Though, for now we just crash when trying to negate binary/unary op that are not negatable.

private:
    bool result;

    void visit(const BinaryOpExpression*) override { result = true; }

    void visit(const UnaryOpExpression*) override { result = true; }

    void visit(const StateConditionExpression* exp) override { result = (exp->get_size_loc_values() == 0); }

    void visit(const LinearExpression*) override { result = true; }

    void visit(const NaryExpression*) override { result = true; }

    explicit IsNegatable():
        result(false) {
    }

public:
    ~IsNegatable() override = default;
    DELETE_CONSTRUCTOR(IsNegatable)

    static bool is_negatable(const Expression& expr) {
        IsNegatable is_negatable;
        expr.accept(&is_negatable);
        return is_negatable.result;
    }

};

/**********************************************************************************************************************/

PushDownNeg::PushDownNeg():
    negationPushedDown(false) {}

PushDownNeg::~PushDownNeg() = default;

void PushDownNeg::push_down_negation(std::unique_ptr<AstElement>& expr) {
    PushDownNeg push_down_neg;
    expr->accept(&push_down_neg);
    if (push_down_neg.replace_by_result) { expr = push_down_neg.move_result<AstElement>(); }
}

/**/

inline bool PushDownNeg::is_to_negate(const Expression& expr) const { return to_negate() and not IsNegatable::is_negatable(expr); }// if (not to_negate() or typeid(expr) == typeid(BinaryOpExpression) or typeid(expr) == typeid(UnaryOpExpression) ) { return false; }

inline bool PushDownNeg::is_to_negate(const Expression* expr) const {
    PLAJA_ASSERT(expr)
    return is_to_negate(*expr);
}

std::unique_ptr<Expression> PushDownNeg::negate(std::unique_ptr<Expression>&& exp) {
    PLAJA_ASSERT(exp)
    PLAJA_ASSERT(exp->get_type()->is_boolean_type())
    PLAJA_ASSERT(is_to_negate(*exp))

    /* Special case. */
    if (exp->is_constant()) { return std::make_unique<BoolValueExpression>(not exp->evaluate_integer_const()); }

    std::unique_ptr<UnaryOpExpression> negation(new UnaryOpExpression(UnaryOpExpression::NOT));

    /* Internal visit. */
    cancel_negation();
    exp->accept(this);
    reset_negation();

    negation->set_operand(replace_by_result ? move_result<Expression>() : std::move(exp));
    negation->determine_type();
    return negation;
}

/**/

void PushDownNeg::visit(BinaryOpExpression* exp) {

    if (not to_negate() and exp->get_op() != BinaryOpExpression::IMPLIES) {
        AstVisitor::visit(exp);
        return;
    }

    JANI_ASSERT(exp->get_left() && exp->get_right())
    switch (exp->get_op()) {

        case BinaryOpExpression::OR: {
            exp->set_op(BinaryOpExpression::AND);
            if (is_to_negate(exp->get_left())) { exp->set_left(negate(exp->set_left())); }
            else { AST_ACCEPT_IF(exp->get_left, exp->set_left, Expression) }
            if (is_to_negate(exp->get_right())) { exp->set_right(negate(exp->set_right())); }
            else { AST_ACCEPT_IF(exp->get_right, exp->set_right, Expression) }
            break;
        }

        case BinaryOpExpression::AND: {
            exp->set_op(BinaryOpExpression::OR);
            if (is_to_negate(exp->get_left())) { exp->set_left(negate(exp->set_left())); }
            else { AST_ACCEPT_IF(exp->get_left, exp->set_left, Expression) }
            if (is_to_negate(exp->get_right())) { exp->set_right(negate(exp->set_right())); }
            else { AST_ACCEPT_IF(exp->get_right, exp->set_right, Expression) }
            break;
        }

        case BinaryOpExpression::IMPLIES: {
            if (to_negate()) {
                exp->set_op(BinaryOpExpression::AND);
                cancel_negation();
                AST_ACCEPT_IF(exp->get_left, exp->set_left, Expression)
                reset_negation();
                if (is_to_negate(exp->get_right())) { exp->set_right(negate(exp->set_right())); }
                else { AST_ACCEPT_IF(exp->get_right, exp->set_right, Expression) }
            } else {
                exp->set_op(BinaryOpExpression::OR);
                push_negation_down();
                if (is_to_negate(exp->get_left())) { exp->set_left(negate(exp->set_left())); }
                else { AST_ACCEPT_IF(exp->get_left, exp->set_left, Expression) }
                cancel_negation();
                AST_ACCEPT_IF(exp->get_right, exp->set_right, Expression)
            }
            break;
        }

        case BinaryOpExpression::EQ: {
            exp->set_op(BinaryOpExpression::NE);
            cancel_negation();
            AST_ACCEPT_IF(exp->get_left, exp->set_left, Expression)
            AST_ACCEPT_IF(exp->get_right, exp->set_right, Expression)
            reset_negation();
            break;
        }

        case BinaryOpExpression::NE: {
            exp->set_op(BinaryOpExpression::EQ);
            cancel_negation();
            AST_ACCEPT_IF(exp->get_left, exp->set_left, Expression)
            AST_ACCEPT_IF(exp->get_right, exp->set_right, Expression)
            reset_negation();
            break;
        }

        case BinaryOpExpression::LT: {
            exp->set_op(BinaryOpExpression::GE);
            cancel_negation();
            AST_ACCEPT_IF(exp->get_left, exp->set_left, Expression)
            AST_ACCEPT_IF(exp->get_right, exp->set_right, Expression)
            reset_negation();
            break;
        }

        case BinaryOpExpression::LE: {
            exp->set_op(BinaryOpExpression::GT);
            cancel_negation();
            AST_ACCEPT_IF(exp->get_left, exp->set_left, Expression)
            AST_ACCEPT_IF(exp->get_right, exp->set_right, Expression)
            reset_negation();
            break;
        }

        case BinaryOpExpression::GT: {
            exp->set_op(BinaryOpExpression::LE);
            cancel_negation();
            AST_ACCEPT_IF(exp->get_left, exp->set_left, Expression)
            AST_ACCEPT_IF(exp->get_right, exp->set_right, Expression)
            reset_negation();
            break;
        }

        case BinaryOpExpression::GE: {
            exp->set_op(BinaryOpExpression::LT);
            cancel_negation();
            AST_ACCEPT_IF(exp->get_left, exp->set_left, Expression)
            AST_ACCEPT_IF(exp->get_right, exp->set_right, Expression)
            reset_negation();
            break;
        }

        default: { PLAJA_ABORT }
    }
}

void PushDownNeg::visit(UnaryOpExpression* exp) {

    if (not to_negate() and exp->get_op() != UnaryOpExpression::NOT) {
        AstVisitor::visit(exp);
        return;
    }

    JANI_ASSERT(exp->get_operand())

    switch (exp->get_op()) {

        case UnaryOpExpression::NOT: {

            if (to_negate()) { // To negate, cancel out negation.

                cancel_negation();
                exp->get_operand()->accept(this);
                if (not replace_by_result) { set_to_replace(exp->release_operand()); }
                reset_negation();

            } else { // Not to negate, thus push down negation.

                push_negation_down();
                if (is_to_negate(exp->get_operand())) { // Exp is already a negation of a leaf.
                    cancel_negation();
                    if (exp->is_constant()) { set_to_replace(std::make_unique<BoolValueExpression>(exp->evaluate_integer_const())); } // special case
                    else { AST_ACCEPT_IF(exp->get_operand, exp->set_operand, Expression) }
                } else { // push negation down
                    exp->get_operand()->accept(this);
                    if (not replace_by_result) { set_to_replace(exp->release_operand()); }
                }

                cancel_negation();
            }

            break;
        }

        default: { PLAJA_ABORT }

    }
}

void PushDownNeg::visit(StateConditionExpression* exp) {

    if (not to_negate()) {
        AstVisitor::visit(exp);
        return;
    }

    PLAJA_ASSERT(IsNegatable::is_negatable(*exp))
    PLAJA_ASSERT(exp->get_size_loc_values() == 0)

    if (is_to_negate(exp->get_constraint())) { exp->set_constraint(negate(exp->set_constraint())); }
    else { AST_ACCEPT_IF(exp->get_constraint, exp->set_constraint, Expression) }

}

void PushDownNeg::visit(LinearExpression* exp) {
    if (not to_negate()) { return; }

    PLAJA_ASSERT(to_negate())

    switch (exp->get_op()) {

        case BinaryOpExpression::BinaryOp::LT: {
            exp->set_op(BinaryOpExpression::GE);
            break;
        }

        case BinaryOpExpression::BinaryOp::LE: {
            if (exp->is_integer()) {
                exp->set_scalar(PLAJA_EXPRESSION::make_int_value(exp->get_scalar()->evaluate_integer_const() + 1));
                exp->set_op(BinaryOpExpression::GE);
            } else { exp->set_op(BinaryOpExpression::GT); }

            break;
        }

        case BinaryOpExpression::BinaryOp::GE: {
            if (exp->is_integer()) {
                exp->set_scalar(PLAJA_EXPRESSION::make_int_value(exp->get_scalar()->evaluate_integer_const() - 1));
                exp->set_op(BinaryOpExpression::LE);
            } else { exp->set_op(BinaryOpExpression::LT); }

            break;
        }

        case BinaryOpExpression::BinaryOp::GT: {
            exp->set_op(BinaryOpExpression::LE);
            break;
        }

        case BinaryOpExpression::BinaryOp::EQ: {
#if 0 // Support NE in linear expression?
            exp->set_op(BinaryOpExpression::NE);
#else
            std::unique_ptr<BinaryOpExpression> disjunction(new BinaryOpExpression(BinaryOpExpression::BinaryOp::OR));
            /* <. */
            auto le_expr = exp->deep_copy();
            le_expr->set_op(BinaryOpExpression::LT);
            le_expr->to_nf();
            disjunction->set_left(std::move(le_expr));
            /* > (move existing expr). */
            auto ge_expr = std::unique_ptr<LinearExpression>(new LinearExpression(BinaryOpExpression::GT));
            ge_expr->set_scalar(std::move(exp->scalar));
            ge_expr->addends = std::move(exp->addends);
            ge_expr->determine_type();
            ge_expr->to_nf();
            disjunction->set_right(std::move(ge_expr));
            /**/
            set_to_replace(std::move(disjunction));
#endif
            break;
        }

        case BinaryOpExpression::BinaryOp::NE: {
            exp->set_op(BinaryOpExpression::EQ);
            break;
        }

        default: { PLAJA_ABORT }
    }

}

void PushDownNeg::visit(NaryExpression* exp) {
    if (not to_negate()) {
        AstVisitor::visit(exp);
        return;
    }

    switch (exp->get_op()) {

        case BinaryOpExpression::BinaryOp::AND: {
            exp->set_op(BinaryOpExpression::OR);

            for (auto it = exp->iterator(); !it.end(); ++it) {
                if (is_to_negate(*it)) { it.set(negate(it.set())); }
                else {
                    it->accept(this);
                    if (replace_by_result) { it.set(move_result<Expression>()); }
                }
            }

            break;
        }

        case BinaryOpExpression::BinaryOp::OR: {
            exp->set_op(BinaryOpExpression::AND);

            for (auto it = exp->iterator(); !it.end(); ++it) {
                if (is_to_negate(*it)) { it.set(negate(it.set())); }
                else {
                    it->accept(this);
                    if (replace_by_result) { it.set(move_result<Expression>()); }
                }
            }

            break;
        }

        default: PLAJA_ABORT
    }
}
