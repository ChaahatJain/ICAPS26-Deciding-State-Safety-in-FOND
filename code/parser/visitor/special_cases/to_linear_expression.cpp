//
// This file is part of the PlaJA code base.
// Copyright (c) (2019 - 2020) Marcel Vinzent.
// See README.md in the top-level directory for licensing information.
//

#include "to_linear_expression.h"
#include "../../../utils/utils.h"
#include "../../ast/expression/special_cases/linear_expression.h"
#include "../../ast/expression/integer_value_expression.h"
#include "../../ast/expression/variable_expression.h"
#include "../../ast/expression/expression_utils.h"
#include "../linear_constraints_checker.h"

namespace TO_LINEAR_EXP {

    bool as_linear_exp(const Expression& expr) { return LinearConstraintsChecker::is_linear_constraint(expr, ToLinearExp::get_specification()) or LinearConstraintsChecker::is_linear_sum(expr, ToLinearExp::get_specification()); }

    bool is_linear_exp(const Expression& expr) { return PLAJA_UTILS::is_dynamic_type<LinearExpression>(expr); }

    bool is_linear_sum(const Expression& expr) {
        const auto* lin_expr = PLAJA_UTILS::cast_ref_if<LinearExpression>(expr);
        return lin_expr and lin_expr->is_linear_sum();
    }

    bool is_linear_assignment(const Expression& expr) {
        const auto* lin_expr = PLAJA_UTILS::cast_ref_if<LinearExpression>(expr);
        return lin_expr and lin_expr->is_linear_sum();
    }

    bool is_linear_constraint(const Expression& expr) {
        const auto* lin_expr = PLAJA_UTILS::cast_ref_if<LinearExpression>(expr);
        return lin_expr and lin_expr->is_linear_constraint();
    }

}

/**********************************************************************************************************************/

ToLinearExp::ToLinearExp():
    specification(new LINEAR_CONSTRAINTS_CHECKER::Specification(get_specification()))
    , linearExpression(new LinearExpression(BinaryOpExpression::BinaryOp::PLUS))
    , globalAddendFactor(PLAJA_EXPRESSION::make_int_value(1))
    , globalScalarFactor(PLAJA_EXPRESSION::make_int_value(1)) {
}

ToLinearExp::~ToLinearExp() = default;

LINEAR_CONSTRAINTS_CHECKER::Specification ToLinearExp::get_specification() {
    LINEAR_CONSTRAINTS_CHECKER::Specification specification;

    specification.specialRequest = LINEAR_CONSTRAINTS_CHECKER::SpecialAstRequest::FALSE;
    specification.allowArrayAccess = false;
    specification.allowBools = false;

    return specification;
}

/**/

std::unique_ptr<LinearExpression> ToLinearExp::to_linear_sum(const Expression& expr) {
    PLAJA_ASSERT(LinearConstraintsChecker::is_linear_sum(expr, get_specification()))
    ToLinearExp to_linear_exp;
    to_linear_exp.unset_global_factor();
    to_linear_exp.to_sum(expr);
    to_linear_exp.linearExpression->determine_type();
    to_linear_exp.linearExpression->to_nf();
    return std::move(to_linear_exp.linearExpression);
}

std::unique_ptr<LinearExpression> ToLinearExp::to_linear_constraint(const Expression& expr) {
    PLAJA_ASSERT(LinearConstraintsChecker::is_linear_constraint(expr, get_specification()))
    ToLinearExp to_linear_exp;
    to_linear_exp.unset_global_factor();
    to_linear_exp.to_constraint(expr);
    to_linear_exp.linearExpression->determine_type();
    to_linear_exp.linearExpression->to_nf();
    return std::move(to_linear_exp.linearExpression);
}

std::unique_ptr<LinearExpression> ToLinearExp::to_linear_exp(const Expression& expr) {
    if (LinearConstraintsChecker::is_linear_sum(expr, get_specification())) { return to_linear_sum(expr); }
    if (LinearConstraintsChecker::is_linear_constraint(expr, get_specification())) { return to_linear_constraint(expr); }
    PLAJA_ABORT
}

/**/

inline void ToLinearExp::set_right_of_op() {
    globalAddendFactor = PLAJA_EXPRESSION::make_int_value(-1);
    globalScalarFactor = PLAJA_EXPRESSION::make_int_value(1);
}

inline void ToLinearExp::set_left_of_op() {
    globalAddendFactor = PLAJA_EXPRESSION::make_int_value(1);
    globalScalarFactor = PLAJA_EXPRESSION::make_int_value(-1);
}

inline void ToLinearExp::scale_factor(const ConstantValueExpression& scale) {
    globalAddendFactor = globalAddendFactor->multiply(scale);
    globalScalarFactor = globalScalarFactor->multiply(scale);
}

inline void ToLinearExp::unset_global_factor() {
    globalAddendFactor = PLAJA_EXPRESSION::make_int_value(1);
    globalScalarFactor = PLAJA_EXPRESSION::make_int_value(1);
}

/**/

void ToLinearExp::to_scalar(const Expression& expr) {
    PLAJA_ASSERT(LinearConstraintsChecker::is_scalar(expr))
    const auto expr_const = expr.evaluate_const();
    linearExpression->add_to_scalar(*expr_const->multiply(*globalScalarFactor));
}

