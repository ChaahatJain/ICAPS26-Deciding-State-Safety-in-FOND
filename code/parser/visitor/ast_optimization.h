//
// This file is part of the PlaJA code base.
// Copyright (c) (2019 - 2022) Marcel Vinzent.
// See README.md in the top-level directory for licensing information.
//

#ifndef PLAJA_AST_OPTIMIZATION_H
#define PLAJA_AST_OPTIMIZATION_H

#include "ast_visitor.h"

class AstOptimization final: public AstVisitor {

private:
    bool evaluate_constant(const Expression& expr);

    void visit(Automaton* automaton) override;
    void visit(Edge* edge) override;
    // expression:
    void visit(BinaryOpExpression* exp) override;
    void visit(ConstantExpression* exp) override;
    void visit(ITE_Expression* exp) override;
    void visit(UnaryOpExpression* exp) override;
    // non-standard:
    void visit(StateConditionExpression* exp) override;
    void visit(StateValuesExpression* exp) override;
    void visit(StatesValuesExpression* exp) override;

    AstOptimization();
public:
    ~AstOptimization() override;

    static void optimize_ast(std::unique_ptr<AstElement>& ast_elem);
    static void optimize_ast(Model& ast_elem); // special support for model, which should not be replaced

};

namespace AST_OPTIMIZATION {
    extern void optimize_ast(std::unique_ptr<AstElement>& ast_elem);
    extern void optimize_ast(std::unique_ptr<Expression>& expr);
    extern void optimize_ast(Model& model);
}

#endif //PLAJA_AST_OPTIMIZATION_H
