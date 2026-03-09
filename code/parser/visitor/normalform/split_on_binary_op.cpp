//
// This file is part of the PlaJA code base.
// Copyright (c) (2019 - 2023) Marcel Vinzent.
// See README.md in the top-level directory for licensing information.
//

#include <algorithm>
#include "split_on_binary_op.h"
#include "../../../utils/utils.h"
#include "../../ast/expression/non_standard/let_expression.h"
#include "../../ast/expression/non_standard/state_condition_expression.h"
#include "../../ast/expression/special_cases/nary_expression.h"
#include "../ast_element_visitor_const.h"
#include "split_on_locs.h"

class IsSplitOnBinaryOp: private AstElementVisitorConst {

private:
    BinaryOpExpression::BinaryOp op;
    bool rlt; // NOLINT(*-use-default-member-init)

    void visit(const BinaryOpExpression* exp) override;
    void visit(const LetExpression* exp) override;
    void visit(const StateConditionExpression* exp) override;
    void visit(const NaryExpression* exp) override;

    explicit IsSplitOnBinaryOp(BinaryOpExpression::BinaryOp op);
public:
    ~IsSplitOnBinaryOp() override;
    DELETE_CONSTRUCTOR(IsSplitOnBinaryOp)

    [[nodiscard]] static bool is_split(const Expression& expr, BinaryOpExpression::BinaryOp op);

};

IsSplitOnBinaryOp::IsSplitOnBinaryOp(BinaryOpExpression::BinaryOp op):
    op(op)
    , rlt(false) {
}

IsSplitOnBinaryOp::~IsSplitOnBinaryOp() = default;

void IsSplitOnBinaryOp::visit(const BinaryOpExpression* exp) { rlt = exp->get_op() == op; }

void IsSplitOnBinaryOp::visit(const LetExpression* exp) {
    if (exp->get_number_of_free_variables() == 0) { exp->get_expression()->accept(this); }
}

void IsSplitOnBinaryOp::visit(const StateConditionExpression* exp) { rlt = op == BinaryOpExpression::AND and exp->get_constraint(); }

void IsSplitOnBinaryOp::visit(const NaryExpression* exp) { rlt = exp->get_op() == op; }

bool IsSplitOnBinaryOp::is_split(const Expression& expr, BinaryOpExpression::BinaryOp op) {
    IsSplitOnBinaryOp is_split_on_binary_op(op);
    expr.accept(&is_split_on_binary_op);
    return is_split_on_binary_op.rlt;
}

/**********************************************************************************************************************/

SplitOnBinaryOpBase::SplitOnBinaryOpBase(BinaryOpExpression::BinaryOp op):
    op(op) {
}

SplitOnBinaryOpBase::~SplitOnBinaryOpBase() = default;

bool SplitOnBinaryOpBase::is_split(const Expression& expr) const { return IsSplitOnBinaryOp::is_split(expr, op); }

bool SplitOnBinaryOpBase::is_split(const Expression& expr, BinaryOpExpression::BinaryOp op) { return IsSplitOnBinaryOp::is_split(expr, op); }

/**********************************************************************************************************************/

SplitOnBinaryOp::SplitOnBinaryOp(BinaryOpExpression::BinaryOp op):
    SplitOnBinaryOpBase(op) {
}

SplitOnBinaryOp::~SplitOnBinaryOp() = default;

/* */

void SplitOnBinaryOp::visit(BinaryOpExpression* exp) {
    PLAJA_ASSERT(is_split(*exp))

    // left
    if (is_split(*exp->get_left())) { exp->get_left()->accept(this); }
    else { splitExpressions.push_back(exp->set_left()); }

    // right
    if (is_split(*exp->get_right())) { exp->get_right()->accept(this); }
    else { splitExpressions.push_back(exp->set_right()); }

}

void SplitOnBinaryOp::visit(LetExpression* exp) {
    PLAJA_ASSERT(exp->get_expression())
    PLAJA_ASSERT(is_split(*exp))
    PLAJA_ASSERT(is_split(*exp->get_expression()))
    exp->get_expression()->accept(this);
}

void SplitOnBinaryOp::visit(StateConditionExpression* exp) {
    PLAJA_ASSERT(is_split(*exp))

    auto state_var_constraint = exp->set_constraint(nullptr);

    if (exp->get_size_loc_values() > 0) { splitExpressions.push_back(exp->deepCopy_Exp()); }

    if (state_var_constraint) {

        if (is_split(*state_var_constraint)) { state_var_constraint->accept(this); }
        else { splitExpressions.push_back(std::move(state_var_constraint)); }

    }

}

void SplitOnBinaryOp::visit(NaryExpression* exp) {
    PLAJA_ASSERT(is_split(*exp))

    for (auto it = exp->iterator(); !it.end(); ++it) {
        if (is_split(*it)) { it->accept(this); }
        else { splitExpressions.push_back(it.set()); }
    }

}

