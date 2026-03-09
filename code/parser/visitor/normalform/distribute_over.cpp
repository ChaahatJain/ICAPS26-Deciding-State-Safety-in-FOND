//
// This file is part of the PlaJA code base.
// Copyright (c) (2019 - 2023) Marcel Vinzent.
// See README.md in the top-level directory for licensing information.
//

#include "distribute_over.h"
#include "../../../utils/utils.h"
#include "../../ast/expression/special_cases/nary_expression.h"
#include "split_on_binary_op.h"
#include "../ast_element_visitor_const.h"

class IsSplitForDistribute: private AstVisitorConst {

private:
    BinaryOpExpression::BinaryOp opToSplit;
    bool rlt; // NOLINT(*-use-default-member-init)

    void visit(const BinaryOpExpression* exp) override;
    void visit(const NaryExpression* exp) override;

    explicit IsSplitForDistribute(BinaryOpExpression::BinaryOp op_to_split);
public:
    ~IsSplitForDistribute() override;
    DELETE_CONSTRUCTOR(IsSplitForDistribute)

    [[nodiscard]] static bool is_split(const Expression& expr, BinaryOpExpression::BinaryOp op_to_split);
    [[nodiscard]] static bool has_split(const NaryExpression& expr, BinaryOpExpression::BinaryOp op_to_split);

};

IsSplitForDistribute::IsSplitForDistribute(BinaryOpExpression::BinaryOp op_to_split):
    opToSplit(op_to_split)
    , rlt(false) {
}

IsSplitForDistribute::~IsSplitForDistribute() = default;

void IsSplitForDistribute::visit(const BinaryOpExpression* exp) { rlt = exp->get_op() == opToSplit; }

void IsSplitForDistribute::visit(const NaryExpression* exp) { rlt = exp->get_op() == opToSplit; }

bool IsSplitForDistribute::is_split(const Expression& expr, BinaryOpExpression::BinaryOp op_to_split) {
    IsSplitForDistribute is_to_distribute(op_to_split);
    expr.accept(&is_to_distribute);
    return is_to_distribute.rlt;
}

bool IsSplitForDistribute::has_split(const NaryExpression& expr, BinaryOpExpression::BinaryOp op_to_split) {
    for (auto it = expr.iterator(); !it.end(); ++it) { if (is_split(*it, op_to_split)) { return true; } }
    return false;
}

/**********************************************************************************************************************/

class SplitForDistribute: private AstVisitor {

private:
    FIELD_IF_DEBUG(BinaryOpExpression::BinaryOp opToSplit;)
    std::list<std::unique_ptr<Expression>> splits;

    void visit(BinaryOpExpression* exp) override;
    void visit(NaryExpression* exp) override;

    explicit SplitForDistribute(BinaryOpExpression::BinaryOp op_to_split);
public:
    ~SplitForDistribute() override;
    DELETE_CONSTRUCTOR(SplitForDistribute)

    static std::list<std::unique_ptr<Expression>> split(std::unique_ptr<Expression>&& split_expr, BinaryOpExpression::BinaryOp op_to_split);

};

#ifdef NDEBUG

SplitForDistribute::SplitForDistribute(BinaryOpExpression::BinaryOp) {}

#else

SplitForDistribute::SplitForDistribute(BinaryOpExpression::BinaryOp op_to_split):
    opToSplit(op_to_split) {
}

#endif

SplitForDistribute::~SplitForDistribute() = default;

void SplitForDistribute::visit(BinaryOpExpression* exp) {
    PLAJA_ASSERT(IsSplitForDistribute::is_split(*exp, opToSplit))
    splits.push_back(exp->set_left(nullptr));
    splits.push_back(exp->set_right(nullptr));
}

void SplitForDistribute::visit(NaryExpression* exp) {
    PLAJA_ASSERT(IsSplitForDistribute::is_split(*exp, opToSplit))
    for (auto it = exp->iterator(); !it.end(); ++it) { splits.push_back(it.set()); }
}

std::list<std::unique_ptr<Expression>> SplitForDistribute::split(std::unique_ptr<Expression>&& split_expr, BinaryOpExpression::BinaryOp op_to_split) {

    if (IsSplitForDistribute::is_split(*split_expr, op_to_split)) {

        SplitForDistribute split_to_distribute(op_to_split);
        split_expr->accept(&split_to_distribute);
        return std::move(split_to_distribute.splits);

    } else {

        std::list<std::unique_ptr<Expression>> splits;
        splits.push_back(std::move(split_expr));
        return splits;

    }

}

