//
// This file is part of the PlaJA code base.
// Copyright (c) (2019 - 2019) Marcel Vinzent.
// See README.md in the top-level directory for licensing information.
//

#include "variable_declaration.h"
#include "../visitor/ast_visitor.h"
#include "../visitor/ast_visitor_const.h"
#include "type/declaration_type.h"
#include "expression/expression.h"

VariableDeclaration::VariableDeclaration(VariableID_type var_id):
    name(PLAJA_UTILS::emptyString)
    , declarationType()
    , transient()
    , initialValue()
    , id(var_id)
    , index(STATE_INDEX::none)
    , size(static_cast<std::size_t>(-1))
/*automatonIndex(AUTOMATON::invalid)*/ {
}

VariableDeclaration::~VariableDeclaration() = default;

/* Setter. */

std::unique_ptr<DeclarationType> VariableDeclaration::set_declarationType(std::unique_ptr<DeclarationType>&& decl_type) {
    std::swap(declarationType, decl_type);
    return std::move(decl_type);
}

std::unique_ptr<Expression> VariableDeclaration::set_initialValue(std::unique_ptr<Expression>&& init_val) {
    std::swap(initialValue, init_val);
    return std::move(init_val);
}

/* */

std::unique_ptr<VariableDeclaration> VariableDeclaration::deep_copy() const {
    std::unique_ptr<VariableDeclaration> copy(new VariableDeclaration(this->id));
    copy->copy_comment(*this);
    copy->set_name(name);
    copy->set_index(index);
    if (declarationType) { copy->set_declarationType(declarationType->deep_copy_decl_type()); }
    copy->set_transient(transient);
    if (initialValue) { copy->set_initialValue(initialValue->deepCopy_Exp()); }
    // copy->automatonIndex = automatonIndex;
    return copy;
}

/* Override. */

void VariableDeclaration::accept(AstVisitor* ast_visitor) { ast_visitor->visit(this); }

void VariableDeclaration::accept(AstVisitorConst* ast_visitor) const { ast_visitor->visit(this); }

/* */

bool VariableDeclaration::is_array() const { return declarationType->is_array_type(); }

bool VariableDeclaration::is_scalar() const {
    PLAJA_ASSERT(declarationType->is_array_type() or declarationType->is_boolean_type() or declarationType->is_integer_type() or declarationType->is_floating_type())
    return not declarationType->is_array_type();
}
