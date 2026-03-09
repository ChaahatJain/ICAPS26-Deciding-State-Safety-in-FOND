//
// This file is part of the PlaJA code base.
// Copyright (c) (2019 - 2019) Marcel Vinzent.
// See README.md in the top-level directory for licensing information.
//

#include "constant_declaration.h"
#include "../visitor/ast_visitor.h"
#include "../visitor/ast_visitor_const.h"
#include "type/declaration_type.h"
#include "expression/expression.h"

ConstantDeclaration::ConstantDeclaration(ConstantIdType id):
    name(PLAJA_UTILS::emptyString)
    , id(id)
    , declarationType()
    , value() {
}

ConstantDeclaration::~ConstantDeclaration() = default;

/* Setter .*/

void ConstantDeclaration::set_declaration_type(std::unique_ptr<DeclarationType>&& decl_type) { declarationType = std::move(decl_type); }

void ConstantDeclaration::set_value(std::unique_ptr<Expression>&& val) { value = std::move(val); }

/* */

std::unique_ptr<ConstantDeclaration> ConstantDeclaration::deep_copy() const {
    std::unique_ptr<ConstantDeclaration> copy(new ConstantDeclaration(id));
    copy->copy_comment(*this);
    copy->set_name(name);
    // copy->id = id;
    if (declarationType) { copy->set_declaration_type(declarationType->deep_copy_decl_type()); }
    if (value) { copy->set_value(value->deepCopy_Exp()); }
    return copy;
}

/* Override. */

void ConstantDeclaration::accept(AstVisitor* ast_visitor) { ast_visitor->visit(this); }

void ConstantDeclaration::accept(AstVisitorConst* ast_visitor) const { ast_visitor->visit(this); }





