//
// This file is part of the PlaJA code base.
// Copyright (c) (2019 - 2019) Marcel Vinzent.
// See README.md in the top-level directory for licensing information.
//

#include "constant_value_expression.h"
#include "../../../exception/not_implemented_exception.h"
#include "../../visitor/ast_visitor.h"
#include "../../visitor/ast_visitor_const.h"

ConstantValueExpression::ConstantValueExpression() = default;

ConstantValueExpression::~ConstantValueExpression() = default;

//

bool ConstantValueExpression::is_boolean() const { return false; }

bool ConstantValueExpression::is_integer() const { return false; }

bool ConstantValueExpression::is_floating() const { return false; }

// arithmetic:

std::unique_ptr<ConstantValueExpression> ConstantValueExpression::add(const ConstantValueExpression& /*addend*/) const { throw NotImplementedException(__PRETTY_FUNCTION__); }

std::unique_ptr<ConstantValueExpression> ConstantValueExpression::multiply(const ConstantValueExpression& /*factor*/) const { throw NotImplementedException(__PRETTY_FUNCTION__); }

std::unique_ptr<ConstantValueExpression> ConstantValueExpression::round() const { throw NotImplementedException(__PRETTY_FUNCTION__); }

std::unique_ptr<ConstantValueExpression> ConstantValueExpression::floor() const { throw NotImplementedException(__PRETTY_FUNCTION__); }

std::unique_ptr<ConstantValueExpression> ConstantValueExpression::ceil() const { throw NotImplementedException(__PRETTY_FUNCTION__); }

// override:

bool ConstantValueExpression::is_constant() const { return true; }

const DeclarationType* ConstantValueExpression::determine_type() {
    PLAJA_ASSERT(resultType) // Constant value expressions may not return nullptr.
    return resultType.get();
}

std::unique_ptr<Expression> ConstantValueExpression::deepCopy_Exp() const { return deepCopy_ConstantValueExp(); }
