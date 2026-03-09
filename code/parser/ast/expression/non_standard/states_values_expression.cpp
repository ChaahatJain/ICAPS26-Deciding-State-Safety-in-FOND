//
// This file is part of the PlaJA code base.
// Copyright (c) (2019 - 2021) Marcel Vinzent.
// See README.md in the top-level directory for licensing information.
//

#include <algorithm>
#include "states_values_expression.h"
#include "../../../visitor/ast_visitor.h"
#include "../../../visitor/ast_visitor_const.h"
#include "../../type/declaration_type.h"
#include "state_values_expression.h"

/* Static. */

const std::string& StatesValuesExpression::get_op_string() {
    static const std::string op_string = "states-values";
    return op_string;
}

StatesValuesExpression::StatesValuesExpression() = default;

StatesValuesExpression::~StatesValuesExpression() = default;

/* Construction. */

void StatesValuesExpression::reserve(std::size_t states_values_cap) { statesValues.reserve(states_values_cap); }

void StatesValuesExpression::add_state_values(std::unique_ptr<StateValuesExpression>&& state_values) { statesValues.emplace_back(std::move(state_values)); }

/* Setter. */

void StatesValuesExpression::set_state_values(std::unique_ptr<StateValuesExpression>&& state_values, std::size_t index) {
    PLAJA_ASSERT(index < statesValues.size())
    statesValues[index] = std::move(state_values);
}

/* Auxiliary. */

std::unordered_set<VariableIndex_type> StatesValuesExpression::retrieve_state_indexes() const {
    std::unordered_set<VariableIndex_type> state_indexes;
    for (const auto& state_values: statesValues) { state_indexes.merge(state_values->retrieve_state_indexes()); }
    return state_indexes;
}

/* Override. */

PLAJA::integer StatesValuesExpression::evaluateInteger(const StateBase* state) const {
    return std::any_of(statesValues.begin(), statesValues.end(), [state](const std::unique_ptr<StateValuesExpression>& value) { return value->evaluateInteger(state); });
}

bool StatesValuesExpression::is_constant() const {
    return std::all_of(statesValues.begin(), statesValues.end(), [](const std::unique_ptr<StateValuesExpression>& value) { return value->is_constant(); });
}

bool StatesValuesExpression::equals(const Expression* exp) const {
    auto* other = PLAJA_UTILS::cast_ptr_if<StatesValuesExpression>(exp);
    if (not other) { return false; }

    /* Locations. */

    const std::size_t l = get_size_states_values();

    if (l != other->get_size_states_values()) { return false; }

    for (std::size_t i = 0; i < l; ++i) {
        if (not get_state_values(i)->equals(other->get_state_values(i))) { return false; }
    }

    return true;
}

std::size_t StatesValuesExpression::hash() const {
    constexpr unsigned prime = 31;
    std::size_t result = 1;
    for (const auto& state_values: statesValues) { result = prime * result + state_values->hash(); }
    return result;
}

std::unique_ptr<Expression> StatesValuesExpression::deepCopy_Exp() const {
    std::unique_ptr<StatesValuesExpression> copy(new StatesValuesExpression());
    if (resultType) { copy->resultType = resultType->deep_copy_decl_type(); }
    copy->reserve(statesValues.size());
    for (const auto& state_values: statesValues) { copy->add_state_values(state_values->deepCopy()); }
    return copy;
}

void StatesValuesExpression::accept(AstVisitor* ast_visitor) { ast_visitor->visit(this); }

void StatesValuesExpression::accept(AstVisitorConst* ast_visitor) const { ast_visitor->visit(this); }
