//
// This file is part of the PlaJA code base.
// Copyright (c) (2019 - 2019) Marcel Vinzent.
// See README.md in the top-level directory for licensing information.
//

#include "array_type.h"
#include "../../../utils/utils.h"
#include "../../visitor/ast_visitor.h"
#include "../../visitor/ast_visitor_const.h"

ArrayType::ArrayType() = default;

ArrayType::~ArrayType() = default;

/* Override. */

void ArrayType::accept(AstVisitor* ast_visitor) { ast_visitor->visit(this); }

void ArrayType::accept(AstVisitorConst* ast_visitor) const { ast_visitor->visit(this); }

bool ArrayType::is_array_type() const { return true; }

bool ArrayType::has_boolean_base() const { return elementType->has_boolean_base(); }

bool ArrayType::has_integer_base() const { return elementType->has_integer_base(); }

bool ArrayType::has_floating_base() const { return elementType->has_floating_base(); }

DeclarationType::TypeKind ArrayType::get_kind() const { return TypeKind::Array; }

bool ArrayType::is_assignable_from(const DeclarationType& assigned_type) const {
    if (TypeKind::Array == assigned_type.get_kind()) {
        JANI_ASSERT(elementType and PLAJA_UTILS::cast_ref<ArrayType>(assigned_type).get_element_type())
        return elementType->is_assignable_from(*PLAJA_UTILS::cast_ref<ArrayType>(assigned_type).get_element_type());
    } else { return false; }
}

/*
bool ArrayType::operator==(const DeclarationType& ref_type) const {
    if (TypeKind::Array == ref_type.get_kind()) {
        JANI_ASSERT(elementType and PLAJA_UTILS::cast_ref<ArrayType>(ref_type).elementType)
        return (*elementType) == (*PLAJA_UTILS::cast_ref<ArrayType>(ref_type).elementType);
    } else { return false; }
}
*/

std::unique_ptr<DeclarationType> ArrayType::infer_common(const DeclarationType& ref_type) const {
    if (TypeKind::Array == ref_type.get_kind()) {

        JANI_ASSERT(elementType and PLAJA_UTILS::cast_ref<ArrayType>(ref_type).get_element_type())

        std::unique_ptr<DeclarationType> common_element_type = elementType->infer_common(*PLAJA_UTILS::cast_ref<ArrayType>(ref_type).get_element_type());

        if (common_element_type) { // not nullptr
            std::unique_ptr<ArrayType> inferred_type(new ArrayType());
            inferred_type->set_element_type(std::move(common_element_type));
            return inferred_type;
        } else { return nullptr; }

    } else { return nullptr; }
}

std::unique_ptr<DeclarationType> ArrayType::deep_copy_decl_type() const {
    std::unique_ptr<ArrayType> copy(new ArrayType());
    if (elementType) { copy->set_element_type(elementType->deep_copy_decl_type()); }
    return copy;
}
