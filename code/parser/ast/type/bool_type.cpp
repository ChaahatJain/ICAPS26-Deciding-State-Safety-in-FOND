//
// This file is part of the PlaJA code base.
// Copyright (c) (2019 - 2019) Marcel Vinzent.
// See README.md in the top-level directory for licensing information.
//

#include "bool_type.h"
#include "../../visitor/ast_visitor.h"
#include "../../visitor/ast_visitor_const.h"

BoolType::BoolType() = default;

BoolType::~BoolType() = default;

bool BoolType::assignable_from(const DeclarationType& assigned_type) { return TypeKind::Bool == assigned_type.get_kind(); }

/* Override. */

bool BoolType::is_boolean_type() const { return true; }

DeclarationType::TypeKind BoolType::get_kind() const { return TypeKind::Bool; }

bool BoolType::is_assignable_from(const DeclarationType& assigned_type) const { return assignable_from(assigned_type); }

std::unique_ptr<DeclarationType> BoolType::infer_common(const DeclarationType& ref_type) const {
    return TypeKind::Bool == ref_type.get_kind() ? std::make_unique<BoolType>() : nullptr;
}

std::unique_ptr<BasicType> BoolType::deep_copy_basic_type() const { return deep_copy(); }

void BoolType::accept(AstVisitor* ast_visitor) { ast_visitor->visit(this); }

void BoolType::accept(AstVisitorConst* ast_visitor) const { ast_visitor->visit(this); }
