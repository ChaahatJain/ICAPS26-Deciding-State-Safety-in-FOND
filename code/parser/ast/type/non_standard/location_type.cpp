//
// This file is part of the PlaJA code base.
// Copyright (c) (2019 - 2023) Marcel Vinzent.
// See README.md in the top-level directory for licensing information.
//

#include "location_type.h"
#include "../../../visitor/ast_visitor.h"
#include "../../../visitor/ast_visitor_const.h"

LocationType::LocationType() = default;

LocationType::~LocationType() = default;

/* */

/* Override. */

LocationType::TypeKind LocationType::get_kind() const { return TypeKind::Location; }

bool LocationType::is_assignable_from(const DeclarationType& assigned_type) const { return assigned_type.get_kind() == TypeKind::Location; }

std::unique_ptr<DeclarationType> LocationType::infer_common(const DeclarationType& ref_type) const { return ref_type.get_kind() == TypeKind::Location ? deep_copy() : nullptr; }

std::unique_ptr<DeclarationType> LocationType::deep_copy_decl_type() const { return deep_copy(); }

void LocationType::accept(AstVisitor* ast_visitor) { ast_visitor->visit(this); }

void LocationType::accept(AstVisitorConst* ast_visitor) const { ast_visitor->visit(this); }
