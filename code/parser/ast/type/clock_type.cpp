//
// This file is part of the PlaJA code base.
// Copyright (c) (2019 - 2019) Marcel Vinzent.
// See README.md in the top-level directory for licensing information.
//

#include "clock_type.h"
#include "../../visitor/ast_visitor.h"
#include "../../visitor/ast_visitor_const.h"

ClockType::ClockType() = default;

ClockType::~ClockType() = default;

/* Override. */

bool ClockType::is_numeric_type() const { return true; }

bool ClockType::is_floating_type() const { return true; }

bool ClockType::is_trivial_type() const { return true; }

DeclarationType::TypeKind ClockType::get_kind() const { return TypeKind::Clock; }

std::unique_ptr<DeclarationType> ClockType::deep_copy_decl_type() const { return deep_copy(); }

void ClockType::accept(AstVisitor* ast_visitor) { ast_visitor->visit(this); }

void ClockType::accept(AstVisitorConst* ast_visitor) const { ast_visitor->visit(this); }

