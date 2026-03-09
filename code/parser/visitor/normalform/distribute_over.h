//
// This file is part of the PlaJA code base.
// Copyright (c) (2019 - 2023) Marcel Vinzent.
// See README.md in the top-level directory for licensing information.
//

#ifndef PLAJA_DISTRIBUTE_OVER_H
#define PLAJA_DISTRIBUTE_OVER_H

#include <memory>
#include "../../ast/expression/binary_op_expression.h"
#include "../ast_visitor.h"

class DistributeOver: public AstVisitor {

private:
    BinaryOpExpression::BinaryOp opToDistribute; // Distribute "or" over "and" : CNF (A && B) || D -> (A || D) && (B || D).
    BinaryOpExpression::BinaryOp opToSplit;

    std::unique_ptr<Expression> distribute(std::unique_ptr<Expression>&& expr_to_split, std::unique_ptr<Expression>&& expr_to_distribute);

    void distribute(NaryExpression& expr);

    void visit(BinaryOpExpression* exp) override;
    void visit(NaryExpression* exp) override;

    explicit DistributeOver(bool or_over_and);
public:
    ~DistributeOver() override;
    DELETE_CONSTRUCTOR(DistributeOver)

    static void distribute_or_over_and(std::unique_ptr<AstElement>& ast_elem);
    static void distribute_and_over_or(std::unique_ptr<AstElement>& ast_elem);

};

#endif //PLAJA_DISTRIBUTE_OVER_H
