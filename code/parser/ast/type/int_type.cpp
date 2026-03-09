//
// This file is part of the PlaJA code base.
// Copyright (c) (2019 - 2019) Marcel Vinzent.
// See README.md in the top-level directory for licensing information.
//

#include "int_type.h"
#include "../../../exception/plaja_exception.h"
#include "../../../utils/utils.h"
#include "../../visitor/ast_visitor.h"
#include "../../visitor/ast_visitor_const.h"
#include "real_type.h"
#include "bounded_type.h"

IntType::IntType() = default;

IntType::~IntType() = default;

bool IntType::assignable_from(const DeclarationType& assigned_type) {
    switch (assigned_type.get_kind()) {
        case TypeKind::Int: { return true; }
        case TypeKind::Bounded: { return TypeKind::Int == PLAJA_UTILS::cast_ref<BoundedType>(assigned_type).get_base()->get_kind(); }
            // no recursion on base type as JANI specification is that concrete (Int or Bounded Int)
        default: { return false; }
    }
}

// override:

bool IntType::is_numeric_type() const { return true; }

bool IntType::is_integer_type() const { return true; }

DeclarationType::TypeKind IntType::get_kind() const { return TypeKind::Int; }

bool IntType::is_assignable_from(const DeclarationType& assigned_type) const { return assignable_from(assigned_type); }

std::unique_ptr<DeclarationType> IntType::infer_common(const DeclarationType& ref_type) const { // NOLINT(misc-no-recursion)
    switch (ref_type.get_kind()) {
        case TypeKind::Int: { return std::make_unique<IntType>(); }
        case TypeKind::Real:
        case TypeKind::Clock:
        case TypeKind::Continuous: { return std::make_unique<RealType>(); }
        case TypeKind::Bool:
        case TypeKind::Array:
        case TypeKind::Location: { return nullptr; }
        case TypeKind::Bounded: { return this->infer_common(*PLAJA_UTILS::cast_ref<BoundedType>(ref_type).get_base()); }
        default: throw PlajaException("IntType::infer_common");
    }
}

std::unique_ptr<BasicType> IntType::deep_copy_basic_type() const { return deep_copy(); }

void IntType::accept(AstVisitor* ast_visitor) { ast_visitor->visit(this); }

void IntType::accept(AstVisitorConst* ast_visitor) const { ast_visitor->visit(this); }
