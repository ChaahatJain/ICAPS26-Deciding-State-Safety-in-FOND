//
// This file is part of the PlaJA code base.
// Copyright (c) (2019 - 2023) Marcel Vinzent.
// See README.md in the top-level directory for licensing information.
//

#include "retrieve_variable_bound.h"
#include "../../utils/utils.h"
#include "../ast/expression/special_cases/linear_expression.h"
#include "../ast/expression/bool_value_expression.h"
#include "../ast/expression/expression_utils.h"
#include "../ast/expression/unary_op_expression.h"
#include "../ast/expression/variable_expression.h"

RetrieveVariableBound::RetrieveVariableBound():
    stateIndex(nullptr)
    , op(BinaryOpExpression::GE)
    , value(nullptr)
    , lastExpression(nullptr) {
}

RetrieveVariableBound::~RetrieveVariableBound() = default;

std::unique_ptr<RetrieveVariableBound> RetrieveVariableBound::retrieve_bound(const Expression& expression) {
    std::unique_ptr<RetrieveVariableBound> retrieve_var_bound(new RetrieveVariableBound());

    expression.accept(retrieve_var_bound.get());

    if (retrieve_var_bound->lastExpression == &expression and retrieve_var_bound->has_bound()) { return retrieve_var_bound; }

    return nullptr;
}

/**********************************************************************************************************************/

void RetrieveVariableBound::visit(const BinaryOpExpression* expr) {
    lastExpression = expr;

    switch (expr->get_op()) {
        case BinaryOpExpression::EQ:
        case BinaryOpExpression::LT:
        case BinaryOpExpression::LE:
        case BinaryOpExpression::GT:
        case BinaryOpExpression::GE: break;
        default: { return; }
    }

    const auto* var = PLAJA_UTILS::cast_ptr_if<VariableExpression>(expr->get_left());
    const auto* value_candidate = var ? expr->get_right() : expr->get_left();
    if (not var) { var = PLAJA_UTILS::cast_ptr_if<VariableExpression>(expr->get_right()); }

    if (var and value_candidate->is_constant()) {
        stateIndex = var;
        op = expr->get_op();
        value = value_candidate->evaluate_const();
    }

}

void RetrieveVariableBound::visit(const UnaryOpExpression* expr) {
    lastExpression = expr;

    if (expr->get_op() == UnaryOpExpression::NOT) {

        const auto* var = PLAJA_UTILS::cast_ptr_if<VariableExpression>(expr->get_operand());

        if (var) {
            PLAJA_ASSERT(var->is_boolean())

            stateIndex = var;
            op = BinaryOpExpression::EQ;
            value = std::make_unique<BoolValueExpression>(false);
        }

    }

}

void RetrieveVariableBound::visit(const VariableExpression* expr) {
    lastExpression = expr;

    if (expr->is_boolean()) {
        stateIndex = expr;
        op = BinaryOpExpression::EQ;
        value = std::make_unique<BoolValueExpression>(true);
    }

}

void RetrieveVariableBound::visit(const LinearExpression* expr) {
    lastExpression = expr;

    if (expr->is_bound()) {
        auto it = expr->addendIterator();
        stateIndex = it.var();
        PLAJA_ASSERT(it.factor()->evaluates_integer_const(1))
        op = expr->get_op();
        value = expr->get_scalar()->evaluate_const();
    }

}

/**********************************************************************************************************************/

VariableIndex_type RetrieveVariableBound::get_state_index() const { return stateIndex->get_variable_index(); }

void RetrieveVariableBound::negate() {
    switch (op) {

        case BinaryOpExpression::EQ: {
            if (stateIndex->is_boolean()) {
                PLAJA_ASSERT(value->is_boolean())
                value = std::make_unique<BoolValueExpression>(value->evaluate_integer_const() ? false : true);
            } else {
                op = BinaryOpExpression::NE;
            }
            return;
        }
        case BinaryOpExpression::NE: {
            op = BinaryOpExpression::EQ;
            return;
        }
        case BinaryOpExpression::LT: {
            op = BinaryOpExpression::GE;
            return;
        }
        case BinaryOpExpression::LE: {
            op = BinaryOpExpression::GT;
            return;
        }
        case BinaryOpExpression::GT: {
            op = BinaryOpExpression::LE;
            return;
        }
        case BinaryOpExpression::GE: {
            op = BinaryOpExpression::LT;
            return;
        }
        default: { PLAJA_ABORT }
    }

    PLAJA_ABORT
}