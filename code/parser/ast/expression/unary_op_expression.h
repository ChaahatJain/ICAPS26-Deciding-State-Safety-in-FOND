//
// This file is part of the PlaJA code base.
// Copyright (c) (2019 - 2019) Marcel Vinzent.
// See README.md in the top-level directory for licensing information.
//

#ifndef PLAJA_UNARY_OP_EXPRESSION_H
#define PLAJA_UNARY_OP_EXPRESSION_H

#include "expression.h"

class UnaryOpExpression final: public Expression {

public:
    enum UnaryOp { NOT, FLOOR, CEIL, ABS, SGN, TRC };

private:
    UnaryOp op;
    std::unique_ptr<Expression> operand;

public:
    explicit UnaryOpExpression(UnaryOp unary_op);
    ~UnaryOpExpression() override;
    DELETE_CONSTRUCTOR(UnaryOpExpression)

    /* static */
    static const std::string& unary_op_to_str(UnaryOp op);
    static std::unique_ptr<UnaryOp> str_to_unary_op(const std::string& op_str);

    /* setter */

    [[maybe_unused]] inline void set_op(UnaryOp unary_op) { op = unary_op; }

    inline void set_operand(std::unique_ptr<Expression>&& operand_) { operand = std::move(operand_); }

    inline std::unique_ptr<Expression> release_operand() { return std::move(operand); }

    /* getter */

    [[nodiscard]] inline UnaryOp get_op() const { return op; }

    inline Expression* get_operand() { return operand.get(); }

    [[nodiscard]] inline const Expression* get_operand() const { return operand.get(); }

    /* override */
    PLAJA::integer evaluateInteger(const StateBase* state) const override;
    PLAJA::floating evaluateFloating(const StateBase* state) const override;
    [[nodiscard]] bool is_constant() const override;
    bool equals(const Expression* exp) const override;
    [[nodiscard]] std::size_t hash() const override;
    const DeclarationType* determine_type() override;
    void accept(AstVisitor* ast_visitor) override;
    void accept(AstVisitorConst* ast_visitor) const override;
    [[nodiscard]] std::unique_ptr<Expression> deepCopy_Exp() const override;
};

#endif //PLAJA_UNARY_OP_EXPRESSION_H
