//
// This file is part of the PlaJA code base.
// Copyright (c) (2019 - 2019) Marcel Vinzent.
// See README.md in the top-level directory for licensing information.
//

#include "derivative_expression.h"
#include "../../../exception/not_implemented_exception.h"
#include "../../visitor/ast_visitor.h"
#include "../../visitor/ast_visitor_const.h"
#include "../type/real_type.h"

DerivativeExpression::DerivativeExpression(const VariableDeclaration& decl):
    var(decl) {
    resultType = std::make_unique<RealType>();
}

DerivativeExpression::~DerivativeExpression() = default;

/* Override. */

bool DerivativeExpression::is_constant() const { return false; }

bool DerivativeExpression::is_proposition() const { throw NotImplementedException(__PRETTY_FUNCTION__); }

const DeclarationType* DerivativeExpression::determine_type() {
    if (resultType) { return resultType.get(); }
    else {
        resultType = std::make_unique<RealType>();
        return resultType.get();
    }
}

void DerivativeExpression::accept(AstVisitor* ast_visitor) { ast_visitor->visit(this); }

void DerivativeExpression::accept(AstVisitorConst* ast_visitor) const { ast_visitor->visit(this); }

std::unique_ptr<Expression> DerivativeExpression::deepCopy_Exp() const {
    PLAJA_ASSERT(var.get_decl())
    return std::make_unique<DerivativeExpression>(*var.get_decl());
}
