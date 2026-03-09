//
// This file is part of the PlaJA code base.
// Copyright (c) (2019 - 2019) Marcel Vinzent.
// See README.md in the top-level directory for licensing information.
//

#include <algorithm>
#include "array_value_expression.h"
#include "../../../exception/semantics_exception.h"
#include "../../visitor/ast_visitor.h"
#include "../../visitor/ast_visitor_const.h"
#include "../type/array_type.h"

ArrayValueExpression::ArrayValueExpression() = default;

ArrayValueExpression::~ArrayValueExpression() = default;

/* Construction. */

void ArrayValueExpression::reserve(std::size_t elements_cap) { elements.reserve(elements_cap); }

void ArrayValueExpression::add_element(std::unique_ptr<Expression>&& element) { elements.push_back(std::move(element)); }

/* Override. */

std::vector<PLAJA::integer> ArrayValueExpression::evaluateIntegerArray(const StateBase* state) const {
    PLAJA_ASSERT(get_type()->is_array_type())
    PLAJA_ASSERT(PLAJA_UTILS::cast_ptr<ArrayType>(get_type())->is_integer_array() or PLAJA_UTILS::cast_ptr<ArrayType>(get_type())->is_boolean_array())

    std::vector<PLAJA::integer> target;
    target.reserve(elements.size());
    for (const auto& element: elements) { target.push_back(element->evaluateInteger(state)); }
    return target;
}

std::vector<PLAJA::floating> ArrayValueExpression::evaluateFloatingArray(const StateBase* state) const {
    std::vector<PLAJA::floating> target;
    target.reserve(elements.size());
    for (const auto& element: elements) { target.push_back(element->evaluateFloating(state)); }
    return target;
}

bool ArrayValueExpression::is_constant() const {
    return std::all_of(elements.cbegin(), elements.cend(), [](const std::unique_ptr<Expression>& element) { return element->is_constant(); });
}

bool ArrayValueExpression::equals(const Expression* exp) const {
    auto* other = PLAJA_UTILS::cast_ptr_if<ArrayValueExpression>(exp);
    if (not other) { return false; }
    const std::size_t l = elements.size();
    if (l != other->elements.size()) { return false; }
    for (std::size_t i = 0; i < l; ++i) {
        if (not this->elements[i]->equals(other->elements[i].get())) { return false; }
    }
    return true;
}

std::size_t ArrayValueExpression::hash() const {
    constexpr unsigned prime = 31;
    std::size_t result = 1;
    for (const auto& element: elements) {
        result = prime * result + element->hash();
    }
    return result;
}

const DeclarationType* ArrayValueExpression::determine_type() {
    if (resultType) { return resultType.get(); }
    else {

        JANI_ASSERT(not elements.empty())
        std::unique_ptr<DeclarationType> element_type = elements[0]->determine_type()->deep_copy_decl_type(); // At least one element invariant guaranteed by constructor.
        const auto end = elements.end();
        std::unique_ptr<DeclarationType> type_tmp;
        // TODO Assumption: if A,B and C can be assigned to a common type, the common type determined for A and B can too
        for (auto it = ++elements.begin(); it != end; ++it) {
            type_tmp = element_type->infer_common(*(*it)->determine_type());
            if (type_tmp != element_type) { // New inferred type.
                element_type = std::move(type_tmp);
                if (not element_type) { throw SemanticsException(this->to_string()); } // No common type.
            }
        }

        /* Set result type. */
        resultType = std::make_unique<ArrayType>();
        PLAJA_UTILS::cast_ptr<ArrayType>(resultType.get())->set_element_type(std::move(element_type));
        return resultType.get();

    }
}

void ArrayValueExpression::accept(AstVisitor* ast_visitor) { ast_visitor->visit(this); }

void ArrayValueExpression::accept(AstVisitorConst* ast_visitor) const { ast_visitor->visit(this); }

std::unique_ptr<Expression> ArrayValueExpression::deepCopy_Exp() const { return deep_copy(); }

std::unique_ptr<ArrayValueExpression> ArrayValueExpression::deep_copy() const {
    std::unique_ptr<ArrayValueExpression> copy(new ArrayValueExpression());

    if (resultType) { copy->resultType = resultType->deep_copy_decl_type(); }

    copy->elements.reserve(elements.size());
    for (const auto& element: elements) { copy->elements.emplace_back(element->deepCopy_Exp()); }

    return copy;
}




