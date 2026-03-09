//
// This file is part of the PlaJA code base.
// Copyright (c) (2019 - 2019) Marcel Vinzent.
// See README.md in the top-level directory for licensing information.
//

#include <cmath>
#include <unordered_map>
#include "binary_op_expression.h"
#include "../../../exception/not_implemented_exception.h"
#include "../../../exception/runtime_exception.h"
#include "../../../utils/floating_utils.h"
#include "../../../utils/utils.h"
#include "../../visitor/ast_visitor.h"
#include "../../visitor/ast_visitor_const.h"
#include "../type/bool_type.h"
#include "../type/int_type.h"
#include "../type/real_type.h"
#include "integer_value_expression.h"


// statics:
#if 0
bool BinaryOpExpression::is_logical(int op) { return  op==BinaryOp::OR || op==BinaryOp::AND || op==BinaryOp::IMPLIES; }
bool BinaryOpExpression::is_relOp(int op) { return op==BinaryOp::EQ || op==BinaryOp::NE || op==BinaryOp::LT || op==BinaryOp::LE || op==BinaryOp::GT || op==BinaryOp::GE; }
bool BinaryOpExpression::is_arithmetic(int op) { return op==BinaryOp::PLUS || op==BinaryOp::MINUS || op==BinaryOp::TIMES || op==BinaryOp::MOD || op==BinaryOp::DIV || op==BinaryOp::POW || op==BinaryOp::LOG || op==BinaryOp::MIN || op==BinaryOp::MAX; }
#endif

BinaryOpExpression::BinaryOpExpression(BinaryOp binary_op):
    op(binary_op)
    , left()
    , right() {}

BinaryOpExpression::~BinaryOpExpression() = default;

// static:

namespace BINARY_OP_EXPRESSION {
    const std::string binaryOpToStr[] { "∨", "∧", "=", "≠", "<", "≤", "+", "-", "*", "%", "/", "pow", "log", "⇒", ">", "≥", "min", "max" }; // NOLINT(cert-err58-cpp)
    const std::unordered_map<std::string, BinaryOpExpression::BinaryOp> strToBinaryOp { { "∨",   BinaryOpExpression::OR }, // NOLINT(cert-err58-cpp)
                                                                                        { "∧",   BinaryOpExpression::AND },
                                                                                        { "=",   BinaryOpExpression::EQ },
                                                                                        { "≠",   BinaryOpExpression::NE },
                                                                                        { "<",   BinaryOpExpression::LT },
                                                                                        { "≤",   BinaryOpExpression::LE },
                                                                                        { "+",   BinaryOpExpression::PLUS },
                                                                                        { "-",   BinaryOpExpression::MINUS },
                                                                                        { "*",   BinaryOpExpression::TIMES },
                                                                                        { "%",   BinaryOpExpression::MOD },
                                                                                        { "/",   BinaryOpExpression::DIV },
                                                                                        { "pow", BinaryOpExpression::POW },
                                                                                        { "log", BinaryOpExpression::LOG },
                                                                                        { "⇒",   BinaryOpExpression::IMPLIES },
                                                                                        { ">",   BinaryOpExpression::GT },
                                                                                        { "≥",   BinaryOpExpression::GE },
                                                                                        { "min", BinaryOpExpression::MIN },
                                                                                        { "max", BinaryOpExpression::MAX } };
}

const std::string& BinaryOpExpression::binary_op_to_str(BinaryOpExpression::BinaryOp op) { return BINARY_OP_EXPRESSION::binaryOpToStr[op]; }

std::unique_ptr<BinaryOpExpression::BinaryOp> BinaryOpExpression::str_to_binary_op(const std::string& op_str) {
    auto it = BINARY_OP_EXPRESSION::strToBinaryOp.find(op_str);
    if (it == BINARY_OP_EXPRESSION::strToBinaryOp.end()) { return nullptr; }
    else { return std::make_unique<BinaryOp>(it->second); }
}

