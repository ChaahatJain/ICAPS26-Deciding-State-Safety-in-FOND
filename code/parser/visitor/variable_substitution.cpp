//
// This file is part of the PlaJA code base.
// Copyright (c) (2019 - 2020) Marcel Vinzent.
// See README.md in the top-level directory for licensing information.
//

#include <algorithm>
#include "variable_substitution.h"
#include "../../exception/not_implemented_exception.h"
#include "../../exception/not_supported_exception.h"
#include "../../utils/utils.h"
#include "../ast/expression/non_standard/variable_value_expression.h"
#include "../ast/expression/special_cases/linear_expression.h"
#include "../ast/expression/variable_expression.h"
#include "../ast/expression/array_access_expression.h"
#include "../ast/expression/array_constructor_expression.h"
#include "../ast/expression/array_value_expression.h"
#include "../ast/expression/constant_expression.h"
#include "../ast/expression/constant_value_expression.h"
#include "../ast/expression/expression_utils.h"
#include "../ast/expression/ite_expression.h"
#include "../ast/expression/unary_op_expression.h"
#include "extern/ast_standardization.h"
#include "extern/to_linear_expression.h"
#include "linear_constraints_checker.h"
#include "variables_of.h"

VariableSubstitution::VariableSubstitution(const std::unordered_map<VariableID_type, const Expression*>& id_to_expression):
    substituted(false)
    , idToExpression(id_to_expression) {
}

VariableSubstitution::~VariableSubstitution() = default;

std::unique_ptr<Expression> VariableSubstitution::substitute(const Expression& exp) {
    std::unique_ptr<Expression> substitution_root = exp.deepCopy_Exp();
    substituted = false;
    substitution_root->accept(this);
    if (replace_by_result) {
        return move_result<Expression>();
    } else {
        if (substituted) { return substitution_root; } else { return nullptr; }
    }
}

bool VariableSubstitution::substitute(std::unique_ptr<Expression>& exp) {
    substituted = false;
    exp->accept(this);
    if (replace_by_result) { exp = move_result<Expression>(); }
    return substituted;
}

std::unique_ptr<Expression> VariableSubstitution::substitute(const std::unordered_map<VariableID_type, const Expression*>& id_to_expression, const Expression& exp) {
    VariableSubstitution variable_substitution(id_to_expression);
    return variable_substitution.substitute(exp);
}

bool VariableSubstitution::substitute(const std::unordered_map<VariableID_type, const Expression*>& id_to_expression, std::unique_ptr<Expression>& exp) {
    VariableSubstitution variable_substitution(id_to_expression);
    variable_substitution.substitute(exp);
    return variable_substitution.substituted;
}

/* Expressions. */

void VariableSubstitution::visit(ArrayAccessExpression* exp) {
    JANI_ASSERT(exp->get_accessedArray() && exp->get_index())
    exp->get_accessedArray()->accept(this);
    // handling the "store" expression will result in significant refactoring throughout the codebase
    if (replace_by_result) { throw NotImplementedException(__PRETTY_FUNCTION__); }
    AST_ACCEPT_IF(exp->get_index, exp->set_index, Expression)
}

void VariableSubstitution::visit(DerivativeExpression* /*exp*/) { throw NotImplementedException(__PRETTY_FUNCTION__); }

void VariableSubstitution::visit(VariableExpression* exp) {
    if (idToExpression.count(exp->get_variable_id())) {
        set_to_replace(AST_STANDARDIZATION::standardize_ast(*idToExpression.at(exp->get_variable_id())));
        substituted = true;
    }
}

void VariableSubstitution::visit(LocationValueExpression*) { throw NotImplementedException("Location substitution."); }

void VariableSubstitution::visit(LinearExpression* expr) {
    bool is_sub = false;
    for (auto it = expr->addendIterator(); !it.end(); ++it) {
        if (idToExpression.count(it.var_id())) {
            is_sub = true;
            break;
        }
    }
    if (is_sub) {
        auto expr_standard = expr->to_standard(); // just convert to standard and apply naive substitution ...
        VariableSubstitution::substitute(idToExpression, expr_standard);
        // if still linear expr, convert to special case again ...
        if (TO_LINEAR_EXP::as_linear_exp(*expr_standard)) { set_to_replace(TO_LINEAR_EXP::to_linear_exp(*expr_standard)); }
        else { set_to_replace(std::move(expr_standard)); } // got non-linear keep using standard now ...
        //
        substituted = true;
    }
}

