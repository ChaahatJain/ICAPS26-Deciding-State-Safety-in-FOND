//
// This file is part of the PlaJA code base.
// Copyright (c) (2019 - 2019) Marcel Vinzent.
// See README.md in the top-level directory for licensing information.
//

#include "bool_value_expression.h"
#include "../../visitor/ast_visitor.h"
#include "../../visitor/ast_visitor_const.h"
#include "../type/bool_type.h"
#include "expression_utils.h"

std::unique_ptr<Expression> PLAJA_EXPRESSION::make_bool(bool value) { return std::make_unique<BoolValueExpression>(value); }

std::unique_ptr<ConstantValueExpression> PLAJA_EXPRESSION::make_bool_value(bool value) { return std::make_unique<BoolValueExpression>(value); }

BoolValueExpression::BoolValueExpression(bool value):
    value(value) { resultType = std::make_unique<BoolType>(); }

BoolValueExpression::~BoolValueExpression() = default;

// override:

bool BoolValueExpression::is_boolean() const { return true; }

PLAJA::integer BoolValueExpression::evaluateInteger(const StateBase* /*state*/) const { return value; }

bool BoolValueExpression::equals(const Expression* exp) const {
    auto* other = PLAJA_UTILS::cast_ptr_if<BoolValueExpression>(exp);
    if (not other) { return false; }
    return this->value == other->value;
}

std::size_t BoolValueExpression::hash() const { return PLAJA_UTILS::hashBool(value); }

void BoolValueExpression::accept(AstVisitor* astVisitor) { astVisitor->visit(this); }

void BoolValueExpression::accept(AstVisitorConst* astVisitor) const { astVisitor->visit(this); }

std::unique_ptr<ConstantValueExpression> BoolValueExpression::deepCopy_ConstantValueExp() const {
    std::unique_ptr<BoolValueExpression> copy(new BoolValueExpression(value));
    if (resultType) { copy->resultType = resultType->deep_copy_decl_type(); }
    return copy;
}