std::unique_ptr<Expression> BinaryOpExpression::construct_bound(std::unique_ptr<Expression>&& var, std::unique_ptr<Expression>&& value, BinaryOpExpression::BinaryOp op) {
    PLAJA_ASSERT(var)
    PLAJA_ASSERT(value)
    PLAJA_ASSERT(op == BinaryOpExpression::EQ or op == BinaryOpExpression::LE or op == BinaryOpExpression::GE or op == BinaryOpExpression::LT or op == BinaryOpExpression::GT)
    PLAJA_ASSERT(value->is_constant())
    PLAJA_ASSERT(var->get_type()->is_assignable_from(*value->get_type()));
    auto bound = std::make_unique<BinaryOpExpression>(op);
    bound->set_left(std::move(var));
    bound->set_right(std::move(value));
    bound->determine_type();
    return bound;
}

std::unique_ptr<Expression> BinaryOpExpression::construct_bound(std::unique_ptr<Expression>&& var, PLAJA::integer value, BinaryOp op) {
    PLAJA_ASSERT(var)
    PLAJA_ASSERT(var->get_type()->is_integer_type())
    return construct_bound(std::move(var), std::make_unique<IntegerValueExpression>(value), op);
}

// override:

PLAJA::integer BinaryOpExpression::evaluateInteger(const StateBase* state) const {
    JANI_ASSERT(left and right)
    switch (op) {
        case BinaryOpExpression::OR: { return left->evaluateInteger(state) or right->evaluateInteger(state); }
        case BinaryOpExpression::AND: { return left->evaluateInteger(state) and right->evaluateInteger(state); }
        case BinaryOpExpression::EQ: {
            if (left->evaluates_floating_type() or right->evaluates_floating_type()) { return PLAJA_FLOATS::equal(left->evaluateFloating(state), right->evaluateFloating(state), PLAJA::floatingPrecision); }
            else { return left->evaluateInteger(state) == right->evaluateInteger(state); }
        }
        case BinaryOpExpression::NE: {
            if (left->evaluates_floating_type() or right->evaluates_floating_type()) { return PLAJA_FLOATS::non_equal(left->evaluateFloating(state), right->evaluateFloating(state), PLAJA::floatingPrecision); }
            else { return left->evaluateInteger(state) != right->evaluateInteger(state); }
        }
        case BinaryOpExpression::LT: {
            if (left->evaluates_floating_type() or right->evaluates_floating_type()) { return PLAJA_FLOATS::lt(left->evaluateFloating(state), right->evaluateFloating(state), PLAJA::floatingPrecision); }
            else { return left->evaluateInteger(state) < right->evaluateInteger(state); }
        }
        case BinaryOpExpression::LE: {
            if (left->evaluates_floating_type() or right->evaluates_floating_type()) { return PLAJA_FLOATS::lte(left->evaluateFloating(state), right->evaluateFloating(state), PLAJA::floatingPrecision); }
            else { return left->evaluateInteger(state) <= right->evaluateInteger(state); }
        }
        case BinaryOpExpression::PLUS: { return left->evaluateInteger(state) + right->evaluateInteger(state); }
        case BinaryOpExpression::MINUS: { return left->evaluateInteger(state) - right->evaluateInteger(state); }
        case BinaryOpExpression::TIMES: { return left->evaluateInteger(state) * right->evaluateInteger(state); }
        case BinaryOpExpression::MOD: {
            // euclidean division (https://www.microsoft.com/en-us/research/wp-content/uploads/2016/02/divmodnote-letter.pdf [October 2019])
            const PLAJA::integer val_l = left->evaluateInteger(state);
            const PLAJA::integer val_r = right->evaluateInteger(state);
            RUNTIME_ASSERT(val_r != 0, "MOD BY 0")
            const PLAJA::integer rem = val_l % val_r;
            return rem + val_r + (rem >= 0 ? 0 : (val_r > 0) ? 1 : 0);
        }
            // case BinaryOpExpression::POW: return static_cast<integer>(pow(left->evaluateInteger(state), right->evaluateInteger(state))); // violation of JANI specification, has been used for some special evaluation
        case BinaryOpExpression::IMPLIES: { return (not left->evaluateInteger(state)) or right->evaluateInteger(state); }
        case BinaryOpExpression::GT: {
            if (left->evaluates_floating_type() or right->evaluates_floating_type()) { return PLAJA_FLOATS::gt(left->evaluateFloating(state), right->evaluateFloating(state), PLAJA::floatingPrecision); }
            else { return left->evaluateInteger(state) > right->evaluateInteger(state); }
        }
        case BinaryOpExpression::GE: {
            if (left->evaluates_floating_type() or right->evaluates_floating_type()) { return PLAJA_FLOATS::gte(left->evaluateFloating(state), right->evaluateFloating(state), PLAJA::floatingPrecision); }
            else { return left->evaluateInteger(state) >= right->evaluateInteger(state); }
        }
        case BinaryOpExpression::MIN: { return std::min(left->evaluateInteger(state), right->evaluateInteger(state)); }
        case BinaryOpExpression::MAX: { return std::max(left->evaluateInteger(state), right->evaluateInteger(state)); }
            /* case DIV:
                break;
            case LOG:
                break; */
        default: throw NotImplementedException(__PRETTY_FUNCTION__);
    }
}

