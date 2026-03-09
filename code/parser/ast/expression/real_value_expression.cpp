//
// This file is part of the PlaJA code base.
// Copyright (c) (2019 - 2019) Marcel Vinzent.
// See README.md in the top-level directory for licensing information.
//

#include <cmath>
#include "real_value_expression.h"
#include "../../visitor/ast_visitor.h"
#include "../../visitor/ast_visitor_const.h"
#include "../type/real_type.h"
#include "expression_utils.h"

// extern:

const std::string constantNameToString[] { "e", "π" }; // NOLINT(cert-err58-cpp)
const std::unordered_map<std::string, RealValueExpression::ConstantName> stringToConstantName { { "e", RealValueExpression::EULER_C },
                                                                                                { "π", RealValueExpression::PI_C } }; // NOLINT(cert-err58-cpp)

std::unique_ptr<Expression> PLAJA_EXPRESSION::make_real(PLAJA::floating value) { return std::make_unique<RealValueExpression>(value); }

std::unique_ptr<ConstantValueExpression> PLAJA_EXPRESSION::make_real_value(PLAJA::floating value) { return std::make_unique<RealValueExpression>(value); }

// class:

RealValueExpression::RealValueExpression(ConstantName constant_name, PLAJA::floating value_):
    name(constant_name)
    , value(value_) {

    switch (constant_name) {
        case EULER_C: {
            this->value = static_cast<PLAJA::floating>(exp(1));
            break;
        }
        case PI_C: {
            this->value = M_1_PI;
            break;
        }
        default: { break; }
    }

    resultType = std::make_unique<RealType>();
}

RealValueExpression::RealValueExpression(PLAJA::floating value_):
    name(ConstantName::NONE_C)
    , value(value_) { resultType = std::make_unique<RealType>(); }

RealValueExpression::~RealValueExpression() = default;

// setter:

void RealValueExpression::set(ConstantName constant_name, PLAJA::floating val) {
    name = constant_name;
    switch (name) {
        case EULER_C: {
            value = static_cast<PLAJA::floating>(exp(1));
            break;
        }
        case PI_C: {
            value = M_1_PI;
            break;
        }
        default: { value = val; }
    }
}

// override:

bool RealValueExpression::is_floating() const { return true; }

std::unique_ptr<ConstantValueExpression> RealValueExpression::add(const ConstantValueExpression& addend) const { return std::make_unique<RealValueExpression>(value + addend.evaluate_floating_const()); }

std::unique_ptr<ConstantValueExpression> RealValueExpression::multiply(const ConstantValueExpression& factor) const { return std::make_unique<RealValueExpression>(value * factor.evaluate_floating_const()); }

std::unique_ptr<ConstantValueExpression> RealValueExpression::round() const { return PLAJA_EXPRESSION::make_int_value(PLAJA_UTILS::cast_numeric<PLAJA::integer>(std::round(value))); }

std::unique_ptr<ConstantValueExpression> RealValueExpression::floor() const { return PLAJA_EXPRESSION::make_int_value(PLAJA_UTILS::cast_numeric<PLAJA::integer>(std::floor(value))); }

std::unique_ptr<ConstantValueExpression> RealValueExpression::ceil() const { return PLAJA_EXPRESSION::make_int_value(PLAJA_UTILS::cast_numeric<PLAJA::integer>(std::ceil(value))); }

PLAJA::floating RealValueExpression::evaluateFloating(const StateBase* /*state*/) const { return value; }

bool RealValueExpression::equals(const Expression* exp) const {
    auto* other = PLAJA_UTILS::cast_ptr_if<RealValueExpression>(exp);
    if (not other) { return false; }
    return this->value == other->value;
}

std::size_t RealValueExpression::hash() const { return PLAJA_UTILS::hashFloating(value); }

void RealValueExpression::accept(AstVisitor* ast_visitor) { ast_visitor->visit(this); }

void RealValueExpression::accept(AstVisitorConst* ast_visitor) const { ast_visitor->visit(this); }

std::unique_ptr<ConstantValueExpression> RealValueExpression::deepCopy_ConstantValueExp() const {
    std::unique_ptr<RealValueExpression> copy(new RealValueExpression(name, value));
    if (resultType) { copy->resultType = resultType->deep_copy_decl_type(); }
    return copy;
}




