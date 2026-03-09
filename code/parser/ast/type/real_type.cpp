//
// This file is part of the PlaJA code base.
// Copyright (c) (2019 - 2019) Marcel Vinzent.
// See README.md in the top-level directory for licensing information.
//

#include "real_type.h"
#include "../../../exception/plaja_exception.h"
#include "../../../utils/utils.h"
#include "../../visitor/ast_visitor.h"
#include "../../visitor/ast_visitor_const.h"
#include "bounded_type.h"

RealType::RealType() = default;

RealType::~RealType() = default;

bool RealType::assignable_from(const DeclarationType& assigned_type) { return assigned_type.is_numeric_type(); }

// override:

void RealType::accept(AstVisitor* ast_visitor) { ast_visitor->visit(this); }

void RealType::accept(AstVisitorConst* ast_visitor) const { ast_visitor->visit(this); }

bool RealType::is_numeric_type() const { return true; }

bool RealType::is_floating_type() const { return true; }

bool RealType::is_assignable_from(const DeclarationType& assigned_type) const { return assignable_from(assigned_type); }

DeclarationType::TypeKind RealType::get_kind() const { return TypeKind::Real; }

std::unique_ptr<DeclarationType> RealType::infer_common(const DeclarationType& ref_type) const { // NOLINT(misc-no-recursion)
    switch (ref_type.get_kind()) {
        case TypeKind::Int:
        case TypeKind::Real:
        case TypeKind::Clock:
        case TypeKind::Continuous: { return std::make_unique<RealType>(); }
        case TypeKind::Bool:
        case TypeKind::Array:
        case TypeKind::Location: { return nullptr; }
        case TypeKind::Bounded: { return this->infer_common(*PLAJA_UTILS::cast_ref<BoundedType>(ref_type).get_base()); } // NOLINT(cppcoreguidelines-pro-type-static-cast-downcast)
        default: { throw PlajaException(__PRETTY_FUNCTION__); }
    }
}

std::unique_ptr<BasicType> RealType::deep_copy_basic_type() const { return deep_copy(); }

