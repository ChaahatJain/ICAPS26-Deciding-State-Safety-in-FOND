//
// This file is part of the PlaJA code base.
// Copyright (c) (2019 - 2024) Marcel Vinzent.
// See README.md in the top-level directory for licensing information.
//

#include <unordered_set>
#include "precedence_checker.h"
#include "../../ast/expression/special_cases/linear_expression.h"
#include "../../ast/expression/special_cases/nary_expression.h"
#include "../ast_element_visitor_const.h"

class IsAtomic final: public AstElementVisitorConst {

private:
    bool isAtomic;

    /* Expressions:. */
    void visit(const BinaryOpExpression*) override { isAtomic = false; }

    void visit(const UnaryOpExpression*) override { isAtomic = false; }

    /* Non-standard. */

    void visit(const StateConditionExpression*) override { isAtomic = false; }

    /* Special cases. */
    void visit(const LinearExpression*) override { isAtomic = false; }

    void visit(const NaryExpression*) override { isAtomic = false; }

public:
    IsAtomic():
        isAtomic(true) {
    }

    ~IsAtomic() override = default;
    DELETE_CONSTRUCTOR(IsAtomic)

    [[nodiscard]] inline bool is_atomic() const { return isAtomic; }

};

/**********************************************************************************************************************/

class TakesPrecedenceBinary final: public AstElementVisitorConst {

private:
    static const std::unordered_set<BinaryOpExpression::BinaryOp> level0;
    static const std::unordered_set<BinaryOpExpression::BinaryOp> level1;
    static const std::unordered_set<BinaryOpExpression::BinaryOp> level2;
    static const std::unordered_set<BinaryOpExpression::BinaryOp> level3;

    BinaryOpExpression::BinaryOp op;
    bool unorderedJunctions;
    bool takesPrecedence;

    void visit(const BinaryOpExpression* expr) override {

        switch (expr->get_op()) {

            case BinaryOpExpression::OR:
            case BinaryOpExpression::AND: {
                takesPrecedence = unorderedJunctions and expr->get_op() == op; // May ignore con/dis-junction order.
                break;
            }

            case BinaryOpExpression::IMPLIES: {
                takesPrecedence = false;
                break;
            }

            case BinaryOpExpression::EQ:
            case BinaryOpExpression::NE:
            case BinaryOpExpression::LT:
            case BinaryOpExpression::LE:
            case BinaryOpExpression::GT:
            case BinaryOpExpression::GE: {
                takesPrecedence = level0.count(op);
                break;
            }

            case BinaryOpExpression::PLUS:
            case BinaryOpExpression::MINUS: {
                takesPrecedence = level0.count(op) or level1.count(op) or level2.count(op);  // Latter since +/- commutative.
                break;
            }

            case BinaryOpExpression::TIMES:
            case BinaryOpExpression::MOD:
            case BinaryOpExpression::DIV: {
                takesPrecedence = level0.count(op) or level1.count(op) or level2.count(op) or (expr->get_op() == BinaryOpExpression::TIMES and op == BinaryOpExpression::TIMES);
                break;
            }
            default: { takesPrecedence = true; }
        }
    }

    /* Special cases. */

    void visit(const LinearExpression* expr) override {

        if (expr->is_linear_sum()) {
            takesPrecedence = level0.count(op) or level1.count(op) or level2.count(op);
        } else {
            takesPrecedence = level0.count(op);
        }

    }

    void visit(const NaryExpression* expr) override { takesPrecedence = expr->get_op() == op; }

public:
    explicit TakesPrecedenceBinary(BinaryOpExpression::BinaryOp op, bool unordered_junctions):
        op(op)
        , unorderedJunctions(unordered_junctions)
        , takesPrecedence(true) {
    }

    ~TakesPrecedenceBinary() override = default;
    DELETE_CONSTRUCTOR(TakesPrecedenceBinary)

    [[nodiscard]] inline bool takes_precedence() const { return takesPrecedence; }

};

const std::unordered_set<BinaryOpExpression::BinaryOp> TakesPrecedenceBinary::level0 { BinaryOpExpression::OR, BinaryOpExpression::AND, BinaryOpExpression::IMPLIES }; // NOLINT(cert-err58-cpp)
const std::unordered_set<BinaryOpExpression::BinaryOp> TakesPrecedenceBinary::level1 { BinaryOpExpression::EQ, BinaryOpExpression::NE, BinaryOpExpression::LT, BinaryOpExpression::LE, BinaryOpExpression::GT, BinaryOpExpression::GE }; // NOLINT(cert-err58-cpp)
const std::unordered_set<BinaryOpExpression::BinaryOp> TakesPrecedenceBinary::level2 { BinaryOpExpression::PLUS, BinaryOpExpression::MINUS }; // NOLINT(cert-err58-cpp)
const std::unordered_set<BinaryOpExpression::BinaryOp> TakesPrecedenceBinary::level3 { BinaryOpExpression::TIMES, BinaryOpExpression::MOD, BinaryOpExpression::DIV }; // NOLINT(cert-err58-cpp)

/**********************************************************************************************************************/

namespace PLAJA_TO_STR {

    bool is_atomic(const Expression& expr) {
        IsAtomic is_atomic;
        expr.accept(&is_atomic);
        return is_atomic.is_atomic();
    }

    bool takes_precedence(const Expression& expr, BinaryOpExpression::BinaryOp op, bool unordered_junctions) {
        TakesPrecedenceBinary takes_precedence_binary(op, unordered_junctions);
        expr.accept(&takes_precedence_binary);
        return takes_precedence_binary.takes_precedence();
    }

    bool takes_precedence(const Expression& expr, UnaryOpExpression::UnaryOp) { return is_atomic(expr); }

}