namespace PLAJA {

    /* Hack to suppress division-by-0 issue when reusing SMT solutions (on some test benchmarks). */
    FIELD_IF_DEBUG(extern bool define_division_by_zero;)
    FIELD_IF_DEBUG(bool define_division_by_zero = false;)

}

PLAJA::floating BinaryOpExpression::evaluateFloating(const StateBase* state) const {
    JANI_ASSERT(left and right)
    if (not resultType or resultType->is_floating_type()) {
        switch (op) {
            case BinaryOpExpression::PLUS: { return left->evaluateFloating(state) + right->evaluateFloating(state); }
            case BinaryOpExpression::MINUS: { return left->evaluateFloating(state) - right->evaluateFloating(state); }
            case BinaryOpExpression::TIMES: { return left->evaluateFloating(state) * right->evaluateFloating(state); }
            case BinaryOpExpression::DIV: {
                const PLAJA::floating val_r = right->evaluateFloating(state);
                STMT_IF_DEBUG(if (PLAJA::define_division_by_zero and PLAJA_FLOATS::is_zero(val_r)) {
                    PLAJA_LOG_DEBUG(PLAJA_UTILS::to_red_string("Performing division by zero. Quotient defaults to 1."))
                    return left->evaluateFloating(state);
                })
                RUNTIME_ASSERT(0.0 != val_r, "DIV BY 0")
                return left->evaluateFloating(state) / val_r;
            }
            case BinaryOpExpression::POW: { return pow(left->evaluateFloating(state), right->evaluateFloating(state)); }
            case BinaryOpExpression::LOG: {
                PLAJA::floating val_r = right->evaluateFloating(state);
                RUNTIME_ASSERT(0 != val_r and 1 != val_r, "INVALID LOG BASE")
                val_r = log(val_r);
                return log(left->evaluateFloating(state)) / val_r;
            }
            case BinaryOpExpression::MIN: { return std::min(left->evaluateFloating(state), right->evaluateFloating(state)); }
            case BinaryOpExpression::MAX: { return std::max(left->evaluateFloating(state), right->evaluateFloating(state)); }
            default: { throw NotImplementedException(__PRETTY_FUNCTION__); }
        }
    } else { return evaluateInteger(state); }
}

bool BinaryOpExpression::is_constant() const {
    JANI_ASSERT(left and right)
    return left->is_constant() and right->is_constant();
}

