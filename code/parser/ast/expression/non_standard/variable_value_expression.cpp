//
// This file is part of the PlaJA code base.
// Copyright (c) (2019 - 2021) Marcel Vinzent.
// See README.md in the top-level directory for licensing information.
//

#include "variable_value_expression.h"
#include "../../../../search/using_search.h"
#include "../../../../utils/floating_utils.h"
#include "../../../visitor/ast_visitor.h"
#include "../../../visitor/ast_visitor_const.h"
#include "../../type/bool_type.h"
#include "../variable_expression.h"

VariableValueExpression::VariableValueExpression() { resultType = std::make_unique<BoolType>(); }

VariableValueExpression::~VariableValueExpression() = default;

/* Setter. */

void VariableValueExpression::set_var(std::unique_ptr<VariableExpression>&& var_r) { var = std::move(var_r); }

/* Override. */

PLAJA::integer VariableValueExpression::evaluateInteger(const StateBase* state) const {
    if (var->get_type()->is_floating_type()) { return PLAJA_FLOATS::equal(var->evaluateFloating(state), val->evaluateFloating(state), PLAJA::floatingPrecision); }
    else { return var->evaluateInteger(state) == val->evaluateInteger(state); }
}

void VariableValueExpression::assign(StateBase& target, const StateBase* source) const { var->assign(target, source, val.get()); }

bool VariableValueExpression::equals(const Expression* exp) const {
    auto* other = PLAJA_UTILS::cast_ptr_if<VariableValueExpression>(exp);
    if (not other) { return false; }
    PLAJA_ASSERT(var and other->get_var() and val and other->get_val())
    return other->get_var()->equals(get_var()) and other->get_val()->equals(get_val());
}

std::size_t VariableValueExpression::hash() const {
    constexpr unsigned prime = 31;
    std::size_t result = 1;
    result = prime * result + var->hash();
    result = prime * result + val->hash();
    return result;
}

void VariableValueExpression::accept(AstVisitor* ast_visitor) { ast_visitor->visit(this); }

void VariableValueExpression::accept(AstVisitorConst* ast_visitor) const { ast_visitor->visit(this); }

std::unique_ptr<Expression> VariableValueExpression::deepCopy_Exp() const { return deepCopy(); }

/**/

std::unique_ptr<VariableValueExpression> VariableValueExpression::deepCopy() const {
    std::unique_ptr<VariableValueExpression> copy(new VariableValueExpression());
    if (resultType) { copy->resultType = resultType->deep_copy_decl_type(); }
    copy->set_var(var->deep_copy());
    copy->set_val(val->deepCopy_Exp());
    return copy;
}




