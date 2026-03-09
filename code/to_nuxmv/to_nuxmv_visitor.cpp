//
// This file is part of the PlaJA code base.
// Copyright (c) (2019 - 2024) Marcel Vinzent.
// See README.md in the top-level directory for licensing information.
//

#include <sstream>
#include "to_nuxmv_visitor.h"
#include "../exception/not_supported_exception.h"
#include "../parser/ast/expression/non_standard/let_expression.h"
#include "../parser/ast/expression/non_standard/state_condition_expression.h"
#include "../parser/ast/expression/special_cases/linear_expression.h"
#include "../parser/ast/expression/special_cases/nary_expression.h"
#include "../parser/ast/expression/bool_value_expression.h"
#include "../parser/ast/expression/integer_value_expression.h"
#include "../parser/ast/expression/real_value_expression.h"
#include "../parser/ast/expression/unary_op_expression.h"
#include "../parser/ast/expression/variable_expression.h"
#include "../parser/ast/type/declaration_type.h"
#include "../parser/ast/assignment.h"
#include "../parser/visitor/to_str/precedence_checker.h"
#include "../parser/visitor/extern/ast_specialization.h"
#include "../utils/utils.h"
#include "to_nuxmv.h"
#include "using_nuxmv.h"

namespace TO_NUXMV {

    std::string op_to_str(BinaryOpExpression::BinaryOp op) {

        switch (op) {
            case BinaryOpExpression::OR: { return "|"; }
            case BinaryOpExpression::AND: { return "&"; }
            case BinaryOpExpression::NE: { return "!="; }
            case BinaryOpExpression::LE: { return "<="; }
            case BinaryOpExpression::GE: { return ">="; }
            case BinaryOpExpression::EQ:
            case BinaryOpExpression::LT:
            case BinaryOpExpression::GT:
            case BinaryOpExpression::PLUS:
            case BinaryOpExpression::MINUS:
            case BinaryOpExpression::TIMES: { return BinaryOpExpression::binary_op_to_str(op); }
            default: { PLAJA_ABORT }
        }

        PLAJA_ABORT

    }

    bool is_numeric_env(BinaryOpExpression::BinaryOp op) {

        switch (op) {
            case BinaryOpExpression::OR:
            case BinaryOpExpression::AND:
            case BinaryOpExpression::IMPLIES: { return false; }
            case BinaryOpExpression::NE:
            case BinaryOpExpression::LE:
            case BinaryOpExpression::GE:
            case BinaryOpExpression::EQ:
            case BinaryOpExpression::LT:
            case BinaryOpExpression::GT:
            case BinaryOpExpression::PLUS:
            case BinaryOpExpression::MINUS:
            case BinaryOpExpression::TIMES: { return true; }
            default: { PLAJA_ABORT }
        }

        PLAJA_ABORT

    }

    std::string op_to_str(UnaryOpExpression::UnaryOp op) {

        switch (op) {
            case UnaryOpExpression::NOT: { return "!"; }
            default: { PLAJA_ABORT }
        }

        PLAJA_ABORT

    }

    const std::string notSupportedRemark("to-nuxmv"); // NOLINT(*-err58-cpp)

}

/**********************************************************************************************************************/

std::unique_ptr<std::string> ToNuxmvVisitor::to_nuxmv(const AstElement& ast_element, bool numeric_env, const ToNuxmv& parent) {
    ToNuxmvVisitor to_nuxmv_visitor(parent, numeric_env);
    ast_element.accept(&to_nuxmv_visitor);
    if (not to_nuxmv_visitor.rlt) { throw NotSupportedException(ast_element.to_string(), TO_NUXMV::notSupportedRemark); }
    return std::move(to_nuxmv_visitor.rlt);
}

std::string ToNuxmvVisitor::child_to_str(const AstElement* ast_element, bool numeric_env) const {
    if (not ast_element) { return PLAJA_UTILS::emptyString; }
    ToNuxmvVisitor to_nuxmv_visitor(*parent, numeric_env);
    ast_element->accept(&to_nuxmv_visitor);
    if (not to_nuxmv_visitor.rlt) { throw NotSupportedException(ast_element->to_string(), TO_NUXMV::notSupportedRemark); }
    return std::move(*to_nuxmv_visitor.rlt);
}

