//
// This file is part of the PlaJA code base.
// Copyright (c) (2019 - 2021) Marcel Vinzent.
// See README.md in the top-level directory for licensing information.
//

#include "free_variable_declaration.h"
#include "../../visitor/ast_visitor.h"
#include "../../visitor/ast_visitor_const.h"
#include "../type/declaration_type.h"

FreeVariableDeclaration::FreeVariableDeclaration(FreeVariableID_type var_id):
    name(PLAJA_UTILS::emptyString)
    , declarationType(nullptr)
    , varID(var_id) {
}

FreeVariableDeclaration::~FreeVariableDeclaration() = default;

void FreeVariableDeclaration::set_type(std::unique_ptr<DeclarationType>&& decl_type) { declarationType = std::move(decl_type); }

std::unique_ptr<FreeVariableDeclaration> FreeVariableDeclaration::deepCopy() const {
    std::unique_ptr<FreeVariableDeclaration> copy(new FreeVariableDeclaration(varID));
    copy->copy_comment(*this);
    copy->set_name(name);
    if (declarationType) { copy->set_type(declarationType->deep_copy_decl_type()); }
    return copy;
}

void FreeVariableDeclaration::accept(AstVisitor* ast_visitor) { ast_visitor->visit(this); }

void FreeVariableDeclaration::accept(AstVisitorConst* ast_visitor) const { ast_visitor->visit(this); }
