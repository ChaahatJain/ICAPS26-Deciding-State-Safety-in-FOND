//
// This file is part of the PlaJA code base.
// Copyright (c) (2019 - 2024) Marcel Vinzent.
// See README.md in the top-level directory for licensing information.
//

#include "constant_array_access_expression.h"
#include "../../../../utils/utils.h"
#include "../../../visitor/ast_visitor.h"
#include "../../../visitor/ast_visitor_const.h"
#include "../../type/array_type.h"
#include "../array_value_expression.h"
#include "../../constant_declaration.h"

ConstantArrayAccessExpression::ConstantArrayAccessExpression(const ConstantDeclaration* accessed_array):
    accessedArray(accessed_array) {
}

ConstantArrayAccessExpression::~ConstantArrayAccessExpression() = default;

/* Short-cut. */

const std::string& ConstantArrayAccessExpression::get_name() const { return accessedArray->get_name(); }

ConstantIdType ConstantArrayAccessExpression::get_id() const { return accessedArray->get_id(); }

const Expression* ConstantArrayAccessExpression::get_array_value() const { return accessedArray->get_value(); }

std::size_t ConstantArrayAccessExpression::get_array_size() const {
    return PLAJA_UTILS::cast_ptr<ArrayValueExpression>(accessedArray->get_value())->get_number_elements();
}

/* */

PLAJA::integer ConstantArrayAccessExpression::evaluateInteger(const StateBase* state) const {
    PLAJA_ASSERT(accessedArray)
    const auto* aa_value = PLAJA_UTILS::cast_ptr<ArrayValueExpression>(accessedArray->get_value());
    PLAJA_ASSERT(aa_value)
    const auto* aa_elem_value = aa_value->get_element(index->evaluateInteger(state));
    PLAJA_ASSERT(aa_elem_value)
    return aa_elem_value->evaluateInteger(state);
}

PLAJA::floating ConstantArrayAccessExpression::evaluateFloating(const StateBase* state) const {
    PLAJA_ASSERT(accessedArray)
    const auto* aa_value = PLAJA_UTILS::cast_ptr<ArrayValueExpression>(accessedArray->get_value());
    PLAJA_ASSERT(aa_value)
    const auto* aa_elem_value = aa_value->get_element(index->evaluateInteger(state));
    PLAJA_ASSERT(aa_elem_value)
    return aa_elem_value->evaluateFloating(state);
}

bool ConstantArrayAccessExpression::is_constant() const { return index->is_constant(); }

bool ConstantArrayAccessExpression::equals(const Expression* exp) const {
    auto* other = PLAJA_UTILS::cast_ptr_if<ConstantArrayAccessExpression>(exp);
    if (not other) { return false; }

    PLAJA_ASSERT(accessedArray and index)
    JANI_ASSERT(other->accessedArray and other->index)

    return get_accessed_array() == other->get_accessed_array() and get_index()->equals(other->get_index());
}

std::size_t ConstantArrayAccessExpression::hash() const {
    JANI_ASSERT(accessedArray and index)
    constexpr unsigned prime = 31;
    std::size_t result = 1;
    result = prime * result + (PLAJA_UTILS::hashString(accessedArray->get_name()));
    result = prime * result + (index->hash());
    return result;
}

const DeclarationType* ConstantArrayAccessExpression::determine_type() {
    if (resultType) { return resultType.get(); }
    else {
        JANI_ASSERT(accessedArray)
        auto* actual_type = PLAJA_UTILS::cast_ptr<ArrayType>(accessedArray->get_value()->get_type());
        resultType = actual_type->get_element_type()->deep_copy_decl_type();
        return resultType.get();
    }
}

void ConstantArrayAccessExpression::accept(AstVisitor* ast_visitor) { ast_visitor->visit(this); }

void ConstantArrayAccessExpression::accept(AstVisitorConst* ast_visitor) const { ast_visitor->visit(this); }

std::unique_ptr<Expression> ConstantArrayAccessExpression::deepCopy_Exp() const { return deep_copy(); }

std::unique_ptr<Expression> ConstantArrayAccessExpression::move_exp() { return move(); }

/* */

std::unique_ptr<ConstantArrayAccessExpression> ConstantArrayAccessExpression::deep_copy() const {
    std::unique_ptr<ConstantArrayAccessExpression> copy(new ConstantArrayAccessExpression(accessedArray));
    if (resultType) { copy->resultType = resultType->deep_copy_decl_type(); }
    if (index) { copy->set_index(index->deepCopy_Exp()); }
    return copy;
}

std::unique_ptr<ConstantArrayAccessExpression> ConstantArrayAccessExpression::move() {
    std::unique_ptr<ConstantArrayAccessExpression> copy(new ConstantArrayAccessExpression(accessedArray));
    if (resultType) { copy->resultType = std::move(resultType); }
    if (index) { copy->set_index(std::move(index)); }
    return copy;
}