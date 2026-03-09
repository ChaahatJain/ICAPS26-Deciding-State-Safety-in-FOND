//
// This file is part of the PlaJA code base.
// Copyright (c) (2019 - 2019) Marcel Vinzent.
// See README.md in the top-level directory for licensing information.
//

#include "expression.h"
#include "../../../exception/not_implemented_exception.h"
#include "../../../utils/floating_utils.h"
#include "../../visitor/extern/to_linear_expression.h"
#include "../type/int_type.h"
#include "constant_value_expression.h"
#include "expression_utils.h"

namespace PLAJA_DEBUG { FIELD_IF_DEBUG(bool printEvalFalseExpression(false);) }

Expression::Expression() = default;

Expression::~Expression() = default;

PLAJA::integer Expression::evaluateInteger(const StateBase* /*state*/) const { throw NotImplementedException(__PRETTY_FUNCTION__); }

PLAJA::floating Expression::evaluateFloating(const StateBase* state) const { return evaluateInteger(state); }

std::vector<PLAJA::integer> Expression::evaluateIntegerArray(const StateBase* /*state*/) const { throw NotImplementedException(__PRETTY_FUNCTION__); }

std::vector<PLAJA::floating> Expression::evaluateFloatingArray(const StateBase* /*state*/) const { throw NotImplementedException(__PRETTY_FUNCTION__); }

std::unique_ptr<ConstantValueExpression> Expression::evaluate(const StateBase* state) const {
    PLAJA_ASSERT(get_type()->is_trivial_type())

    if (get_type()->is_floating_type()) {

        auto rlt = evaluateFloating(state);
        if (PLAJA_FLOATS::is_integer(rlt, PLAJA::integerPrecision)) { return PLAJA_EXPRESSION::make_int_value(static_cast<PLAJA::integer>(rlt)); }
        else { return PLAJA_EXPRESSION::make_real_value(rlt); }

    } else { return PLAJA_EXPRESSION::make_int_value(evaluateInteger(state)); }

}

std::unique_ptr<ConstantValueExpression> Expression::evaluate(const StateBase& state) const { return evaluate(&state); }

std::unique_ptr<ConstantValueExpression> Expression::evaluate_const() const {
    PLAJA_ASSERT(is_constant())
    return evaluate(nullptr);
}

bool Expression::evaluates_floating_type() const { return get_type()->is_floating_type(); }

bool Expression::evaluates_integer(const StateBase* state, const PLAJA::integer* value) const {
    PLAJA_ASSERT(get_type()->is_trivial_type())

    if (evaluates_floating_type()) {

        auto rlt = evaluateFloating(state);
        if (value) { return PLAJA_FLOATS::equals_integer(rlt, *value, PLAJA::integerPrecision); }
        else { return PLAJA_FLOATS::is_integer(rlt, PLAJA::integerPrecision); }

    } else {

        PLAJA_ASSERT(get_type()->is_integer_type() or get_type()->is_boolean_type())
        return not value or *value == evaluateInteger(state);

    }
}

bool Expression::is_constant() const { return false; } // default

bool Expression::equals(const Expression* /*exp*/) const { throw NotImplementedException(__PRETTY_FUNCTION__); }

std::size_t Expression::hash() const { throw NotImplementedException(__PRETTY_FUNCTION__); }

std::unique_ptr<Expression> Expression::move_exp() { throw NotImplementedException(__PRETTY_FUNCTION__); }

// override:

bool Expression::is_proposition() const { return true; }

std::unique_ptr<PropertyExpression> Expression::deepCopy_PropExp() const { return deepCopy_Exp(); }

std::unique_ptr<PropertyExpression> Expression::move_prop_exp() { return move_exp(); }
