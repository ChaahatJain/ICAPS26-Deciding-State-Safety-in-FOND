//
// This file is part of the PlaJA code base.
// Copyright (c) (2019 - 2019) Marcel Vinzent.
// See README.md in the top-level directory for licensing information.
//

#include <cmath>
#include <unordered_map>
#include "unary_op_expression.h"
#include "../../../exception/not_implemented_exception.h"
#include "../../visitor/ast_visitor.h"
#include "../../visitor/ast_visitor_const.h"
#include "../type/bool_type.h"
#include "../type/int_type.h"
#include "../type/real_type.h"

#if 0
bool UnaryOpExpression::isLogical(int op) { return op==NOT; }
bool UnaryOpExpression::isRelOp(int op) { return false; }
bool UnaryOpExpression::isArithmetic(int op) { return op==FLOOR || op==CEIL || op==ABS || op==SGN || op==TRC; }
#endif

UnaryOpExpression::UnaryOpExpression(UnaryOp unary_op):
    op(unary_op)
    , operand() {}

UnaryOpExpression::~UnaryOpExpression() = default;

// static:

namespace UNARY_OP_EXPRESSION {
    const std::string unaryOpToStr[] { "¬", "floor", "ceil", "abs", "sgn", "trc" }; // NOLINT(cert-err58-cpp)
    const std::unordered_map<std::string, UnaryOpExpression::UnaryOp> strToUnaryOp { { "¬",     UnaryOpExpression::NOT },
                                                                                     { "floor", UnaryOpExpression::FLOOR },
                                                                                     { "ceil",  UnaryOpExpression::CEIL },
                                                                                     { "abs",   UnaryOpExpression::ABS },
                                                                                     { "sgn",   UnaryOpExpression::SGN },
                                                                                     { "trc",   UnaryOpExpression::TRC } }; // NOLINT(cert-err58-cpp)
}

const std::string& UnaryOpExpression::unary_op_to_str(UnaryOpExpression::UnaryOp op) { return UNARY_OP_EXPRESSION::unaryOpToStr[op]; }

std::unique_ptr<UnaryOpExpression::UnaryOp> UnaryOpExpression::str_to_unary_op(const std::string& op_str) {
    auto it = UNARY_OP_EXPRESSION::strToUnaryOp.find(op_str);
    if (it == UNARY_OP_EXPRESSION::strToUnaryOp.end()) { return nullptr; }
    else { return std::make_unique<UnaryOp>(it->second); }
}

// override:

PLAJA::integer UnaryOpExpression::evaluateInteger(const StateBase* state) const {
    JANI_ASSERT(operand)
    switch (op) {
        case UnaryOpExpression::NOT: { return not operand->evaluateInteger(state); }
        case UnaryOpExpression::FLOOR: { return static_cast<PLAJA::integer>(std::floor(operand->evaluateFloating(state))); }
        case UnaryOpExpression::CEIL: { return static_cast<PLAJA::integer>(std::ceil(operand->evaluateFloating(state))); }
        case UnaryOpExpression::ABS: { return std::abs(operand->evaluateInteger(state)); }
        case UnaryOpExpression::SGN: {
            const PLAJA::integer rlt = operand->evaluateInteger(state);
            return rlt == 0 ? 0 : (rlt < 0 ? -1 : 1);
        }
        case UnaryOpExpression::TRC: { return static_cast<PLAJA::integer>(std::trunc(operand->evaluateFloating(state))); }
    }
    PLAJA_ABORT
}

PLAJA::floating UnaryOpExpression::evaluateFloating(const StateBase* state) const {
    JANI_ASSERT(operand)
    if (resultType->is_floating_type()) {
        if (op == UnaryOpExpression::ABS) { return std::abs(operand->evaluateFloating(state)); }
        else { throw NotImplementedException(__PRETTY_FUNCTION__); }
    } else { return evaluateInteger(state); }
}

bool UnaryOpExpression::is_constant() const {
    JANI_ASSERT(operand)
    return operand->is_constant();
}

bool UnaryOpExpression::equals(const Expression* exp) const {
    JANI_ASSERT(operand)
    auto* other = PLAJA_UTILS::cast_ptr_if<UnaryOpExpression>(exp);
    if (not other) { return false; }

    JANI_ASSERT(other->operand)
    return this->op == other->op and this->operand->equals(other->operand.get());
}

std::size_t UnaryOpExpression::hash() const {
    JANI_ASSERT(operand)
    constexpr unsigned prime = 31;
    std::size_t result = 1;
    result = prime * result + op;
    result = prime * result + (operand->hash());
    return result;
}

const DeclarationType* UnaryOpExpression::determine_type() {
    if (resultType) { return resultType.get(); }
    else {
        JANI_ASSERT(operand)
        if (op == NOT) { return (resultType = std::make_unique<BoolType>()).get(); }
        else if (op == UnaryOp::ABS) {
            if (IntType::assignable_from(*operand->determine_type())) { return (resultType = std::make_unique<IntType>()).get(); }
            else { return (resultType = std::make_unique<RealType>()).get(); }
        } else { return (resultType = std::make_unique<IntType>()).get(); }
    }
}

void UnaryOpExpression::accept(AstVisitor* ast_visitor) { ast_visitor->visit(this); }

void UnaryOpExpression::accept(AstVisitorConst* ast_visitor) const { ast_visitor->visit(this); }

std::unique_ptr<Expression> UnaryOpExpression::deepCopy_Exp() const {
    std::unique_ptr<UnaryOpExpression> copy(new UnaryOpExpression(op));
    if (resultType) { copy->resultType = resultType->deep_copy_decl_type(); }
    if (operand) { copy->set_operand(operand->deepCopy_Exp()); }
    return copy;
}
