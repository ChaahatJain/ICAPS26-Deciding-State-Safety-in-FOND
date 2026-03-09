//
// This file is part of the PlaJA code base.
// Copyright (c) (2019 - 2022) Marcel Vinzent.
// See README.md in the top-level directory for licensing information.
//

#include "ast_standardization.h"
#include "../ast/expression/special_cases/nary_expression.h"
#include "../ast/expression/special_cases/linear_expression.h"
#include "../ast/model.h"
#include "extern/visitor_base.h"
#include "extern/ast_specialization.h"
#include "normalform/split_on_binary_op.h"

AstStandardization::AstStandardization() = default;

AstStandardization::~AstStandardization() = default;

/**********************************************************************************************************************/

void AstStandardization::standardize_ast(std::unique_ptr<AstElement>& ast_elem) {
    AstStandardization ast_opt;
    ast_elem->accept(&ast_opt);
    if (ast_opt.replace_by_result) { ast_elem = ast_opt.move_result<AstElement>(); }
}

void AstStandardization::standardize_ast(Model& model) {
    AstStandardization ast_opt;
    model.accept(&ast_opt);
    PLAJA_ASSERT(not ast_opt.replace_by_result)
}

/**********************************************************************************************************************/

void AstStandardization::visit(LinearExpression* expr) { set_to_replace(expr->to_standard()); }

void AstStandardization::visit(NaryExpression* expr) {

    std::list<std::unique_ptr<Expression>> sub_expressions;

    for (auto it = expr->iterator(); !it.end(); ++it) {
        it->accept(this);
        if (replace_by_result) { sub_expressions.push_back(move_result<Expression>()); }
        else { sub_expressions.push_back(it.set(nullptr)); }
    }

    set_to_replace(ConstructBinaryOp::construct(expr->get_op(), std::move(sub_expressions)));
}

/**********************************************************************************************************************/

namespace AST_STANDARDIZATION {

    void standardize_ast(std::unique_ptr<AstElement>& ast_elem) {
        AST_SPECIALIZATION::specialize_ast(ast_elem); // We first specialize!
        AstStandardization::standardize_ast(ast_elem);
    }

    std::unique_ptr<AstElement> standardize_ast(const AstElement& ast_elem) {
        auto copy = ast_elem.deep_copy_ast();
        standardize_ast(copy);
        return copy;
    }

    void standardize_ast(Model& model) {
        AST_SPECIALIZATION::specialize_ast(model);
        AstStandardization::standardize_ast(model);
    }

}