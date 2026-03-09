//
// This file is part of the PlaJA code base.
// Copyright (c) (2019 - 2019) Marcel Vinzent.
// See README.md in the top-level directory for licensing information.
//

#include "array_access_expression.h"
#include "../../../exception/runtime_exception.h"
#include "../../../search/states/state_base.h"
#include "../../visitor/ast_visitor.h"
#include "../../visitor/ast_visitor_const.h"
#include "../type/array_type.h"
#include "integer_value_expression.h"

#ifdef RUNTIME_CHECKS

#include "../../../search/information/model_information.h"
#include "../../../globals.h"
#include "../model.h"

#endif

ArrayAccessExpression::ArrayAccessExpression() = default;

ArrayAccessExpression::~ArrayAccessExpression() = default;

/* */

std::unique_ptr<VariableExpression> ArrayAccessExpression::set_accessedArray(std::unique_ptr<VariableExpression>&& accessed_array) {
    std::swap(accessedArray, accessed_array);
    return std::move(accessed_array);
}

std::unique_ptr<Expression> ArrayAccessExpression::set_index(std::unique_ptr<Expression>&& index_r) {
    std::swap(index, index_r);
    return std::move(index_r);
}

std::unique_ptr<VariableExpression> ArrayAccessExpression::set_accessedArray(const VariableDeclaration& var_decl) { return set_accessedArray(std::make_unique<VariableExpression>(var_decl)); }

std::unique_ptr<Expression> ArrayAccessExpression::set_index(PLAJA::integer index_v) { return set_index(std::make_unique<IntegerValueExpression>(index_v)); }

/* */

#ifdef RUNTIME_CHECKS

void ArrayAccessExpression::check_offset(PLAJA::integer offset) const {
    MAYBE_UNUSED(offset)
    RUNTIME_ASSERT(0 <= offset, "negative index")
    const VariableID_type var_id = accessedArray->get_id();
    RUNTIME_ASSERT(offset < PLAJA_GLOBAL::currentModel->get_model_information().get_array_size(var_id), "assignment out of array bounds")
}

#else

inline void ArrayAccessExpression::check_offset(PLAJA::integer /*offset*/) const {}

#endif

/* Override. */

void ArrayAccessExpression::assign(StateBase& target, const StateBase* source, const Expression* value) const {
    PLAJA_ASSERT(not accessedArray->is_constant()) // to identify error in case we use, e.g., array values beyond initialization (currently ruled out by fixing VariableExpression)
    const PLAJA::integer offset = index->evaluateInteger(source);

    check_offset(offset); // Check index within bounds.

    if (resultType->is_integer_type()) { target.assign_int<true>(accessedArray->get_index() + offset, value->evaluateInteger(source)); }
    else {
        PLAJA_ASSERT(resultType->is_floating_type())
        target.assign_float<true>(accessedArray->get_index() + offset, value->evaluateFloating(source));
    }

    PLAJA_ASSERT(target.is_valid())
}

PLAJA::integer ArrayAccessExpression::access_integer(const StateBase& target, const StateBase* source) const {
    const PLAJA::integer offset = index->evaluateInteger(source);
    check_offset(offset); // Check index within bounds.
    PLAJA_ASSERT(resultType->has_integer_base() or resultType->has_boolean_base())
    return target.get_int(accessedArray->get_index() + offset);
}

PLAJA::floating ArrayAccessExpression::access_floating(const StateBase& target, const StateBase* source) const {
    const PLAJA::integer offset = index->evaluateInteger(source);
    check_offset(offset); // Check index within bounds.
    return target.get_float(accessedArray->get_index() + offset);
}

VariableID_type ArrayAccessExpression::get_variable_id() const { return accessedArray->get_id(); }

const Expression* ArrayAccessExpression::get_array_index() const { return index.get(); }

VariableIndex_type ArrayAccessExpression::get_variable_index(const StateBase* state) const {
    PLAJA_ASSERT(state or index->is_constant())
    const PLAJA::integer offset = index->evaluateInteger(state);
    STMT_IF_RUNTIME_CHECKS(if (state) { check_offset(offset); }) // Check index within bounds -- only if "state" to quick-fix usage during some tests.
    return accessedArray->get_index() + offset;
}

PLAJA::integer ArrayAccessExpression::evaluateInteger(const StateBase* state) const {
    const PLAJA::integer offset = index->evaluateInteger(state);
    check_offset(offset); // Check index within bounds.
    PLAJA_ASSERT(resultType->has_integer_base() or resultType->has_boolean_base())
    return state->get_int(accessedArray->get_index() + offset);
}

PLAJA::floating ArrayAccessExpression::evaluateFloating(const StateBase* state) const {
    const PLAJA::integer offset = index->evaluateInteger(state);
    check_offset(offset); // Check index within bounds.
    return state->operator[](accessedArray->get_index() + offset);
}

bool ArrayAccessExpression::is_constant() const {
    JANI_ASSERT(accessedArray)
    JANI_ASSERT(index)
    return accessedArray->is_constant() and index->is_constant();
}

bool ArrayAccessExpression::equals(const Expression* exp) const {
    JANI_ASSERT(accessedArray and index)
    auto* other = PLAJA_UTILS::cast_ptr_if<ArrayAccessExpression>(exp);
    if (not other) { return false; }

    JANI_ASSERT(other->accessedArray and other->index)
    return this->accessedArray->equals(other->accessedArray.get()) and this->index->equals(other->index.get());
}

std::size_t ArrayAccessExpression::hash() const {
    JANI_ASSERT(accessedArray and index)
    constexpr unsigned prime = 31;
    std::size_t result = 1;
    result = prime * result + (accessedArray->hash());
    result = prime * result + (index->hash());
    return result;
}

const DeclarationType* ArrayAccessExpression::determine_type() {
    if (resultType) { return resultType.get(); }
    else {
        JANI_ASSERT(accessedArray)
        auto* actual_type = PLAJA_UTILS::cast_ptr<ArrayType>(accessedArray->determine_type());
        resultType = actual_type->get_element_type()->deep_copy_decl_type();
        return resultType.get();
    }
}

void ArrayAccessExpression::accept(AstVisitor* ast_visitor) { ast_visitor->visit(this); }

void ArrayAccessExpression::accept(AstVisitorConst* ast_visitor) const { ast_visitor->visit(this); }

std::unique_ptr<LValueExpression> ArrayAccessExpression::deep_copy_lval_exp() const { return deep_copy(); }

/* */

std::unique_ptr<ArrayAccessExpression> ArrayAccessExpression::deep_copy() const {
    std::unique_ptr<ArrayAccessExpression> copy(new ArrayAccessExpression());
    if (resultType) { copy->resultType = resultType->deep_copy_decl_type(); }
    if (accessedArray) { copy->set_accessedArray(accessedArray->deep_copy()); }
    if (index) { copy->set_index(index->deepCopy_Exp()); }
    return copy;
}









