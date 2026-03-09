//
// This file is part of the PlaJA code base.
// Copyright (c) (2019 - 2022) Marcel Vinzent.
// See README.md in the top-level directory for licensing information.
//

#include "linear_expression.h"
#include "../../../../search/states/state_base.h"
#include "../../../../utils/floating_utils.h"
#include "../../../visitor/ast_visitor.h"
#include "../../../visitor/ast_visitor_const.h"
#include "../../../visitor/extern/to_linear_expression.h"
#include "../../../visitor/linear_constraints_checker.h"
#include "../../type/bool_type.h"
#include "../../type/int_type.h"
#include "../../type/real_type.h"
#include "../constant_value_expression.h"
#include "../expression_utils.h"
#include "../variable_expression.h"
#include <cmath>

/* Auxiliary. */

bool LinearExpression::is_linear_op(BinaryOpExpression::BinaryOp op) {
    static const std::unordered_set<BinaryOpExpression::BinaryOp> linear_ops { BinaryOpExpression::BinaryOp::LT, BinaryOpExpression::BinaryOp::LE, BinaryOpExpression::BinaryOp::EQ, BinaryOpExpression::BinaryOp::NE, BinaryOpExpression::BinaryOp::GE, BinaryOpExpression::BinaryOp::GT, BinaryOpExpression::BinaryOp::PLUS };
    return linear_ops.count(op);
}

BinaryOpExpression::BinaryOp LinearExpression::invert_op(BinaryOpExpression::BinaryOp op) {
    PLAJA_ASSERT(is_linear_op(op))
    PLAJA_ASSERT(op != BinaryOpExpression::BinaryOp::PLUS)
    switch (op) {
        case BinaryOpExpression::BinaryOp::LT: { return BinaryOpExpression::BinaryOp::GT; }
        case BinaryOpExpression::BinaryOp::LE: { return BinaryOpExpression::BinaryOp::GE; }
        case BinaryOpExpression::BinaryOp::EQ: { return BinaryOpExpression::BinaryOp::EQ; }
        case BinaryOpExpression::BinaryOp::NE: { return BinaryOpExpression::BinaryOp::NE; }
        case BinaryOpExpression::BinaryOp::GE: { return BinaryOpExpression::BinaryOp::LE; }
        case BinaryOpExpression::BinaryOp::GT: { return BinaryOpExpression::BinaryOp::LT; }
        default: { PLAJA_ABORT }
    }
}

/**********************************************************************************************************************/

LinearExpression::Addend::Addend(std::unique_ptr<ConstantValueExpression>&& factor, std::unique_ptr<VariableExpression>&& var_expr):
    factor(std::move(factor))
    , var(std::move(var_expr)) { PLAJA_ASSERT(var) }

LinearExpression::Addend::~Addend() = default;

VariableID_type LinearExpression::Addend::get_var_id() const { return var->get_id(); }

VariableIndex_type LinearExpression::Addend::get_var_index() const { return var->get_index(); }

bool LinearExpression::Addend::is_integer() const { return not factor->is_floating() and not var->is_floating(); }

bool LinearExpression::Addend::operator==(const Addend& other) const { return *(this->factor) == *(other.factor) and *(this->var) == *(other.var); }

std::unique_ptr<Expression> LinearExpression::Addend::to_standard() const {

    if (factor->evaluates_integer_const(1)) { return var->deep_copy(); }

    auto addend_standard = std::make_unique<BinaryOpExpression>(BinaryOpExpression::TIMES);
    addend_standard->set_left(factor->deepCopy_ConstantValueExp());
    addend_standard->set_right(var->deep_copy());
    addend_standard->determine_type();
    return addend_standard;
}

/**********************************************************************************************************************/

LinearExpression::LinearExpression(BinaryOpExpression::BinaryOp linear_op):
    op(linear_op)
    , scalar(PLAJA_EXPRESSION::make_int_value(0)) { PLAJA_ASSERT(is_linear_op(op)) }

LinearExpression::~LinearExpression() = default;

