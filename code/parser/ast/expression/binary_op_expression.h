//
// This file is part of the PlaJA code base.
// Copyright (c) (2019 - 2019) Marcel Vinzent.
// See README.md in the top-level directory for licensing information.
//

#ifndef PLAJA_BINARY_OP_EXPRESSION_H
#define PLAJA_BINARY_OP_EXPRESSION_H

#include <memory>
#include "expression.h"

class BinaryOpExpression final: public Expression {
public:
    enum BinaryOp { OR, AND, EQ, NE, LT, LE, PLUS, MINUS, TIMES, MOD, DIV, POW, LOG, IMPLIES, GT, GE, MIN, MAX };

private:
    BinaryOp op;
    std::unique_ptr<Expression> left;
    std::unique_ptr<Expression> right;

public:
    explicit BinaryOpExpression(BinaryOp binary_op);
    ~BinaryOpExpression() override;
    DELETE_CONSTRUCTOR(BinaryOpExpression)

    /* Static. */
    static const std::string& binary_op_to_str(BinaryOp op);
    static std::unique_ptr<BinaryOp> str_to_binary_op(const std::string& op_str);
    static std::unique_ptr<Expression> construct_bound(std::unique_ptr<Expression>&& var, std::unique_ptr<Expression>&& value, BinaryOp op);
    static std::unique_ptr<Expression> construct_bound(std::unique_ptr<Expression>&& var, PLAJA::integer value, BinaryOp op);

    /* Setter. */

    [[maybe_unused]] inline void set_op(BinaryOp binary_op) { op = binary_op; }

    inline std::unique_ptr<Expression> set_left(std::unique_ptr<Expression>&& left_ = nullptr) {
        std::swap(left, left_);
        return std::move(left_);
    }

    inline std::unique_ptr<Expression> set_right(std::unique_ptr<Expression>&& right_ = nullptr) {
        std::swap(right, right_);
        return std::move(right_);
    }

    /* Getter. */

    [[nodiscard]] inline BinaryOp get_op() const { return op; }

    inline Expression* get_left() { return left.get(); }

    [[nodiscard]] inline const Expression* get_left() const { return left.get(); }

    inline Expression* get_right() { return right.get(); }

    [[nodiscard]] inline const Expression* get_right() const { return right.get(); }

    /* Override. */
    PLAJA::integer evaluateInteger(const StateBase* state) const override;
    PLAJA::floating evaluateFloating(const StateBase* state) const override;
    [[nodiscard]] bool is_constant() const override;
    bool equals(const Expression* exp) const override;
    [[nodiscard]] std::size_t hash() const override;
    const DeclarationType* determine_type() override;
    void accept(AstVisitor* ast_visitor) override;
    void accept(AstVisitorConst* ast_visitor) const override;
    [[nodiscard]] std::unique_ptr<Expression> deepCopy_Exp() const override;
    [[nodiscard]] std::unique_ptr<Expression> move_exp() override;

    /* */
    [[nodiscard]] std::unique_ptr<BinaryOpExpression> deepCopy() const;
    [[nodiscard]] std::unique_ptr<BinaryOpExpression> move();
};

#endif //PLAJA_BINARY_OP_EXPRESSION_H