std::list<std::unique_ptr<Expression>> SplitOnBinaryOp::split(std::unique_ptr<Expression>&& expr, BinaryOpExpression::BinaryOp op) {
#if 0

    std::list<std::unique_ptr<BinaryOpExpression>> expressions_to_split;
    std::list<std::unique_ptr<Expression>> split_expressions;

    if (is_split(*expr, op)) { expressions_to_split.emplace_back(static_cast<BinaryOpExpression*>(expr.release())); } // NOLINT(cppcoreguidelines-pro-type-static-cast-downcast)
    else { split_expressions.push_back(std::move(expr)); }

    while (!expressions_to_split.empty()) {
        auto expr_to_split = std::move(expressions_to_split.front());
        expressions_to_split.pop_front();

        auto left = expr_to_split->set_left();
        if (is_split(*left, op)) {
            expressions_to_split.emplace_back(static_cast<BinaryOpExpression*>(left.release())); // NOLINT(cppcoreguidelines-pro-type-static-cast-downcast)
        } else {
            split_expressions.push_back(std::move(left));
        }

        auto right = expr_to_split->set_right();
        if (is_split(*right, op)) {
            expressions_to_split.emplace_back(static_cast<BinaryOpExpression*>(right.release())); // NOLINT(cppcoreguidelines-pro-type-static-cast-downcast)
        } else {
            split_expressions.push_back(std::move(right));
        }

    }

    if (standardize) { for (auto& split: split_expressions) { AST_STANDARDIZATION::standardize_ast(split); } }

    return split_expressions;

#else

    SplitOnBinaryOp split_on_binary_op(op);

    if (split_on_binary_op.is_split(*expr)) { expr->accept(&split_on_binary_op); }
    else { split_on_binary_op.splitExpressions.push_back(std::move(expr)); }

    return std::move(split_on_binary_op.splitExpressions);

#endif
}

std::list<std::unique_ptr<Expression>> SplitOnBinaryOp::split(const Expression& expr, BinaryOpExpression::BinaryOp op) { return split(expr.deepCopy_Exp(), op); }

std::unique_ptr<Expression> SplitOnBinaryOp::to_expr(std::list<std::unique_ptr<Expression>>&& splits, BinaryOpExpression::BinaryOp op) { // NOLINT(*-no-recursion)
    /* Special case. */
    if (splits.size() <= 2) { return ConstructBinaryOp::construct(op, std::move(splits)); }

    std::unique_ptr<StateConditionExpression> state_condition = op == BinaryOpExpression::AND ? ExtractOnLocs::extract(splits) : nullptr;
    PLAJA_ASSERT(not state_condition or not state_condition->get_constraint()) // Var constraint moved to splits.
    // if (state_condition and state_condition->get_constraint()) { splits.push_back(state_condition->set_constraint()); }

    auto nary = std::make_unique<NaryExpression>(op);
    nary->reserve(splits.size());
    for (auto& split: splits) { nary->add_sub(std::move(split)); }
    nary->determine_type();

    if (state_condition) {
        PLAJA_ASSERT(not state_condition->get_constraint())
        PLAJA_ASSERT(op == BinaryOpExpression::AND)
        state_condition->set_constraint(std::move(nary));
        return state_condition;
    }

    return nary;
}

std::unique_ptr<Expression> SplitOnBinaryOp::to_expr(std::unique_ptr<Expression>&& expr, BinaryOpExpression::BinaryOp op) { return to_expr(split(std::move(expr), op), op); }

/**********************************************************************************************************************/

ToNary::ToNary(BinaryOpExpression::BinaryOp op, bool recursive):
    SplitOnBinaryOpBase(op)
    , recursive(recursive) {
}

ToNary::~ToNary() = default;

/* */

inline void ToNary::process_split(Expression* expr) {
    PLAJA_ASSERT(is_split(*expr))
    auto splits = SplitOnBinaryOp::split(expr->move_exp(), get_op());
    PLAJA_ASSERT(not std::any_of(splits.cbegin(), splits.cend(), [this](const auto& split) { return this->is_split(*split); }))
    if (recursive) { for (auto& split: splits) { to_nary(split, get_op(), recursive); } }
    set_to_replace(SplitOnBinaryOp::to_expr(std::move(splits), get_op()));
}

/* */

void ToNary::visit(BinaryOpExpression* exp) {
    if (is_split(*exp)) { process_split(exp); }
    else if (recursive) { AstVisitor::visit(exp); }
}

void ToNary::visit(LetExpression* exp) {
    if (is_split(*exp)) { process_split(exp); }
    else if (recursive) { AstVisitor::visit(exp); }
}