std::unique_ptr<Expression> LinearExpression::construct_bound(std::unique_ptr<Expression>&& var, std::unique_ptr<Expression>&& value, BinaryOpExpression::BinaryOp op) {
    auto bound = BinaryOpExpression::construct_bound(std::move(var), std::move(value), op);
    return TO_LINEAR_EXP::to_linear_constraint(*bound);
}

/* Internals. */

const LinearExpression::Addend* LinearExpression::get_addend(VariableIndex_type var_index) const {
    auto addend_it = addends.find(var_index);
    if (addend_it == addends.end()) { return nullptr; }
    else { return addend_it->second.get(); }
}

LinearExpression::Addend* LinearExpression::get_addend(VariableIndex_type var_index) {
    auto addend_it = addends.find(var_index);
    if (addend_it == addends.end()) { return nullptr; }
    else { return addend_it->second.get(); }
}

void LinearExpression::set_trivially_true() {
    addends.clear();
    set_op(BinaryOpExpression::BinaryOp::GE);
    set_scalar(PLAJA_EXPRESSION::make_int_value(0));
}

void LinearExpression::set_trivially_false() {
    addends.clear();
    set_op(BinaryOpExpression::BinaryOp::GE);
    set_scalar(PLAJA_EXPRESSION::make_int_value(1));
}

void LinearExpression::to_integer() {
    for (auto& var_addend: addends) {
        auto& addend = var_addend.second;
        addend->factor = addend->factor->evaluate_const(); // evaluate_const return integer instance if possible
    }
    scalar = scalar->evaluate_const();
}

/* Construction. */

void LinearExpression::add_addend(std::unique_ptr<ConstantValueExpression>&& factor, std::unique_ptr<VariableExpression>&& var_expr) {
    PLAJA_ASSERT(var_expr->is_integer() or var_expr->is_floating())
    if (factor->evaluates_integer_const(0)) { return; }
    const VariableIndex_type var_index = var_expr->get_index();
    auto addend_it = addends.find(var_index);
    if (addend_it == addends.end()) { addends.emplace(var_index, std::make_unique<Addend>(std::move(factor), std::move(var_expr))); }
    else { // update factor and (special case) remove addend if 0
        auto& addend = addend_it->second;
        addend->factor = addend->factor->add(*factor);
        if (addend->factor->evaluates_integer_const(0)) { remove_addend(addend_it); }
    }
}

void LinearExpression::remove_addend(std::map<VariableIndex_type, std::unique_ptr<Addend>>::iterator addend_it) { addends.erase(addend_it); }

inline void LinearExpression::remove_addend(VariableIndex_type var_index) {
    auto addend_it = addends.find(var_index);
    if (addend_it != addends.end()) { remove_addend(addend_it); }
}

void LinearExpression::add_to_scalar(const ConstantValueExpression& scalar_) { scalar = scalar->add(scalar_); }

void LinearExpression::add(const LinearExpression& lin_expr) {
    PLAJA_ASSERT(lin_expr.is_linear_assignment())
    //
    for (auto addend_it = lin_expr.addendIterator(); !addend_it.end(); ++addend_it) { add_addend(addend_it.factor()->deepCopy_ConstantValueExp(), addend_it.var()->deep_copy()); }
    //
    PLAJA_ASSERT(lin_expr.get_scalar())
    add_to_scalar(*lin_expr.get_scalar());
}

bool LinearExpression::is_addends_integer() const {
    for (auto it = addendIterator(); !it.end(); ++it) { if (not it.addend().is_integer()) { return false; } }
    return true;
}

/* Setter. */

void LinearExpression::set_scalar(std::unique_ptr<ConstantValueExpression>&& scalar_r) { scalar = std::move(scalar_r); }

bool LinearExpression::is_scalar_integer() const { return not scalar->is_floating(); }

/* */

bool LinearExpression::is_bound() const { return is_linear_constraint() and addends.size() == 1 and addendIterator().factor()->evaluates_integer_const(1); }

