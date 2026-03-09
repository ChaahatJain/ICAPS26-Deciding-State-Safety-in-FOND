//
// This file is part of the PlaJA code base.
// Copyright (c) (2019 - 2019) Marcel Vinzent.
// See README.md in the top-level directory for licensing information.
//

#include "bounded_type.h"
#include "../../../utils/utils.h"
#include "../../visitor/ast_visitor.h"
#include "../../visitor/ast_visitor_const.h"
#include "../expression/expression.h"
#include "basic_type.h"
#include "int_type.h"
#include "real_type.h"

BoundedType::BoundedType() = default;

BoundedType::~BoundedType() = default;

/* Setter. */

void BoundedType::set_base(std::unique_ptr<BasicType>&& base_) {
    JANI_ASSERT(base_->get_kind() == TypeKind::Int or base_->get_kind() == TypeKind::Real)
    base = std::move(base_);
}

void BoundedType::set_lower_bound(std::unique_ptr<Expression>&& lower_bound) { lowerBound = std::move(lower_bound); }

void BoundedType::set_upper_bound(std::unique_ptr<Expression>&& upper_bound) { upperBound = std::move(upper_bound); }

/* Override. */

bool BoundedType::is_integer_type() const {
    JANI_ASSERT(base)
    return base->is_integer_type();
}

bool BoundedType::is_floating_type() const {
    JANI_ASSERT(base)
    return base->is_floating_type();
}

bool BoundedType::is_numeric_type() const {
    JANI_ASSERT(base)
    return base->is_numeric_type();
}

bool BoundedType::is_trivial_type() const {
    JANI_ASSERT(base)
    return base->is_trivial_type();
}

bool BoundedType::is_bounded_type() const { return true; }

DeclarationType::TypeKind BoundedType::get_kind() const { return TypeKind::Bounded; }

bool BoundedType::is_assignable_from(const DeclarationType& assigned_type) const {
    JANI_ASSERT(base)
    return base->is_assignable_from(assigned_type);
}

std::unique_ptr<DeclarationType> BoundedType::infer_common(const DeclarationType& ref_type) const {
    switch (ref_type.get_kind()) {
        case TypeKind::Array: { return nullptr; }
        case TypeKind::Bounded: {
            /********************************************************************************************/
            const auto& actual_type = PLAJA_UTILS::cast_ref<BoundedType>(ref_type);
            JANI_ASSERT(base && actual_type.base)
            switch (actual_type.base->get_kind()) {
                case TypeKind::Int: {
                    if (TypeKind::Int == base->get_kind()) { return std::make_unique<IntType>(); }
                    else {
                        PLAJA_ASSERT(TypeKind::Real == base->get_kind())
                        return std::make_unique<RealType>();
                    }
                }
                case TypeKind::Real: { return std::make_unique<RealType>(); }
                default: { PLAJA_ABORT }
            }
            PLAJA_ABORT
            /********************************************************************************************/
        }
        default: { return base->infer_common(ref_type); }
    }
}

/*
bool BoundedType::operator==(const DeclarationType& ref_type) const {
    const auto* bounded_type = PLAJA_UTILS::cast_ref_if<BoundedType>(ref_type);

    if (!bounded_type) { return false; }
    if (base->get_kind() != bounded_type->get_base()->get_kind()) { return false; }

    if (lowerBound) {
        if (bounded_type->lowerBound) {
            if (TypeKind::INT == base->get_kind()) {
                if (lowerBound->evaluateInteger(nullptr) != bounded_type->lowerBound->evaluateInteger(nullptr)) { return false; }
            }
            else if (lowerBound->evaluateFloating(nullptr) != bounded_type->lowerBound->evaluateFloating(nullptr)) { return false; }
        } else { return false; }
    } else if (bounded_type->lowerBound) { return false; }

    if (upperBound) {
        if (bounded_type->upperBound) {
            if (TypeKind::INT == base->get_kind()) {
                if (upperBound->evaluateInteger(nullptr) != bounded_type->upperBound->evaluateInteger(nullptr)) { return false; }
            }
            else if (upperBound->evaluateFloating(nullptr) != bounded_type->upperBound->evaluateFloating(nullptr)) { return false; }
        } else { return false; }
    } else if (bounded_type->upperBound) { return false; }

    return true;
}
*/

std::unique_ptr<DeclarationType> BoundedType::deep_copy_decl_type() const {
    std::unique_ptr<BoundedType> copy(new BoundedType());
    if (base) { copy->set_base(base->deep_copy_basic_type()); }
    if (lowerBound) { copy->set_lower_bound(lowerBound->deepCopy_Exp()); }
    if (upperBound) { copy->set_upper_bound(upperBound->deepCopy_Exp()); }
    return copy;
}

void BoundedType::accept(AstVisitor* ast_visitor) { ast_visitor->visit(this); }

void BoundedType::accept(AstVisitorConst* ast_visitor) const { ast_visitor->visit(this); }