/**********************************************************************************************************************/

#include "../../utils/floating_utils.h"
#include "../ast/assignment.h"

std::unique_ptr<Expression> VariableSubstitution::substitute(const Expression& exp, const std::unordered_map<VariableID_type, const Assignment*>& id_to_sub_non_det) {

    // Are there non-deterministic assignments?
    if (id_to_sub_non_det.empty()) { return substitute(exp); }

    // Is the expression actually affected by non-deterministic assignments?
    {

        auto vars_of = VARIABLES_OF::vars_of(&exp, false);

        if (not std::any_of(id_to_sub_non_det.cbegin(), id_to_sub_non_det.cend(), [vars_of](const std::pair<VariableID_type, const Assignment*> var_bounds) { return vars_of.count(var_bounds.first); })) {
            return substitute(exp);
        }

    }

    // We assume a predicate of the form d_i * y_i [op] d,
    // where d / d_i is integer if y_i is integer.
    std::unique_ptr<LinearExpression> exp_linear(nullptr);
    if (TO_LINEAR_EXP::is_linear_constraint(exp)) {
        exp_linear = PLAJA_UTILS::cast_unique<LinearExpression>(exp.deepCopy_Exp());
        exp_linear->to_predicate_nf();
    } else {
        PLAJA_ASSERT_EXPECT(TO_LINEAR_EXP::as_linear_exp(exp))
        exp_linear = TO_LINEAR_EXP::to_linear_exp(exp);
    }

    PLAJA_ASSERT(exp_linear->is_linear_constraint())

    if (exp_linear->get_number_addends() != 1 or exp_linear->get_op() != BinaryOpExpression::GE
        or (exp_linear->addendIterator().var()->is_integer() and not exp_linear->get_scalar()->is_integer())) {
        throw NotSupportedException(exp.to_string(), "Constraint with non-det. sub.");
    }

    PLAJA_ASSERT(PLAJA_FLOATS::equals_integer(exp_linear->addendIterator().factor()->evaluate_floating_const(), 1, PLAJA::integerPrecision))

    std::unordered_map<VariableID_type, const Expression*> non_det_sub;

    auto addend_it = exp_linear->addendIterator();
    const auto var = addend_it.var_id();
    const auto* factor = addend_it.factor();

    if (PLAJA_FLOATS::is_positive(factor->evaluate_floating_const())) {
        non_det_sub.emplace(var, id_to_sub_non_det.at(var)->get_upper_bound());
    } else {
        non_det_sub.emplace(var, id_to_sub_non_det.at(var)->get_lower_bound());
    }

    // We assume a substitution of the form d_i * y_i + d.
    STMT_IF_DEBUG(
        if (not PLAJA_UTILS::cast_ptr<const ConstantValueExpression>(non_det_sub.begin()->second)) {
            const auto* sub_linear = PLAJA_UTILS::cast_ptr<const LinearExpression>(non_det_sub.begin()->second); // Might easily support case of TO_LINEAR_EXP::as_linear_exp.
            PLAJA_ASSERT(sub_linear)
            PLAJA_ASSERT(sub_linear->is_linear_sum())
            PLAJA_ASSERT(sub_linear->get_number_addends() <= 1)
        }
    )

    auto exp_sub = VariableSubstitution::substitute(non_det_sub, *exp_linear);

    if (exp_sub->is_constant()) {
        PLAJA_ASSERT(exp_sub->evaluate_integer_const())
        return PLAJA_EXPRESSION::make_bool(true);
    }

    STMT_IF_DEBUG(
        const auto* exp_linear_sub = PLAJA_UTILS::cast_ptr<const LinearExpression>(exp_sub.get()); // Might easily support case of TO_LINEAR_EXP::as_linear_exp.
        PLAJA_ASSERT(exp_linear_sub)
        PLAJA_ASSERT(exp_linear_sub->is_linear_constraint())
        PLAJA_ASSERT(exp_linear_sub->get_number_addends() <= 1)
        PLAJA_ASSERT(exp_linear_sub->get_op() == BinaryOpExpression::GE)
        PLAJA_ASSERT(not exp_linear->addendIterator().var()->is_integer() or
                     (exp_linear->get_scalar()->is_integer() and PLAJA_FLOATS::equals_integer(exp_linear->addendIterator().factor()->evaluate_floating_const(), 1, PLAJA::integerPrecision)))
    )

    return exp_sub;

}