void LinearExpression::to_nf(bool as_predicate) {

    if (not is_linear_constraint()) {
        PLAJA_ASSERT(not as_predicate) // Do not expect as_predicate for non-constraint.
        to_integer();
        return;
    }

    PLAJA_ASSERT(not addends.empty())

    if constexpr (true) { // First variable coefficient is positive.

        const auto norm_factor_float = addends.begin()->second->factor->evaluate_floating_const();
        PLAJA_ASSERT(not PLAJA_FLOATS::is_zero(norm_factor_float))

        if (norm_factor_float < 0) {
            /* Invert op. */
            set_op(invert_op(op));
            /* Update coefficients. */
            auto norm_factor = PLAJA_EXPRESSION::make_int_value(-1);
            for (const auto& addend: addends) { addend.second->factor = addend.second->factor->multiply(*norm_factor); }
            /* Update scalar. */
            scalar = scalar->multiply(*norm_factor);
        }

    } else if constexpr (false) { // TODO might normalize to 1 (for non-integer sums) or gcd (for integer sums).

        const auto norm_q = addends.begin()->second->factor->evaluate_floating_const();
        PLAJA_ASSERT(not PLAJA_FLOATS::is_zero(norm_q))

        /* Update coefficients. */
        for (const auto& addend: addends) { addend.second->factor = PLAJA_EXPRESSION::make_real_value(addend.second->factor->evaluate_floating_const() / norm_q); }

        /* Negative factor? */
        if (norm_q < 0) { set_op(invert_op(op)); }

        /* Update scalar. */
        scalar = PLAJA_EXPRESSION::make_real_value(scalar->evaluate_floating_const() / norm_q);
    }

    // Special case. */
    if (get_number_addends() == 1) {
        /* Retrieve factor. */
        auto& addend = addends.begin()->second;
        PLAJA_ASSERT(not addend->factor->evaluates_integer_const(0))
        auto factor_rlt = addend->factor->evaluate_floating_const();
        PLAJA_ASSERT(factor_rlt > 0) // Enforced above.
        /* Update factor and scalar. */
        addend->factor = PLAJA_EXPRESSION::make_int_value(1);
        set_scalar(PLAJA_EXPRESSION::make_real_value(scalar->evaluate_floating_const() / factor_rlt));
    }

    to_integer();

    /* Special handling for integer addends. */
    auto const is_integer_addends_c = is_addends_integer();
    auto const is_integer_scalar_c = is_scalar_integer();

    if (is_integer_addends_c) {

        if (op == BinaryOpExpression::BinaryOp::EQ or op == BinaryOpExpression::BinaryOp::NE) { // Equation special case.

            if (not is_integer_scalar_c) { // Trivial true/false for integer variables.
                if (op == BinaryOpExpression::BinaryOp::EQ) { set_trivially_false(); }
                else {
                    PLAJA_ASSERT(op == BinaryOpExpression::BinaryOp::NE)
                    set_trivially_true();
                }
                return;
            }

        } else { // In-equation.

            switch (op) {

                case BinaryOpExpression::LT: {
                    if (is_integer_scalar_c) { set_scalar(PLAJA_EXPRESSION::make_int_value(-1)->add(*scalar)); }
                    else { set_scalar(scalar->floor()); }
                    set_op(BinaryOpExpression::BinaryOp::LE);
                    break;
                }

                    /* a * x <= c -> x <= floor(c/a); a * x >= c -> x >= ceil(c/a) [for a >= 0] */
                case BinaryOpExpression::LE: {
                    if (not is_integer_scalar_c) { set_scalar(scalar->floor()); }
                    break;
                }

                case BinaryOpExpression::GE: {
                    if (not is_integer_scalar_c) { set_scalar(scalar->ceil()); }
                    break;
                }

                case BinaryOpExpression::GT: {
                    if (is_integer_scalar_c) { set_scalar(PLAJA_EXPRESSION::make_int_value(1)->add(*scalar)); }
                    else { set_scalar(scalar->ceil()); }
                    set_op(BinaryOpExpression::BinaryOp::GE);
                    break;
                }

                default: { PLAJA_ABORT }

            }
        }
    }

    /* Enforced above. */
    PLAJA_ASSERT(addends.begin()->second->factor->evaluate_floating_const() > 0)
    PLAJA_ASSERT(not addends.begin()->second->var->is_integer() or scalar->is_integer())
    PLAJA_ASSERT(scalar->is_integer() or not scalar->evaluates_integer_const())

    /* Normalize predicates. */
    if (as_predicate) {

        if (is_integer_addends_c) {

            PLAJA_ASSERT(op == BinaryOpExpression::BinaryOp::LE or op == BinaryOpExpression::BinaryOp::GE) // Enforced above.

            if (op == BinaryOpExpression::LE) { // "sum <= c" is predicate-equivalent to "sum >= c + 1".
                set_op(BinaryOpExpression::GE);
                set_scalar(PLAJA_EXPRESSION::make_int_value(scalar->evaluate_integer_const() + 1));
            }

        } else {

            switch (op) { // Prefer non-strict op (for SMT).

                case BinaryOpExpression::LT: {
                    set_op(BinaryOpExpression::GE);
                    break;
                }

                case BinaryOpExpression::GT: {
                    set_op(BinaryOpExpression::LE);
                    break;
                }

                case BinaryOpExpression::NE: {
                    set_op(BinaryOpExpression::EQ);
                    break;
                }

                default: { break; }
            }

        }

    }

}

