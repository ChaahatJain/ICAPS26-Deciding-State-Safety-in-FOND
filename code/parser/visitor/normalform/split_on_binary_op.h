//
// This file is part of the PlaJA code base.
// Copyright (c) (2019 - 2023) Marcel Vinzent.
// See README.md in the top-level directory for licensing information.
//

#ifndef PLAJA_SPLIT_ON_BINARY_OP_H
#define PLAJA_SPLIT_ON_BINARY_OP_H

#include <list>
#include <memory>
#include "../../ast/expression/binary_op_expression.h"
#include "../ast_visitor.h"

class SplitOnBinaryOpBase: protected AstVisitor {

private:
    BinaryOpExpression::BinaryOp op;

protected:
    [[nodiscard]] BinaryOpExpression::BinaryOp get_op() const { return op; }

    [[nodiscard]] bool is_split(const Expression& expr) const;

    static bool is_split(const Expression& expr, BinaryOpExpression::BinaryOp op);

    explicit SplitOnBinaryOpBase(BinaryOpExpression::BinaryOp op);
public:
    ~SplitOnBinaryOpBase() override = 0;
    DELETE_CONSTRUCTOR(SplitOnBinaryOpBase)
};

/**********************************************************************************************************************/

class SplitOnBinaryOp final: private SplitOnBinaryOpBase {

private:
    std::list<std::unique_ptr<Expression>> splitExpressions;

    void visit(BinaryOpExpression* exp) override;
    void visit(LetExpression* exp) override;
    void visit(StateConditionExpression* exp) override;
    void visit(NaryExpression* exp) override;

    explicit SplitOnBinaryOp(BinaryOpExpression::BinaryOp op);
public:
    ~SplitOnBinaryOp() override;
    DELETE_CONSTRUCTOR(SplitOnBinaryOp)

    static std::list<std::unique_ptr<Expression>> split(std::unique_ptr<Expression>&& expr, BinaryOpExpression::BinaryOp op);
    static std::list<std::unique_ptr<Expression>> split(const Expression& expr, BinaryOpExpression::BinaryOp op);

    static std::unique_ptr<Expression> to_expr(std::list<std::unique_ptr<Expression>>&& splits, BinaryOpExpression::BinaryOp op);
    static std::unique_ptr<Expression> to_expr(std::unique_ptr<Expression>&& expr, BinaryOpExpression::BinaryOp op);

};

/**********************************************************************************************************************/

class ToNary final: private SplitOnBinaryOpBase {

private:
    bool recursive;

    void visit(BinaryOpExpression* exp) override;
    void visit(LetExpression* exp) override;
    void visit(StateConditionExpression* exp) override;
    void visit(NaryExpression* exp) override;

    void process_split(Expression* expr);

    explicit ToNary(BinaryOpExpression::BinaryOp op, bool recursive);
public:
    ~ToNary() override;
    DELETE_CONSTRUCTOR(ToNary)

    static void to_nary(std::unique_ptr<Expression>& expr, BinaryOpExpression::BinaryOp op, bool recursive);

    /* Special structures to facilitate usage. */
    // static void to_nary(std::unique_ptr<AstElement>& ast_element, BinaryOpExpression::BinaryOp op, bool recursive);
    // static void to_nary(Model& model);

};

/**********************************************************************************************************************/

class ConstructBinaryOp final: private AstVisitor {

private:
    BinaryOpExpression::BinaryOp op;
    // std::unique_ptr<Expression> left;
    // std::unique_ptr<Expression> right;

    // void visit(StateConditionExpression* exp) override;

    std::unique_ptr<Expression> merge(std::unique_ptr<Expression>&& left, std::unique_ptr<Expression>&& right);

    static std::unique_ptr<Expression> construct(BinaryOpExpression::BinaryOp op, std::list<std::unique_ptr<Expression>>& sub_expressions);

    explicit ConstructBinaryOp(BinaryOpExpression::BinaryOp op);
public:
    ~ConstructBinaryOp() override;
    DELETE_CONSTRUCTOR(ConstructBinaryOp)

    static std::unique_ptr<Expression> construct(BinaryOpExpression::BinaryOp op, std::list<std::unique_ptr<Expression>>&& sub_expressions);

};

#endif //PLAJA_SPLIT_ON_BINARY_OP_H
