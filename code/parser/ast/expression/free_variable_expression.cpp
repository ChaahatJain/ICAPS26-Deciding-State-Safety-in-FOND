//
// This file is part of the PlaJA code base.
// Copyright (c) (2019 - 2019) Marcel Vinzent.
// See README.md in the top-level directory for licensing information.
//

#include "free_variable_expression.h"
#include "../../visitor/ast_visitor.h"
#include "../../visitor/ast_visitor_const.h"
#include "../type/int_type.h"

FreeVariableExpression::FreeVariableExpression(std::string name_, FreeVariableID_type var_id, const DeclarationType* type):
    name(std::move(name_))
    , id(var_id) {
    PLAJA_ASSERT(type) // expectation: should not be nullptr; may be revised though
    if (type) { resultType = type->deep_copy_decl_type(); }
}

FreeVariableExpression::~FreeVariableExpression() = default;

// override:

const DeclarationType* FreeVariableExpression::determine_type() { return resultType.get(); }

void FreeVariableExpression::accept(AstVisitor* ast_visitor) { ast_visitor->visit(this); }

void FreeVariableExpression::accept(AstVisitorConst* ast_visitor) const { ast_visitor->visit(this); }

std::unique_ptr<Expression> FreeVariableExpression::deepCopy_Exp() const { return deepCopy(); }

std::unique_ptr<FreeVariableExpression> FreeVariableExpression::deepCopy() const {
    std::unique_ptr<FreeVariableExpression> copy(new FreeVariableExpression(name, id, resultType.get()));
    return copy;
}