/* Override. */

std::unique_ptr<Expression> LinearExpression::to_standard() const {

    /* Special case. */
    if (get_number_addends() == 0) {
        if (is_linear_assignment()) { return scalar->deepCopy_Exp(); }
        else { return PLAJA_EXPRESSION::make_bool(evaluate_integer_const()); }
    }

    /* Addends. */
    std::unique_ptr<Expression> addends_standard;
    for (auto addend_it = addendIterator(); !addend_it.end(); ++addend_it) {

        auto addend_standard = addend_it.to_standard();

        if (addends_standard) {
            std::unique_ptr<BinaryOpExpression> addends_standard_n(new BinaryOpExpression(BinaryOpExpression::PLUS));
            addends_standard_n->set_left(std::move(addends_standard));
            addends_standard_n->set_right(std::move(addend_standard));
            addends_standard_n->determine_type();
            addends_standard = std::move(addends_standard_n);
        } else {
            addends_standard = std::move(addend_standard);
        }

    }

    // special case: x1 + ... + xn + 0
    if (op == BinaryOpExpression::PLUS and scalar->evaluates_integer_const(0)) { return addends_standard; }

    // scalar:
    std::unique_ptr<BinaryOpExpression> standard_expr(new BinaryOpExpression(op));
    standard_expr->set_left(std::move(addends_standard));
    standard_expr->set_right(scalar->deepCopy_Exp());
    standard_expr->determine_type();
    return standard_expr;
}

PLAJA::integer LinearExpression::evaluateInteger(const StateBase* state) const {

    if (is_integer()) {

        PLAJA::integer sum = 0;
        for (auto addend_it = addendIterator(); !addend_it.end(); ++addend_it) { sum += addend_it.factor()->evaluate_integer_const() * addend_it.var()->evaluateInteger(state); }

        switch (op) {
            case BinaryOpExpression::BinaryOp::PLUS: { return sum + scalar->evaluate_integer_const(); }
            case BinaryOpExpression::BinaryOp::LT: { return sum < scalar->evaluate_integer_const(); }
            case BinaryOpExpression::BinaryOp::LE: { return sum <= scalar->evaluate_integer_const(); }
            case BinaryOpExpression::BinaryOp::EQ: { return sum == scalar->evaluate_integer_const(); }
            case BinaryOpExpression::BinaryOp::NE: { return sum != scalar->evaluate_integer_const(); }
            case BinaryOpExpression::BinaryOp::GE: { return sum >= scalar->evaluate_integer_const(); }
            case BinaryOpExpression::BinaryOp::GT: { return sum > scalar->evaluate_integer_const(); }
            default: { PLAJA_ABORT }
        }

    } else {
        PLAJA_ASSERT(op != BinaryOpExpression::BinaryOp::PLUS)

        PLAJA::floating sum = 0;
        for (auto addend_it = addendIterator(); !addend_it.end(); ++addend_it) { sum += addend_it.factor()->evaluate_floating_const() * addend_it.var()->evaluateFloating(state); }

        switch (op) {
            case BinaryOpExpression::BinaryOp::LT: { return PLAJA_FLOATS::lt(sum, scalar->evaluate_floating_const(), PLAJA::floatingPrecision); }
            case BinaryOpExpression::BinaryOp::LE: { return PLAJA_FLOATS::lte(sum, scalar->evaluate_floating_const(), PLAJA::floatingPrecision); }
            case BinaryOpExpression::BinaryOp::EQ: { return PLAJA_FLOATS::equal(sum, scalar->evaluate_floating_const(), PLAJA::floatingPrecision); }
            case BinaryOpExpression::BinaryOp::NE: { return PLAJA_FLOATS::non_equal(sum, scalar->evaluate_floating_const(), PLAJA::floatingPrecision); }
            case BinaryOpExpression::BinaryOp::GE: { return PLAJA_FLOATS::gte(sum, scalar->evaluate_floating_const(), PLAJA::floatingPrecision); }
            case BinaryOpExpression::BinaryOp::GT: { return PLAJA_FLOATS::gt(sum, scalar->evaluate_floating_const(), PLAJA::floatingPrecision); }
            default: { PLAJA_ABORT }
        }

    }

}

