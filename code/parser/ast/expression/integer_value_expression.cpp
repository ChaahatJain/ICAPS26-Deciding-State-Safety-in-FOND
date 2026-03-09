//
// This file is part of the PlaJA code base.
// Copyright (c) (2019 - 2019) Marcel Vinzent.
// See README.md in the top-level directory for licensing information.
//

#include "integer_value_expression.h"
#include "../../visitor/ast_visitor.h"
#include "../../visitor/ast_visitor_const.h"
#include "../type/int_type.h"
#include "expression_utils.h"

std::unique_ptr<Expression> PLAJA_EXPRESSION::make_int(PLAJA::integer value) { return std::make_unique<IntegerValueExpression>(value); }

std::unique_ptr<ConstantValueExpression> PLAJA_EXPRESSION::make_int_value(PLAJA::integer value) { return std::make_unique<IntegerValueExpression>(value); }

IntegerValueExpression::IntegerValueExpression(int val):
    value(val) { resultType = std::make_unique<IntType>(); }

IntegerValueExpression::~IntegerValueExpression() = default;

// override:

bool IntegerValueExpression::is_integer() const { return true; }

std::unique_ptr<ConstantValueExpression> IntegerValueExpression::add(const ConstantValueExpression& addend) const {
    if (addend.get_type()->is_floating_type()) { return PLAJA_EXPRESSION::make_real_value(value + addend.evaluate_floating_const()); }
    else { return PLAJA_EXPRESSION::make_int_value(value + addend.evaluate_integer_const()); }
}

std::unique_ptr<ConstantValueExpression> IntegerValueExpression::multiply(const ConstantValueExpression& factor) const {
    if (factor.get_type()->is_floating_type()) { return PLAJA_EXPRESSION::make_real_value(value * factor.evaluate_floating_const()); }
    else { return PLAJA_EXPRESSION::make_int_value(value * factor.evaluate_integer_const()); }
}

std::unique_ptr<ConstantValueExpression> IntegerValueExpression::round() const { return deepCopy_ConstantValueExp(); }

std::unique_ptr<ConstantValueExpression> IntegerValueExpression::floor() const { return deepCopy_ConstantValueExp(); }

std::unique_ptr<ConstantValueExpression> IntegerValueExpression::ceil() const { return deepCopy_ConstantValueExp(); }

PLAJA::integer IntegerValueExpression::evaluateInteger(const StateBase* /*state*/) const { return value; }

bool IntegerValueExpression::equals(const Expression* exp) const {
    auto* other = PLAJA_UTILS::cast_ptr_if<IntegerValueExpression>(exp);
    if (not other) { return false; }
    return this->value == other->value;
}

std::size_t IntegerValueExpression::hash() const { return PLAJA_UTILS::hashInteger(value); }

void IntegerValueExpression::accept(AstVisitor* ast_visitor) { ast_visitor->visit(this); }

void IntegerValueExpression::accept(AstVisitorConst* ast_visitor) const { ast_visitor->visit(this); }

std::unique_ptr<ConstantValueExpression> IntegerValueExpression::deepCopy_ConstantValueExp() const {
    std::unique_ptr<IntegerValueExpression> copy(new IntegerValueExpression(value));
    if (resultType) { copy->resultType = resultType->deep_copy_decl_type(); }
    return copy;
}