/**********************************************************************************************************************/

DistributeOver::DistributeOver(bool or_over_and):
    opToDistribute(or_over_and ? BinaryOpExpression::OR : BinaryOpExpression::AND)
    , opToSplit(or_over_and ? BinaryOpExpression::AND : BinaryOpExpression::OR) {
}

DistributeOver::~DistributeOver() = default;

void DistributeOver::distribute_or_over_and(std::unique_ptr<AstElement>& ast_elem) {
    DistributeOver distribute_or_over_and(true);
    ast_elem->accept(&distribute_or_over_and);
    if (distribute_or_over_and.replace_by_result) { ast_elem = distribute_or_over_and.move_result<AstElement>(); }
}

void DistributeOver::distribute_and_over_or(std::unique_ptr<AstElement>& ast_elem) {
    DistributeOver distribute_and_over_or(false);
    ast_elem->accept(&distribute_and_over_or);
    if (distribute_and_over_or.replace_by_result) { ast_elem = distribute_and_over_or.move_result<AstElement>(); }
}

/*********************************************************************************************************************/

std::unique_ptr<Expression> DistributeOver::distribute(std::unique_ptr<Expression>&& expr_to_split, std::unique_ptr<Expression>&& expr_to_distribute) { // NOLINT(bugprone-easily-swappable-parameters)

    /* Check condition and cache subexpressions depending on which constellation we consider. */
    auto splits = SplitForDistribute::split(std::move(expr_to_split), opToSplit);
    PLAJA_ASSERT(splits.size() >= 2)

    /* Split & distribute. */

    std::list<std::unique_ptr<Expression>> new_subs;

    while (not splits.empty()) {

        std::unique_ptr<BinaryOpExpression> new_sub(new BinaryOpExpression(opToDistribute));
        new_sub->set_left(splits.size() > 1 ? expr_to_distribute->deepCopy_Exp() : std::move(expr_to_distribute)); // Save one copy.
        new_sub->set_right(std::move(splits.front()));
        new_sub->determine_type();

        /* Recursion. */
        new_sub->accept(this);
        if (replace_by_result) { new_subs.push_back(move_result<Expression>()); }
        else { new_subs.push_back(std::move(new_sub)); }

        splits.pop_front();
    }

    return SplitOnBinaryOp::to_expr(std::move(new_subs), opToSplit);
}

void DistributeOver::distribute(NaryExpression& expr) {

    /* Transform. */
    std::list<std::unique_ptr<Expression>> subs;
    for (auto it = expr.iterator(); !it.end(); ++it) { subs.push_back(it.set()); }
    auto binary_transform = ConstructBinaryOp::construct(expr.get_op(), std::move(subs));

    /* Distribute. */
    binary_transform->accept(this);
    if (replace_by_result) { binary_transform = move_result<Expression>(); }

    /* Transform back. */
    ToNary::to_nary(binary_transform, opToSplit, true);
    ToNary::to_nary(binary_transform, opToDistribute, true);

    set_to_replace(std::move(binary_transform));

}

/*********************************************************************************************************************/

void DistributeOver::visit(BinaryOpExpression* exp) {

    if (exp->get_op() != opToDistribute) {
        AstVisitor::visit(exp);
        return;
    }

    if (IsSplitForDistribute::is_split(*exp->get_left(), opToSplit)) {

        set_to_replace(distribute(exp->set_left(), exp->set_right()));

    } else if (IsSplitForDistribute::is_split(*exp->get_right(), opToSplit)) {

        set_to_replace(distribute(exp->set_right(), exp->set_left()));

    } else {

        /* Else: first go over sub-trees. */

        AST_ACCEPT_IF(exp->get_left, exp->set_left, Expression)
        if (IsSplitForDistribute::is_split(*exp->get_left(), opToSplit)) {
            set_to_replace(distribute(exp->set_left(), exp->set_right()));
            return;
        }

        AST_ACCEPT_IF(exp->get_right, exp->set_right, Expression)
        if (IsSplitForDistribute::is_split(*exp->get_right(), opToSplit)) {
            set_to_replace(distribute(exp->set_right(), exp->set_left()));
            return;
        }

    }

}

void DistributeOver::visit(NaryExpression* exp) {

    if (exp->get_op() == opToDistribute) {

        if (IsSplitForDistribute::has_split(*exp, opToSplit)) { distribute(*exp); }
        else {
            AstVisitor::visit(exp);
            if (IsSplitForDistribute::has_split(*exp, opToSplit)) { distribute(*exp); }
        }

    } else { AstVisitor::visit(exp); }

}