bool BinaryOpExpression::equals(const Expression* exp) const {
    JANI_ASSERT(left and right)
    auto* other = PLAJA_UTILS::cast_ptr_if<BinaryOpExpression>(exp);
    if (not other) { return false; }

    JANI_ASSERT(other->left and other->right)
    return this->op == other->op and this->left->equals(other->left.get()) and this->right->equals(other->right.get());
}

std::size_t BinaryOpExpression::hash() const {
    JANI_ASSERT(left && right)
    constexpr unsigned prime = 31;
    std::size_t result = 1;
    result = prime * result + op;
    result = prime * result + (left->hash());
    result = prime * result + (right->hash());
    return result;
}

const DeclarationType* BinaryOpExpression::determine_type() {
    JANI_ASSERT(left and right)
    if (resultType) { return resultType.get(); }
    else {
        switch (op) {
            case BinaryOp::OR:
            case BinaryOp::AND:
            case BinaryOp::EQ:
            case BinaryOp::NE:
            case BinaryOp::LT:
            case BinaryOp::LE: { return (resultType = std::make_unique<BoolType>()).get(); }
            case BinaryOp::PLUS:
            case BinaryOp::MINUS:
            case BinaryOp::TIMES: {
                if (IntType::assignable_from(*left->determine_type()) and IntType::assignable_from(*right->determine_type())) { return (resultType = std::make_unique<IntType>()).get(); }
                else { return (resultType = std::make_unique<RealType>()).get(); }
            }
            case BinaryOp::MOD: { return (resultType = std::make_unique<IntType>()).get(); }
            case BinaryOp::DIV: { return (resultType = std::make_unique<RealType>()).get(); } // NOLINT(bugprone-branch-clone)
            case BinaryOp::POW: {
                // if (IntType::assignable_from(left->determine_type()) and IntType::assignable_from(right->determine_type())) { // violation of JANI specification, pow is always real
                //    return (resultType = std::make_unique<IntType>()).get();
                // } else {
                return (resultType = std::make_unique<RealType>()).get();
                // }
            }
            case BinaryOp::LOG: { return (resultType = std::make_unique<RealType>()).get(); }
            case BinaryOp::IMPLIES:
            case BinaryOp::GT:
            case BinaryOp::GE: { return (resultType = std::make_unique<BoolType>()).get(); }
            case BinaryOp::MIN:
            case BinaryOp::MAX: {
                if (IntType::assignable_from(*left->determine_type()) and IntType::assignable_from(*right->determine_type())) { return (resultType = std::make_unique<IntType>()).get(); }
                else { return (resultType = std::make_unique<RealType>()).get(); }
            }
        }
    }
    PLAJA_ABORT
}

void BinaryOpExpression::accept(AstVisitor* ast_visitor) { ast_visitor->visit(this); }

void BinaryOpExpression::accept(AstVisitorConst* ast_visitor) const { ast_visitor->visit(this); }

std::unique_ptr<Expression> BinaryOpExpression::deepCopy_Exp() const { return deepCopy(); }

std::unique_ptr<Expression> BinaryOpExpression::move_exp() { return move(); }

std::unique_ptr<BinaryOpExpression> BinaryOpExpression::deepCopy() const {
    std::unique_ptr<BinaryOpExpression> copy(new BinaryOpExpression(op));
    if (resultType) { copy->resultType = resultType->deep_copy_decl_type(); }
    if (left) { copy->set_left(left->deepCopy_Exp()); }
    if (right) { copy->set_right(right->deepCopy_Exp()); }
    return copy;
}

std::unique_ptr<BinaryOpExpression> BinaryOpExpression::move() {
    std::unique_ptr<BinaryOpExpression> fresh(new BinaryOpExpression(op));
    fresh->resultType = std::move(resultType);
    fresh->left = std::move(left);
    fresh->right = std::move(right);
    return fresh;
}
