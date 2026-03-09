//
// This file is part of the PlaJA code base.
// Copyright (c) (2019 - 2023) Marcel Vinzent.
// See README.md in the top-level directory for licensing information.
//

#include <algorithm>
#include "nary_expression.h"
#include "../../../visitor/extern/ast_standardization.h"
#include "../../../visitor/ast_visitor_const.h"
#include "../../../visitor/ast_visitor.h"
#include "../../type/bool_type.h"

bool NaryExpression::is_op(BinaryOpExpression::BinaryOp op) {
    return op == BinaryOpExpression::AND or op == BinaryOpExpression::OR;
}

/*********************************************************************************************************************/

NaryExpression::NaryExpression(BinaryOpExpression::BinaryOp op):
    op(op) {
    PLAJA_ASSERT(is_op(op))
}

NaryExpression::~NaryExpression() = default;

/*********************************************************************************************************************/

/* Construction. */

void NaryExpression::reserve(std::size_t cap) { subExpressions.reserve(cap); }

void NaryExpression::add_sub(std::unique_ptr<Expression>&& sub) { subExpressions.push_back(std::move(sub)); }

/* Override. */

std::unique_ptr<Expression> NaryExpression::to_standard() const { return AST_STANDARDIZATION::standardize_ast<Expression>(*this); }

PLAJA::integer NaryExpression::evaluateInteger(const StateBase* state) const {
    switch (op) {
        case BinaryOpExpression::AND: { return std::all_of(subExpressions.cbegin(), subExpressions.cend(), [state](const std::unique_ptr<Expression>& sub_expr) { return sub_expr->evaluateInteger(state); }); }
        case BinaryOpExpression::OR: { return std::any_of(subExpressions.cbegin(), subExpressions.cend(), [state](const std::unique_ptr<Expression>& sub_expr) { return sub_expr->evaluateInteger(state); }); }
        default: PLAJA_ABORT
    }

    PLAJA_ABORT
}

bool NaryExpression::is_constant() const {
    return std::all_of(subExpressions.cbegin(), subExpressions.cend(), [](const std::unique_ptr<Expression>& sub_expr) { return sub_expr->is_constant(); });
}

bool NaryExpression::equals(const Expression* exp) const {

    auto* other = PLAJA_UTILS::cast_ptr_if<NaryExpression>(exp);

    if (not other) { return false; }

    /* Op. */
    if (this->get_op() != other->get_op()) { return false; }

    /* Sub expressions. */
    const auto size_this = get_size();
    if (size_this != other->get_size()) { return false; }

    for (std::size_t i = 0; i < size_this; ++i) {
        const auto* sub = get_sub(i);
        const auto* sub_other = other->get_sub(i);

        if (not sub) {
            if (sub_other) { return false; }
            else { continue; }
        }

        if (not sub->equals(sub_other)) { return false; }
    }

    return true;

}

std::size_t NaryExpression::hash() const {

    constexpr unsigned prime = 31;
    std::size_t result = 1;
    //
    result = prime * result;
    for (const auto& sub: subExpressions) { result += sub->hash(); }
    result = prime * result + op;
    //
    return result;

}

void NaryExpression::accept(AstVisitor* ast_visitor) { ast_visitor->visit(this); }

void NaryExpression::accept(AstVisitorConst* ast_visitor) const { ast_visitor->visit(this); }

const DeclarationType* NaryExpression::determine_type() {
    resultType = std::make_unique<BoolType>();
    return resultType.get();
}

std::unique_ptr<Expression> NaryExpression::deepCopy_Exp() const { return deep_copy(); }

std::unique_ptr<Expression> NaryExpression::move_exp() { return move(); }

/*********************************************************************************************************************/

std::unique_ptr<NaryExpression> NaryExpression::deep_copy() const {
    std::unique_ptr<NaryExpression> copy(new NaryExpression(op));
    SpecialCaseExpression::deep_copy(*copy);
    for (const auto& sub: subExpressions) { copy->add_sub(sub->deepCopy_Exp()); }
    return copy;
}

std::unique_ptr<NaryExpression> NaryExpression::move() {
    std::unique_ptr<NaryExpression> fresh(new NaryExpression(op));
    SpecialCaseExpression::move(*fresh);
    fresh->subExpressions = std::move(subExpressions);
    return fresh;
}

std::list<std::unique_ptr<Expression>> NaryExpression::split() const {
    std::list<std::unique_ptr<Expression>> splits;
    for (const auto& sub: subExpressions) { splits.push_back(sub->deepCopy_Exp()); }
    return splits;
}

std::list<std::unique_ptr<Expression>> NaryExpression::move_to_split() {
    std::list<std::unique_ptr<Expression>> splits;
    for (auto& sub: subExpressions) { splits.push_back(std::move(sub)); }
    return splits;
}