/* */

ToNuxmvVisitor::ToNuxmvVisitor(const ToNuxmv& parent, bool numeric_env):
    parent(&parent)
    , numericEnv(numeric_env) {
}

ToNuxmvVisitor::~ToNuxmvVisitor() = default;

/**********************************************************************************************************************/

namespace TO_NUXMV_VISITOR {

    inline void make_unique_str(std::unique_ptr<std::string>& rlt, const std::string& str) { rlt = std::make_unique<std::string>(str); }

    inline void make_unique_str(std::unique_ptr<std::string>& rlt, std::string&& str) { rlt = std::make_unique<std::string>(std::move(str)); }

}

/**********************************************************************************************************************/

void ToNuxmvVisitor::visit(const BoolValueExpression* exp) { TO_NUXMV_VISITOR::make_unique_str(rlt, exp->get_value() ? "TRUE" : "FALSE"); }

void ToNuxmvVisitor::visit(const BinaryOpExpression* exp) {
    const bool numeric_env(TO_NUXMV::is_numeric_env(exp->get_op()));

    std::stringstream ss;

    /* Backwards special case. */
    if (parent->specialize_trivial_bounds()) {
        static const std::unordered_set bound_ops { BinaryOpExpression::LT, BinaryOpExpression::LE, BinaryOpExpression::GE, BinaryOpExpression::GT };
        if (bound_ops.count(exp->get_op()) and exp->get_right()->is_constant() and PLAJA_UTILS::is_dynamic_ptr_type<BinaryOpExpression>(exp->get_left())) {
            const auto* left = PLAJA_UTILS::cast_ptr<BinaryOpExpression>(exp->get_left());
            if (left->get_op() == BinaryOpExpression::PLUS or left->get_op() == BinaryOpExpression::MINUS) {
                const bool has_var = PLAJA_UTILS::is_dynamic_ptr_type<VariableExpression>(left->get_left()) or PLAJA_UTILS::is_dynamic_ptr_type<VariableExpression>(left->get_right());
                const bool has_scalar = left->get_left()->is_constant() or left->get_right()->is_constant();
                if (has_var and has_scalar) {
                    auto exp_copy = exp->deep_copy_ast();
                    AST_SPECIALIZATION::specialize_ast(exp_copy);
                    TO_NUXMV_VISITOR::make_unique_str(rlt, *ToNuxmvVisitor::to_nuxmv(*exp_copy, *parent));
                    return;
                }
            }
        }
    }

    /* Left. */
    if (PLAJA_TO_STR::takes_precedence(*exp->get_left(), exp->get_op(), false)) {
        ss << child_to_str(exp->get_left(), numeric_env);
    } else {
        ss << PLAJA_UTILS::lParenthesisString << child_to_str(exp->get_left(), numeric_env) << PLAJA_UTILS::rParenthesisString;
    }

    ss << PLAJA_UTILS::spaceString << TO_NUXMV::op_to_str(exp->get_op()) << PLAJA_UTILS::spaceString;

    /* Right. */
    if (PLAJA_TO_STR::takes_precedence(*exp->get_right(), exp->get_op(), false)) {
        ss << child_to_str(exp->get_right(), numeric_env);
    } else {
        ss << PLAJA_UTILS::lParenthesisString << child_to_str(exp->get_right(), numeric_env) << PLAJA_UTILS::rParenthesisString;
    }

    TO_NUXMV_VISITOR::make_unique_str(rlt, ss.str());
}

void ToNuxmvVisitor::visit(const IntegerValueExpression* exp) { TO_NUXMV_VISITOR::make_unique_str(rlt, std::to_string(exp->get_value())); }

void ToNuxmvVisitor::visit(const RealValueExpression* exp) { TO_NUXMV_VISITOR::make_unique_str(rlt, PLAJA_UTILS::to_string_with_precision(exp->get_value(), USING_NUXMV::float_precision)); }

