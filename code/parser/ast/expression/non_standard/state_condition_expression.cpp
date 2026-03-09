//
// This file is part of the PlaJA code base.
// Copyright (c) (2019 - 2021) Marcel Vinzent.
// See README.md in the top-level directory for licensing information.
//

#include "state_condition_expression.h"
#include "../../../../search/predicate_abstraction/pa_states/abstract_state.h"
#include "../../../visitor/normalform/split_on_locs.h"
#include "../../../visitor/ast_visitor.h"
#include "../../../visitor/ast_visitor_const.h"
#include "../../type/bool_type.h"
#include "../binary_op_expression.h"
#include "location_value_expression.h"

/* Static. */

const std::string& StateConditionExpression::get_op_string() {
    static const std::string op_string = "state-condition";
    return op_string;
}

std::unique_ptr<StateConditionExpression> StateConditionExpression::transform(std::unique_ptr<Expression>&& expr) {
    PLAJA_ASSERT(not CheckForStatesValues::is_state_or_states_values(*expr))
    return SplitOnLocs::split(std::move(expr));
}

/**/

StateConditionExpression::StateConditionExpression() { resultType = std::make_unique<BoolType>(); }

StateConditionExpression::~StateConditionExpression() = default;

/* Construction. */

void StateConditionExpression::reserve(std::size_t loc_values_cap) { locValues.reserve(loc_values_cap); }

void StateConditionExpression::add_loc_value(std::unique_ptr<LocationValueExpression>&& value) {
    PLAJA_ASSERT_EXPECT(std::all_of(locValues.cbegin(), locValues.cend(), [&value](const auto& existing_value) { return existing_value->get_location_index() != value->get_location_index(); }))
    locValues.push_back(std::move(value));
}

/* Setter. */

void StateConditionExpression::set_loc_value(std::unique_ptr<LocationValueExpression>&& value, std::size_t index) {
    PLAJA_ASSERT(index < locValues.size())
    locValues[index] = std::move(value);
}

/**/

bool StateConditionExpression::check_location_constraint(const StateBase* state) const {
    return std::all_of(locValues.cbegin(), locValues.cend(), [state](const std::unique_ptr<LocationValueExpression>& loc_value) { return loc_value->evaluateInteger(state); });
}

bool StateConditionExpression::check_state_variable_constraint(const StateBase* state) const {
    if (constraint) { return constraint->evaluateInteger(state); }
    else { return true; }
}

bool StateConditionExpression::check_location_constraint(const AbstractState* state) const {
    return std::all_of(locValues.cbegin(), locValues.cend(), [state](const std::unique_ptr<LocationValueExpression>& location_value) { return state->location_value(location_value->get_location_index()) == location_value->get_location_value(); });
}

/* Override. */

PLAJA::integer StateConditionExpression::evaluateInteger(const StateBase* state) const {
    return check_location_constraint(state) and check_state_variable_constraint(state);
}

bool StateConditionExpression::is_constant() const {
    return locValues.empty() and (not constraint or constraint->is_constant());
}

bool StateConditionExpression::equals(const Expression* exp) const {
    auto* other = PLAJA_UTILS::cast_ptr_if<StateConditionExpression>(exp);
    if (not other) { return false; }

    /* Locations. */

    const std::size_t l = get_size_loc_values();

    if (l != other->get_size_loc_values()) { return false; }

    for (std::size_t i = 0; i < l; ++i) {
        if (not get_loc_value(i)->equals(other->get_loc_value(i))) { return false; }
    }

    /* Constraint. */

    if (constraint) {
        return other->get_constraint() and constraint->equals(other->get_constraint());
    } else {
        return not other->get_constraint();
    }

}

std::size_t StateConditionExpression::hash() const {
    constexpr unsigned prime = 31;
    std::size_t result = 1;
    for (const auto& loc_value: locValues) {
        result = prime * result + loc_value->hash();
    }
    if (constraint) { result = prime * result + constraint->hash(); }
    return result;
}

void StateConditionExpression::accept(AstVisitor* ast_visitor) { ast_visitor->visit(this); }

void StateConditionExpression::accept(AstVisitorConst* ast_visitor) const { ast_visitor->visit(this); }

std::unique_ptr<Expression> StateConditionExpression::deepCopy_Exp() const { return deep_copy(); }

std::unique_ptr<Expression> StateConditionExpression::move_exp() { return move(); }

//

std::unique_ptr<StateConditionExpression> StateConditionExpression::deep_copy() const {
    std::unique_ptr<StateConditionExpression> copy(new StateConditionExpression());
    if (resultType) { copy->resultType = resultType->deep_copy_decl_type(); }
    copy->reserve(locValues.size());
    for (const auto& value: locValues) { copy->add_loc_value(value->deep_copy()); }
    if (constraint) { copy->set_constraint(constraint->deepCopy_Exp()); }
    return copy;
}

std::unique_ptr<StateConditionExpression> StateConditionExpression::move() {
    std::unique_ptr<StateConditionExpression> fresh(new StateConditionExpression());
    fresh->resultType = std::move(resultType);
    fresh->locValues = std::move(locValues);
    fresh->constraint = std::move(constraint);
    return fresh;
}

void StateConditionExpression::move_locs_from(StateConditionExpression& other) {
    reserve(get_size_loc_values() + other.get_size_loc_values());
    for (auto it = other.init_loc_value_iterator(); !it.end(); ++it) { add_loc_value(it.set()); } // Cannot move vector ...
}

void StateConditionExpression::move_constraint_from(StateConditionExpression& other) {
    if (other.get_constraint()) {

        if (get_constraint()) {

            auto merge_expr = std::make_unique<BinaryOpExpression>(BinaryOpExpression::AND);
            merge_expr->set_left(set_constraint());
            merge_expr->set_right(other.set_constraint());
            merge_expr->determine_type();
            set_constraint(std::move(merge_expr));

        } else {
            set_constraint(other.set_constraint());
        }

    }
}
