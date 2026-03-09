//
// This file is part of the PlaJA code base.
// Copyright (c) (2019 - 2019) Marcel Vinzent.
// See README.md in the top-level directory for licensing information.
//

#include "lvalue_expression.h"
#include "../../../exception/not_implemented_exception.h"
#include "../type/declaration_type.h"

LValueExpression::LValueExpression() = default;
LValueExpression::~LValueExpression() = default;

// short-cuts:

bool LValueExpression::is_boolean() const { return get_type()->has_boolean_base(); }

bool LValueExpression::is_integer() const { return get_type()->has_integer_base(); }

bool LValueExpression::is_floating() const { return get_type()->has_floating_base(); }

// override:

void LValueExpression::assign(StateBase& /*target*/, const StateBase* /*source*/, const Expression* /*value*/) const { throw NotImplementedException(__PRETTY_FUNCTION__); }

PLAJA::integer LValueExpression::access_integer(const StateBase& /*target*/, const StateBase* /*source*/) const { throw NotImplementedException(__PRETTY_FUNCTION__); }

PLAJA::floating LValueExpression::access_floating(const StateBase& /*target*/, const StateBase* /*source*/) const { throw NotImplementedException(__PRETTY_FUNCTION__); }

const Expression* LValueExpression::get_array_index() const { return nullptr; }

bool LValueExpression::is_constant() const { return false; }

std::unique_ptr<Expression> LValueExpression::deepCopy_Exp() const { return deep_copy_lval_exp(); }