PLAJA::floating LinearExpression::evaluateFloating(const StateBase* state) const {

    if (op != BinaryOpExpression::BinaryOp::PLUS) { return evaluateInteger(state); }

    PLAJA::floating sum = 0;
    for (auto addend_it = addendIterator(); !addend_it.end(); ++addend_it) { sum += addend_it.factor()->evaluate_floating_const() * addend_it.var()->evaluateFloating(state); }
    return sum + scalar->evaluate_floating_const();

}

bool LinearExpression::is_constant() const { return addends.empty(); }

bool LinearExpression::equals(const Expression* exp) const {
    auto* other = PLAJA_UTILS::cast_ptr_if<LinearExpression>(exp);
    if (not other) { return false; }

    /* Op. */
    if (this->get_op() != other->get_op()) { return false; }

    /* Scalar. */
    if (*(this->get_scalar()) != *(other->get_scalar())) { return false; }

    /* Addends. */
    if (get_number_addends() != other->get_number_addends()) { return false; }
    for (auto addend_it = addendIterator(); !addend_it.end(); ++addend_it) {
        auto* factor_other = other->get_factor(addend_it.var_index());
        if (not factor_other) { return false; }
        if (*(addend_it.factor()) != *(factor_other)) { return false; }
    }

    return true;
}

std::size_t LinearExpression::hash() const {
    constexpr unsigned prime = 31;
    std::size_t result = 1;

    for (auto addend_it = addendIterator(); !addend_it.end(); ++addend_it) {
        result = prime * result;
        result += addend_it.factor()->hash() * addend_it.var()->hash();
    }

    result = prime * result + op;
    result = prime * result + scalar->hash();

    return result;
}

const DeclarationType* LinearExpression::determine_type() {
    if (not resultType) {

        switch (op) {

            case BinaryOpExpression::BinaryOp::PLUS: {
                if (is_integer()) { resultType = std::make_unique<IntType>(); }
                else { resultType = std::make_unique<RealType>(); }
                break;
            }

            default: {
                resultType = std::make_unique<BoolType>();
                break;
            }

        }

    }
    return resultType.get();
}

void LinearExpression::accept(AstVisitor* ast_visitor) { ast_visitor->visit(this); }

void LinearExpression::accept(AstVisitorConst* ast_visitor) const { ast_visitor->visit(this); }

std::unique_ptr<Expression> LinearExpression::deepCopy_Exp() const { return deep_copy(); }

std::unique_ptr<LinearExpression> LinearExpression::deep_copy() const {
    std::unique_ptr<LinearExpression> copy(new LinearExpression(op));
    SpecialCaseExpression::deep_copy(*copy);
    for (auto addend_it = addendIterator(); !addend_it.end(); ++addend_it) { copy->add_addend(addend_it.factor()->deepCopy_ConstantValueExp(), addend_it.var()->deep_copy()); }
    copy->set_scalar(get_scalar()->deepCopy_ConstantValueExp());
    return copy;
}