void ToNuxmvVisitor::visit(const VariableExpression* exp) {
    TO_NUXMV_VISITOR::make_unique_str(rlt, parent->is_marked_boolean(exp->get_id()) and numericEnv ? ToNuxmv::generate_to_int(exp->get_name()) : exp->get_name());
}

void ToNuxmvVisitor::visit(const UnaryOpExpression* exp) {
    PLAJA_ASSERT(exp->get_type())

    if (PLAJA_TO_STR::takes_precedence(*exp->get_operand(), exp->get_op())) {
        TO_NUXMV_VISITOR::make_unique_str(rlt, TO_NUXMV::op_to_str(exp->get_op()) + child_to_str(exp->get_operand(), exp->get_type()->is_integer_type()));
    } else {
        TO_NUXMV_VISITOR::make_unique_str(rlt, TO_NUXMV::op_to_str(exp->get_op()) + PLAJA_UTILS::lParenthesisString + child_to_str(exp->get_operand(), exp->get_type()->is_integer_type()) + PLAJA_UTILS::rParenthesisString);
    }
}

/* Non-standard. */

void ToNuxmvVisitor::visit(const LetExpression* exp) {
    if (exp->get_number_of_free_variables() > 0) { throw NotSupportedException(exp->to_string(), TO_NUXMV::notSupportedRemark); }
    AST_CONST_ACCEPT_IF(exp->get_expression(), this)
}

void ToNuxmvVisitor::visit(const StateConditionExpression* exp) {
    if (exp->get_size_loc_values() > 1) { throw NotSupportedException(exp->to_string(), TO_NUXMV::notSupportedRemark); }
    AST_CONST_ACCEPT_IF(exp->get_constraint(), this)
}

/* Special cases. */

void ToNuxmvVisitor::visit(const LinearExpression* exp) {
    std::stringstream ss;

    /* Linear sum. */
    auto it = exp->addendIterator();
    /* 1st */
    ss << child_to_str(it.to_standard().get(), true);
    /* 2.. */
    for (++it; !it.end(); ++it) { ss << " + " << child_to_str(it.to_standard().get(), true); }
    /* */

    /* Scalar. */
    if (exp->is_linear_sum()) {
        ss << " + " << child_to_str(exp->get_scalar(), true);
    } else {
        ss << PLAJA_UTILS::spaceString << TO_NUXMV::op_to_str(exp->get_op()) << PLAJA_UTILS::spaceString << child_to_str(exp->get_scalar(), true);
    }

    TO_NUXMV_VISITOR::make_unique_str(rlt, ss.str());

}

void ToNuxmvVisitor::visit(const NaryExpression* exp) {
    const bool numeric_env(TO_NUXMV::is_numeric_env(exp->get_op()));

    std::stringstream ss;
    PLAJA_ASSERT(exp->get_size() > 2)
    auto it = exp->iterator();
    /* 1st */
    if (PLAJA_TO_STR::takes_precedence(*it, exp->get_op(), true)) { ss << child_to_str(it(), numeric_env); } else { ss << PLAJA_UTILS::lParenthesisString << child_to_str(it(), numeric_env) << PLAJA_UTILS::rParenthesisString; }
    /* 2.. */
    for (++it; !it.end(); ++it) {
        if (PLAJA_TO_STR::takes_precedence(*it, exp->get_op(), true)) { ss << PLAJA_UTILS::spaceString << TO_NUXMV::op_to_str(exp->get_op()) << PLAJA_UTILS::spaceString << child_to_str(it(), numeric_env); }
        else { ss << PLAJA_UTILS::spaceString << TO_NUXMV::op_to_str(exp->get_op()) << PLAJA_UTILS::spaceString << PLAJA_UTILS::lParenthesisString << child_to_str(it(), numeric_env) << PLAJA_UTILS::rParenthesisString; }
    }
    /* */
    TO_NUXMV_VISITOR::make_unique_str(rlt, ss.str());
}