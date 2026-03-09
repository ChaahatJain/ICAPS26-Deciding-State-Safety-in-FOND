//
// This file is part of the PlaJA code base.
// Copyright (c) (2019 - 2023) Marcel Vinzent.
// See README.md in the top-level directory for licensing information.
//

#include "split_on_locs.h"
#include "../../ast/expression/non_standard/location_value_expression.h"
#include "../../ast/expression/non_standard/state_condition_expression.h"
#include "split_on_binary_op.h"

CheckForStatesValues::CheckForStatesValues():
    rlt(0) {
}

CheckForStatesValues::~CheckForStatesValues() = default;

void CheckForStatesValues::visit(const StateValuesExpression*) { rlt = 1; }

void CheckForStatesValues::visit(const StatesValuesExpression*) { rlt = 2; }

bool CheckForStatesValues::is_state_values(const Expression& expr) {
    CheckForStatesValues check_for_states_values;
    expr.accept(&check_for_states_values);
    return check_for_states_values.rlt == 1;
}

bool CheckForStatesValues::is_states_values(const Expression& expr) {
    CheckForStatesValues check_for_states_values;
    expr.accept(&check_for_states_values);
    return check_for_states_values.rlt == 2;
}

bool CheckForStatesValues::is_state_or_states_values(const Expression& expr) {
    CheckForStatesValues check_for_states_values;
    expr.accept(&check_for_states_values);
    return check_for_states_values.rlt != 0;
}

/**********************************************************************************************************************/

std::unique_ptr<StateConditionExpression> ExtractOnLocs::extract(std::list<std::unique_ptr<Expression>>& conjuncts) {

    std::unique_ptr<StateConditionExpression> location_condition(nullptr);
    ExtractOnLocs extract_state_condition;

    for (auto it = conjuncts.begin(); it != conjuncts.end();) {
        PLAJA_ASSERT(not CheckForStatesValues::is_state_or_states_values(**it))

        PLAJA_ASSERT(not extract_state_condition.stateCondition)

        (*it)->accept(&extract_state_condition);

        auto& state_condition = extract_state_condition.stateCondition;
        if (state_condition) {

            if (state_condition->get_constraint()) { // Keep variable constraint in split list ...
                *it = extract_state_condition.stateCondition->set_constraint(nullptr);
                ++it;
            } else {
                it = conjuncts.erase(it);
            }

            if (location_condition) { // Merge locations.
                location_condition->move_locs_from(*state_condition);
                state_condition = nullptr; // Done here ...
            } else {
                location_condition = std::move(state_condition);
            }

        } else { ++it; }

    }

    return location_condition;

}

ExtractOnLocs::ExtractOnLocs():
    stateCondition(nullptr) {
}

ExtractOnLocs::~ExtractOnLocs() = default;

void ExtractOnLocs::visit(StateConditionExpression* exp) {
    PLAJA_ASSERT(not stateCondition)
    stateCondition = exp->move();
}

/**********************************************************************************************************************/

SplitOnLocs::SplitOnLocs() = default;

SplitOnLocs::~SplitOnLocs() = default;

std::unique_ptr<StateConditionExpression> SplitOnLocs::split(std::unique_ptr<Expression>&& expr) {
    PLAJA_ASSERT(not CheckForStatesValues::is_state_or_states_values(*expr))

    auto splits = SplitOnBinaryOp::split(std::move(expr), BinaryOpExpression::AND);

    auto state_condition = ExtractOnLocs::extract(splits);
    PLAJA_ASSERT(not state_condition or not state_condition->get_constraint()) // Var constraint moved to splits.
    if (not state_condition) { state_condition = std::make_unique<StateConditionExpression>(); }

    state_condition->set_constraint(SplitOnBinaryOp::to_expr(std::move(splits), BinaryOpExpression::AND));

    return state_condition;

}