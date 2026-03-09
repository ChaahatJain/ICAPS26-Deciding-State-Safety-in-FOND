//
// This file is part of the PlaJA code base.
// Copyright (c) (2019 - 2019) Marcel Vinzent.
// See README.md in the top-level directory for licensing information.
//

#include "continuous_type.h"
#include "../../visitor/ast_visitor.h"
#include "../../visitor/ast_visitor_const.h"

ContinuousType::ContinuousType() = default;

ContinuousType::~ContinuousType() = default;

/* Override. */

bool ContinuousType::is_numeric_type() const { return true; }

bool ContinuousType::is_floating_type() const { return true; }

bool ContinuousType::is_trivial_type() const { return true; }

DeclarationType::TypeKind ContinuousType::get_kind() const { return TypeKind::Continuous; }

std::unique_ptr<DeclarationType> ContinuousType::deep_copy_decl_type() const { return deep_copy(); }

void ContinuousType::accept(AstVisitor* ast_visitor) { ast_visitor->visit(this); }

void ContinuousType::accept(AstVisitorConst* ast_visitor) const { ast_visitor->visit(this); }
