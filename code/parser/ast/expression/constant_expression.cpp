//
// This file is part of the PlaJA code base.
// Copyright (c) (2019 - 2019) Marcel Vinzent.
// See README.md in the top-level directory for licensing information.
//

#include "constant_expression.h"
#include "../../visitor/ast_visitor.h"
#include "../../visitor/ast_visitor_const.h"
#include "../type/declaration_type.h"

ConstantExpression::ConstantExpression(const ConstantDeclaration& const_decl):
    decl(&const_decl) {
    PLAJA_ASSERT(const_decl.get_type())
    resultType = decl->get_type()->deep_copy_decl_type();
}

ConstantExpression::~ConstantExpression() = default;

/* Setter. */

void ConstantExpression::set_constant(const ConstantDeclaration& const_decl) {
    decl = &const_decl;
    resultType = decl->get_type()->deep_copy_decl_type();
}

/* Override. */

PLAJA::integer ConstantExpression::evaluateInteger(const StateBase* state) const {
    JANI_ASSERT(get_value())
    return get_value()->evaluateInteger(state);
}

PLAJA::floating ConstantExpression::evaluateFloating(const StateBase* state) const {
    JANI_ASSERT(get_value())
    return get_value()->evaluateFloating(state);
}

bool ConstantExpression::is_constant() const { return true; }

bool ConstantExpression::equals(const Expression* exp) const {
    auto* other = PLAJA_UTILS::cast_ptr_if<ConstantExpression>(exp);
    if (not other) { return false; }

    /* Set value of the constant is not part of its identity.
    if (this->value) {
        if (other->value) {
            if (not this->value->equals(other->value.get())) { return false; }
        }
        else { return false; }
    } else {
        if (other->value) { return false; }
    }
    */

    return this->get_id() == other->get_id(); // return this->get_name() == other->get_name();

}

std::size_t ConstantExpression::hash() const {
    constexpr unsigned prime = 31;
    std::size_t result = 1;
    result = prime * result + get_id(); // PLAJA_UTILS::hashString(get_name());
    return result;
}

const DeclarationType* ConstantExpression::determine_type() {
    PLAJA_ASSERT(resultType) // expressions may not return nullptr
    return resultType.get();
}

void ConstantExpression::accept(AstVisitor* ast_visitor) { ast_visitor->visit(this); }

void ConstantExpression::accept(AstVisitorConst* ast_visitor) const { ast_visitor->visit(this); }

std::unique_ptr<Expression> ConstantExpression::deepCopy_Exp() const {
    PLAJA_ASSERT(decl)
    return std::make_unique<ConstantExpression>(*decl);
}

