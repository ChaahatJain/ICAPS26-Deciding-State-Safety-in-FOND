//
// This file is part of the PlaJA code base.
// Copyright (c) (2019 - 2022) Marcel Vinzent.
// See README.md in the top-level directory for licensing information.
//

#ifndef PLAJA_AST_STANDARDIZATION_H
#define PLAJA_AST_STANDARDIZATION_H

#include "ast_visitor.h"

class AstStandardization final: public AstVisitor {

private:
    void visit(LinearExpression* expr) override;
    void visit(NaryExpression* expr) override;

    AstStandardization();
public:
    ~AstStandardization() override;
    DELETE_CONSTRUCTOR(AstStandardization)

    static void standardize_ast(std::unique_ptr<AstElement>& ast_elem);
    static void standardize_ast(Model& ast_elem); // Special support for model, which should not be replaced.

};

namespace AST_STANDARDIZATION {
    extern void standardize_ast(std::unique_ptr<AstElement>& ast_elem);
    extern std::unique_ptr<AstElement> standardize_ast(const AstElement& ast_elem);
    extern void standardize_ast(Model& model);
}

#endif //PLAJA_AST_STANDARDIZATION_H
