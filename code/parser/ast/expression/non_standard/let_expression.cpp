//
// This file is part of the PlaJA code base.
// Copyright (c) (2019 - 2021) Marcel Vinzent.
// See README.md in the top-level directory for licensing information.
//

#include "let_expression.h"
#include "../../../visitor/ast_visitor.h"
#include "../../../visitor/ast_visitor_const.h"
#include "../../non_standard/free_variable_declaration.h"
#include "../../type/declaration_type.h"

/* Static. */

const std::string& LetExpression::get_op_string() {
    static const std::string op_string = "let";
    return op_string;
}

/**/

LetExpression::LetExpression() = default;

LetExpression::~LetExpression() = default;

//

void LetExpression::reserve(std::size_t free_variables_cap) {
    freeVariables.reserve(free_variables_cap);
}

void LetExpression::add_free_variable(std::unique_ptr<FreeVariableDeclaration>&& free_variable) {
    freeVariables.push_back(std::move(free_variable));
}

void LetExpression::set_free_variable(std::unique_ptr<FreeVariableDeclaration>&& free_variable, std::size_t index) {
    PLAJA_ASSERT(index < freeVariables.size())
    freeVariables[index] = std::move(free_variable);
}

//

PLAJA::integer LetExpression::evaluateInteger(const StateBase* state) const {
    JANI_ASSERT(expression)
    return expression->evaluateInteger(state);
}

bool LetExpression::is_constant() const {
    JANI_ASSERT(expression)
    return expression->is_constant();
}

bool LetExpression::equals(const Expression* exp) const {
    auto* other = PLAJA_UTILS::cast_ptr_if<LetExpression>(exp);
    if (not other) { return false; }

    /* Locations. */

    const std::size_t l = get_number_of_free_variables();

    if (l != other->get_number_of_free_variables()) { return false; }

    for (std::size_t i = 0; i < l; ++i) {
        if (get_free_variable(i)->get_id() != other->get_free_variable(i)->get_id()) { return false; }
    }

    /* Constraint. */
    JANI_ASSERT(expression)
    JANI_ASSERT(other->get_expression())
    return expression->equals(other->get_expression());

}

std::size_t LetExpression::hash() const {
    constexpr unsigned prime = 31;
    std::size_t result = 1;
    for (const auto& free_var: freeVariables) { result = prime * result + free_var->get_id(); }
    JANI_ASSERT(expression)
    result = prime * result + expression->hash();
    return result;
}

const DeclarationType* LetExpression::determine_type() {
    JANI_ASSERT(expression)
    const auto* type = expression->determine_type();
    if (type) { resultType = type->deep_copy_decl_type(); }
    return resultType.get();
}

//

void LetExpression::accept(AstVisitor* ast_visitor) { ast_visitor->visit(this); }

void LetExpression::accept(AstVisitorConst* ast_visitor) const { ast_visitor->visit(this); }

std::unique_ptr<Expression> LetExpression::deepCopy_Exp() const { return deep_copy(); }

std::unique_ptr<Expression> LetExpression::move_exp() { return move(); }

std::unique_ptr<LetExpression> LetExpression::deep_copy() const {
    std::unique_ptr<LetExpression> copy(new LetExpression());

    if (resultType) { copy->resultType = resultType->deep_copy_decl_type(); }

    copy->reserve(freeVariables.size());
    for (const auto& free_variable: freeVariables) { copy->add_free_variable(free_variable->deepCopy()); }
    if (expression) { copy->set_expression(expression->deepCopy_Exp()); }

    return copy;
}

std::unique_ptr<LetExpression> LetExpression::move() {
    std::unique_ptr<LetExpression> fresh(new LetExpression());
    fresh->resultType = std::move(resultType);
    fresh->freeVariables = std::move(freeVariables);
    fresh->expression = std::move(expression);
    return fresh;
}
