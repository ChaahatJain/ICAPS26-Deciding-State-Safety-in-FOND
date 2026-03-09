//
// This file is part of the PlaJA code base.
// Copyright (c) (2019 - 2019) Marcel Vinzent.
// See README.md in the top-level directory for licensing information.
//

#include "variable_expression.h"
#include "../../../search/states/state_base.h"
#include "../../visitor/ast_visitor.h"
#include "../../visitor/ast_visitor_const.h"
#include "../type/declaration_type.h"
#include "../variable_declaration.h"

VariableExpression::VariableExpression(const VariableDeclaration& var_decl):
    decl(&var_decl) {
    resultType = var_decl.get_type()->deep_copy_decl_type();
}

VariableExpression::~VariableExpression() = default;

/* Setter. */

void VariableExpression::set_var(const VariableDeclaration& var_decl) {
    decl = &var_decl;
    resultType = var_decl.get_type()->deep_copy_decl_type();
}

/* Getter. */

const std::string& VariableExpression::get_name() const {
    PLAJA_ASSERT(decl)
    return decl->get_name();
}

/* Override. */

void VariableExpression::assign(StateBase& target, const StateBase* source, const Expression* value) const {
    const auto index = get_index();
    PLAJA_ASSERT(/*0 <= index && */ index < target.size())
    if (resultType->is_floating_type()) { target.assign_float<true>(index, value->evaluateFloating(source)); }
    else {
        PLAJA_ASSERT(resultType->is_integer_type() or resultType->is_boolean_type())
        target.assign_int<true>(index, value->evaluateInteger(source));
    }
    PLAJA_ASSERT(target.is_valid())
}

PLAJA::integer VariableExpression::access_integer(const StateBase& target, const StateBase* /*source*/) const { return target.get_int(get_index()); }

PLAJA::floating VariableExpression::access_floating(const StateBase& target, const StateBase* /*source*/) const { return target.get_float(get_index()); }

VariableID_type VariableExpression::get_variable_id() const { return get_id(); }

VariableIndex_type VariableExpression::get_variable_index(const StateBase* /*state*/) const { return get_index(); }

PLAJA::integer VariableExpression::evaluateInteger(const StateBase* state) const { return state->get_int(get_index()); }

PLAJA::floating VariableExpression::evaluateFloating(const StateBase* state) const { return state->operator[](get_index()); }

bool VariableExpression::is_constant() const { return false; }

bool VariableExpression::equals(const Expression* exp) const {
    auto* other = PLAJA_UTILS::cast_ptr_if<VariableExpression>(exp);
    if (not other) { return false; }
    PLAJA_ASSERT(this->get_id() != other->get_id() or (this->get_index() == other->get_index() and this->get_name() == other->get_name()))
    return this->get_id() == other->get_id();
}

std::size_t VariableExpression::hash() const { return get_id(); }

const DeclarationType* VariableExpression::determine_type() {
    PLAJA_ASSERT(resultType)
    return resultType.get(); // Expressions may not return nullptr.
}

void VariableExpression::accept(AstVisitor* ast_visitor) { ast_visitor->visit(this); }

void VariableExpression::accept(AstVisitorConst* ast_visitor) const { ast_visitor->visit(this); }

std::unique_ptr<LValueExpression> VariableExpression::deep_copy_lval_exp() const { return deep_copy(); }

/* */

std::unique_ptr<VariableExpression> VariableExpression::deep_copy() const {
    PLAJA_ASSERT(decl)
    return std::make_unique<VariableExpression>(*decl);
}








