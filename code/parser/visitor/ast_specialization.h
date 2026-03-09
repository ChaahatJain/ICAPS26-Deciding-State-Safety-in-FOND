//
// This file is part of the PlaJA code base.
// Copyright (c) (2019 - 2022) Marcel Vinzent.
// See README.md in the top-level directory for licensing information.
//

#ifndef PLAJA_AST_SPECIALIZATION_H
#define PLAJA_AST_SPECIALIZATION_H

#include "ast_visitor.h"

class AstSpecialization final: public AstVisitor {

private:
    bool doToNary; // Call to_nary once top-down.

    void visit(BinaryOpExpression* expr) override;
    // void visit(ITE_Expression* expr) override;
    void visit(UnaryOpExpression* expr) override;
    void visit(NaryExpression* expr) override;

    AstSpecialization();
public:
    ~AstSpecialization() override;

    static void specialize_ast(std::unique_ptr<AstElement>& ast_elem);
    static void specialize_ast(Model& ast_elem); // special support for model, which should not be replaced
    DELETE_CONSTRUCTOR(AstSpecialization)

};

namespace AST_SPECIALIZATION {
    extern void specialize_ast(std::unique_ptr<AstElement>& ast_elem);
    extern std::unique_ptr<AstElement> specialize_ast(const AstElement& ast_elem);
    extern void specialize_ast(Model& model);
}

#endif //PLAJA_AST_SPECIALIZATION_H