void ToLinearExp::to_addend(const Expression& expr) {
    PLAJA_ASSERT(LinearConstraintsChecker::is_addend(expr, *specification))
    if (LinearConstraintsChecker::is_state_variable_index(expr, *specification)) {
        const auto* var_expr = PLAJA_UTILS::cast_ptr<VariableExpression>(&expr);
        PLAJA_ASSERT(var_expr) // for now no array indexes supported
        linearExpression->add_addend(globalAddendFactor->deepCopy_ConstantValueExp(), var_expr->deep_copy());
        return;
    } else {
        const auto& binary_exp = PLAJA_UTILS::cast_ref<BinaryOpExpression>(expr);
        PLAJA_ASSERT(binary_exp.get_op() == BinaryOpExpression::TIMES)
        if (LinearConstraintsChecker::is_state_variable_index(*binary_exp.get_left(), *specification)) { // variable to the left
            const auto* var_expr = PLAJA_UTILS::cast_ptr<VariableExpression>(binary_exp.get_left());
            PLAJA_ASSERT(var_expr)
            PLAJA_ASSERT(LinearConstraintsChecker::is_factor(*binary_exp.get_right()))
            linearExpression->add_addend(binary_exp.get_right()->evaluate_const()->multiply(*globalAddendFactor), var_expr->deep_copy());
            return;
        }
        if (LinearConstraintsChecker::is_state_variable_index(*binary_exp.get_right(), *specification)) { // variable to the right
            const auto* var_expr = PLAJA_UTILS::cast_ptr<VariableExpression>(binary_exp.get_right());
            PLAJA_ASSERT(var_expr)
            PLAJA_ASSERT(LinearConstraintsChecker::is_factor(*binary_exp.get_left()))
            linearExpression->add_addend(binary_exp.get_left()->evaluate_const()->multiply(*globalAddendFactor), var_expr->deep_copy());
            return;
        }
    }
    PLAJA_ABORT
}

void ToLinearExp::to_sum(const Expression& expr) { // NOLINT(misc-no-recursion)

    if (LinearConstraintsChecker::is_addend(expr, *specification)) {
        to_addend(expr);
        return;
    }

    if (LinearConstraintsChecker::is_scalar(expr)) {
        to_scalar(expr);
        return;
    }

    /* Else. */
    const auto& binary_exp = PLAJA_UTILS::cast_ref<BinaryOpExpression>(expr);
    PLAJA_ASSERT(binary_exp.get_left() and binary_exp.get_right())
    switch (binary_exp.get_op()) {
        case BinaryOpExpression::PLUS: {
            to_sum(*binary_exp.get_left());
            to_sum(*binary_exp.get_right());
            return;
        }
        case BinaryOpExpression::MINUS: { // special case
            to_sum(*binary_exp.get_left());
            auto scale = PLAJA_EXPRESSION::make_int_value(-1);
            scale_factor(*scale);
            to_sum(*binary_exp.get_right());
            scale_factor(*scale);
            return;
        }
        case BinaryOpExpression::TIMES: {
            if (LinearConstraintsChecker::is_factor(*binary_exp.get_left())) {
                auto scale = binary_exp.get_left()->evaluate_const();
                scale_factor(*scale);
                to_sum(*binary_exp.get_right());
                scale_factor(*PLAJA_EXPRESSION::make_real_value(1 / scale->evaluate_floating_const())); // descale
            } else {
                PLAJA_ASSERT(LinearConstraintsChecker::is_factor(*binary_exp.get_right()))
                auto scale = binary_exp.get_right()->evaluate_const();
                scale_factor(*scale);
                to_sum(*binary_exp.get_left());
                scale_factor(*PLAJA_EXPRESSION::make_real_value(1 / scale->evaluate_floating_const())); // descale
            }
            return;
        }
        default: PLAJA_ABORT
    }
}

void ToLinearExp::to_constraint(const Expression& expr) {
    PLAJA_ASSERT(LinearConstraintsChecker::is_linear_constraint(expr, *specification))
    const auto& binary_exp = PLAJA_UTILS::cast_ref<BinaryOpExpression>(expr);
    // set op
    PLAJA_ASSERT(LinearExpression::is_linear_op(binary_exp.get_op()))
    linearExpression->set_op(binary_exp.get_op());
    set_left_of_op();
    to_sum(*binary_exp.get_left());
    set_right_of_op();
    to_sum(*binary_exp.get_right());
}

/* Extern. */
namespace TO_LINEAR_EXP {

    std::unique_ptr<LinearExpression> to_linear_sum(const Expression& expr) { return ToLinearExp::to_linear_sum(expr); }

    std::unique_ptr<LinearExpression> to_linear_constraint(const Expression& expr) { return ToLinearExp::to_linear_constraint(expr); }

    std::unique_ptr<LinearExpression> to_linear_exp(const Expression& expr) { return ToLinearExp::to_linear_exp(expr); }

    std::unique_ptr<Expression> to_exp(const Expression& expr) { return ToLinearExp::to_linear_exp(expr); }

    std::unique_ptr<Expression> to_standard(const Expression& expr) { return to_linear_exp(expr)->to_standard(); }

    std::unique_ptr<Expression> to_standard_predicate(const Expression& expr) {
        auto lin_expr = to_linear_exp(expr);
        lin_expr->to_predicate_nf();
        return lin_expr->to_standard();
    }

}