//
// This file is part of the PlaJA code base.
// Copyright (c) (2019 - 2019) Marcel Vinzent.
// See README.md in the top-level directory for licensing information.
//

#include "array_constructor_expression.h"
#include "../../visitor/ast_visitor.h"
#include "../../visitor/ast_visitor_const.h"
#include "../non_standard/free_variable_declaration.h"
#include "../type/declaration_type.h"

ArrayConstructorExpression::ArrayConstructorExpression() = default;

ArrayConstructorExpression::~ArrayConstructorExpression() = default;

/* */

void ArrayConstructorExpression::set_freeVar(std::unique_ptr<FreeVariableDeclaration>&& free_var) { freeVar = std::move(free_var); }

/* Auxiliary. */

const std::string& ArrayConstructorExpression::get_free_var_name() const { return freeVar->get_name(); }

/* Override. */

bool ArrayConstructorExpression::is_constant() const {
    JANI_ASSERT(evalExp)
    JANI_ASSERT(length->is_constant()) // Length must be constant anyway.
    return evalExp->is_constant(); // exp is not constant in particular if it does contain the free variable.
}

bool ArrayConstructorExpression::equals(const Expression* exp) const {
    JANI_ASSERT(length and evalExp)
    auto* other = PLAJA_UTILS::cast_ptr_if<ArrayConstructorExpression>(exp);
    if (not other) { return false; }

    JANI_ASSERT(other->length and other->evalExp)
    return this->freeVar->get_id() == this->freeVar->get_id()
           and this->length->equals(other->length.get())
           and this->evalExp->equals(other->evalExp.get());
}

std::size_t ArrayConstructorExpression::hash() const {
    JANI_ASSERT(length and evalExp)
    constexpr unsigned prime = 31;
    std::size_t result = 1;
    result = prime * result + (freeVar->get_id());
    result = prime * result + (length->hash());
    result = prime * result + (evalExp->hash());
    return result;
}

const DeclarationType* ArrayConstructorExpression::determine_type() {
    if (resultType) { return resultType.get(); }
    else {
        JANI_ASSERT(evalExp)
        resultType = evalExp->determine_type()->deep_copy_decl_type();
        return resultType.get();
    }
}

void ArrayConstructorExpression::accept(AstVisitor* ast_visitor) { ast_visitor->visit(this); }

void ArrayConstructorExpression::accept(AstVisitorConst* ast_visitor) const { ast_visitor->visit(this); }

std::unique_ptr<Expression> ArrayConstructorExpression::deepCopy_Exp() const {
    std::unique_ptr<ArrayConstructorExpression> copy(new ArrayConstructorExpression());
    if (resultType) { copy->resultType = resultType->deep_copy_decl_type(); }
    copy->set_freeVar(freeVar->deepCopy());
    if (length) { copy->set_length(length->deepCopy_Exp()); }
    if (evalExp) { copy->set_evalExp(evalExp->deepCopy_Exp()); }
    return copy;
}



