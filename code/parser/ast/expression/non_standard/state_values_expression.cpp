//
// This file is part of the PlaJA code base.
// Copyright (c) (2019 - 2021) Marcel Vinzent.
// See README.md in the top-level directory for licensing information.
//

#include <algorithm>
#include "state_values_expression.h"
#include "../../../../search/states/state_values.h"
#include "../../../visitor/ast_visitor.h"
#include "../../../visitor/ast_visitor_const.h"
#include "../../type/bool_type.h"
#include "../variable_expression.h"
#include "location_value_expression.h"
#include "variable_value_expression.h"

/* Static. */

#if 0
const std::string& StateConditionExpression::get_op_string() {
    static const std::string op_string = "state-values";
    return op_string;
}
#endif

StateValuesExpression::StateValuesExpression() { resultType = std::make_unique<BoolType>(); }

StateValuesExpression::~StateValuesExpression() = default;

// construction:

void StateValuesExpression::reserve(std::size_t loc_values_cap, std::size_t var_values_cap) { // NOLINT(*-easily-swappable-parameters)
    locValues.reserve(loc_values_cap);
    varValues.reserve(var_values_cap);
}

void StateValuesExpression::add_loc_value(std::unique_ptr<LocationValueExpression>&& value) {
    locValues.emplace_back(std::move(value));
}

void StateValuesExpression::add_var_value(std::unique_ptr<VariableValueExpression>&& value) {
    varValues.emplace_back(std::move(value));
}

// setter:

void StateValuesExpression::set_loc_value(std::unique_ptr<LocationValueExpression>&& value, std::size_t index) {
    PLAJA_ASSERT(index < locValues.size())
    locValues[index] = std::move(value);
}

void StateValuesExpression::set_var_value(std::unique_ptr<VariableValueExpression>&& value, std::size_t index) {
    PLAJA_ASSERT(index < varValues.size())
    varValues[index] = std::move(value);
}

//

std::unordered_set<VariableIndex_type> StateValuesExpression::retrieve_state_indexes() const {
    std::unordered_set<VariableIndex_type> state_indexes;
    // location values
    for (const auto& value: locValues) { state_indexes.insert(value->get_location_index()); }
    // variable values
    for (const auto& value: varValues) { state_indexes.insert(value->get_var()->get_index()); }
    //
    return state_indexes;
}

StateValues StateValuesExpression::construct_state_values(const StateValues& constructor) const {

    StateValues state_values(constructor);

    // location values
    for (const auto& value: locValues) { state_values.assign_int<true>(value->get_location_index(), value->get_location_value()); }

    // variable values
    for (const auto& value: varValues) { value->assign(state_values, nullptr); }

    return state_values;
}

// override:

PLAJA::integer StateValuesExpression::evaluateInteger(const StateBase* state) const {
    return std::all_of(locValues.begin(), locValues.end(), [state](const std::unique_ptr<LocationValueExpression>& value) { return value->evaluateInteger(state); }) and
           std::all_of(varValues.begin(), varValues.end(), [state](const std::unique_ptr<VariableValueExpression>& value) { return value->evaluateInteger(state); });
}

bool StateValuesExpression::is_constant() const { return locValues.empty() && varValues.empty(); }

bool StateValuesExpression::equals(const Expression* exp) const {
    auto* other = PLAJA_UTILS::cast_ptr_if<StateValuesExpression>(exp);
    if (not other) { return false; }

    /* Locations. */

    const std::size_t l_loc = get_size_loc_values();
    if (l_loc != other->get_size_loc_values()) { return false; }

    for (std::size_t i = 0; i < l_loc; ++i) {
        if (not get_loc_value(i)->equals(other->get_loc_value(i))) { return false; }
    }

    /* Variables. */

    const std::size_t l_var = get_size_var_values();
    if (l_var != other->get_size_loc_values()) { return false; }

    for (std::size_t i = 0; i < l_var; ++i) {
        if (not get_var_value(i)->equals(other->get_var_value(i))) { return false; }
    }

    return true;
}

std::size_t StateValuesExpression::hash() const {
    constexpr unsigned prime = 31;
    std::size_t result = 1;
    for (const auto& loc_value: locValues) { result = prime * result + loc_value->hash(); }
    return result;
}

void StateValuesExpression::accept(AstVisitor* ast_visitor) { ast_visitor->visit(this); }

void StateValuesExpression::accept(AstVisitorConst* ast_visitor) const { ast_visitor->visit(this); }

std::unique_ptr<Expression> StateValuesExpression::deepCopy_Exp() const { return deepCopy(); }

/**/

std::unique_ptr<StateValuesExpression> StateValuesExpression::deepCopy() const {
    std::unique_ptr<StateValuesExpression> copy(new StateValuesExpression());
    if (resultType) { copy->resultType = resultType->deep_copy_decl_type(); }
    copy->reserve(locValues.size(), varValues.size());
    for (const auto& value: locValues) { copy->add_loc_value(value->deep_copy()); }
    for (const auto& value: varValues) { copy->add_var_value(value->deepCopy()); }
    return copy;
}