void ToNary::visit(StateConditionExpression* exp) {
    if (is_split(*exp)) { process_split(exp); }
    else if (recursive) { AstVisitor::visit(exp); }
}

void ToNary::visit(NaryExpression* exp) {
    if (is_split(*exp)) { process_split(exp); }
    else if (recursive) { AstVisitor::visit(exp); }
}

/* */

#if 0

void ToNary::to_nary(std::unique_ptr<Expression>& ast_element, BinaryOpExpression::BinaryOp op, bool recursive) {
    ToNary to_nary(op, recursive);
    ast_element->accept(&to_nary);
    if (to_nary.replace_by_result) { ast_element = to_nary.move_result<AstElement>(); }
}

#endif

void ToNary::to_nary(std::unique_ptr<Expression>& expr, BinaryOpExpression::BinaryOp op, bool recursive) {
#if 0
    auto elem = PLAJA_UTILS::cast_unique<AstElement>(std::move(expr));
    to_nary(elem, op, recursive);
    expr = PLAJA_UTILS::cast_unique<Expression>(std::move(elem));
#else
    ToNary to_nary(op, recursive);
    expr->accept(&to_nary);
    if (to_nary.replace_by_result) { expr = to_nary.move_result<Expression>(); }
#endif
}

#if 0

void ToNary::to_nary(Model& model) {

    ToNary to_nary_and(BinaryOpExpression::AND, true);
    model.accept(&to_nary_and);
    PLAJA_ASSERT(not to_nary_and.replace_by_result)

    ToNary to_nary_or(BinaryOpExpression::OR, true);
    model.accept(&to_nary_or);
    PLAJA_ASSERT(not to_nary_or.replace_by_result)

}

#endif

/**********************************************************************************************************************/

ConstructBinaryOp::ConstructBinaryOp(BinaryOpExpression::BinaryOp op):
    op(op) {
}

ConstructBinaryOp::~ConstructBinaryOp() = default;

/* */

std::unique_ptr<Expression> ConstructBinaryOp::merge(std::unique_ptr<Expression>&& left, std::unique_ptr<Expression>&& right) {

    // this->left = std::move(left_);
    // this->right = std::move(right_);

    // left->accept(this);

    // if (not left) { return std::move(right); }

    // else if (not right) { return std::move(left); }

    // else { // default:

    std::unique_ptr<BinaryOpExpression> rlt_expr(new BinaryOpExpression(op));

    rlt_expr->set_left(std::move(left));
    rlt_expr->set_right(std::move(right));

    rlt_expr->determine_type();

    return rlt_expr;

    // }

}

#if 0

void ConstructBinaryOp::visit(StateConditionExpression* exp) {

    PLAJA_ASSERT(exp == left.get())

    if (op != BinaryOpExpression::AND) { return; }

    if (exp->get_constraint()) {

        std::list<std::unique_ptr<Expression>> sub_expr_list;
        sub_expr_list.push_back(exp->set_constraint(nullptr));
        sub_expr_list.push_back(std::move(right));

        exp->set_constraint(construct(op, std::move(sub_expr_list)));

    } else {

        exp->set_constraint(std::move(right));

    }

}

#endif

/* */

std::unique_ptr<Expression> ConstructBinaryOp::construct(BinaryOpExpression::BinaryOp op, std::list<std::unique_ptr<Expression>>& sub_expressions) { // NOLINT(misc-no-recursion)

    if (sub_expressions.size() == 2) {

        return ConstructBinaryOp(op).merge(std::move(sub_expressions.front()), std::move(sub_expressions.back()));

    } else {

        auto left = std::move(sub_expressions.front());
        sub_expressions.pop_front();
        auto right = construct(op, sub_expressions);

        return ConstructBinaryOp(op).merge(std::move(left), std::move(right));

    }

}

std::unique_ptr<Expression> ConstructBinaryOp::construct(BinaryOpExpression::BinaryOp op, std::list<std::unique_ptr<Expression>>&& sub_expressions) { // NOLINT(*-no-recursion)

    PLAJA_ASSERT(not sub_expressions.empty());

    if (sub_expressions.size() == 1) { return std::move(sub_expressions.front()); }

    std::unique_ptr<StateConditionExpression> state_condition = op == BinaryOpExpression::AND ? ExtractOnLocs::extract(sub_expressions) : nullptr;
    if (state_condition) {
        PLAJA_ASSERT(not state_condition->get_constraint()) // if (state_condition->get_constraint()) { sub_expressions.push_back(state_condition->set_constraint()); } // Var constraint moved to splits.
        state_condition->set_constraint(construct(BinaryOpExpression::AND, std::move(sub_expressions)));
        return state_condition;
    }

    return construct(op, sub_expressions);
}
