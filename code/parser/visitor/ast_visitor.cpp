//
// This file is part of the PlaJA code base.
// Copyright (c) (2019 - 2019) Marcel Vinzent.
// See README.md in the top-level directory for licensing information.
//

#include "ast_visitor.h"

#include "../ast/iterators/model_iterator.h"
#include "../ast/non_standard/free_variable_declaration.h"
#include "../ast/non_standard/properties.h"
#include "../ast/non_standard/reward_accumulation.h"
#include "../ast/action.h"
#include "../ast/assignment.h"
#include "../ast/automaton.h"
#include "../ast/composition.h"
#include "../ast/composition_element.h"
#include "../ast/constant_declaration.h"
#include "../ast/destination.h"
#include "../ast/edge.h"
#include "../ast/location.h"
#include "../ast/property.h"
#include "../ast/property_interval.h"
#include "../ast/reward_bound.h"
#include "../ast/reward_instant.h"
#include "../ast/transient_value.h"
#include "../ast/synchronisation.h"
#include "../ast/variable_declaration.h"

#include "../ast/expression/non_standard/constant_array_access_expression.h"
#include "../ast/expression/non_standard/let_expression.h"
#include "../ast/expression/non_standard/location_value_expression.h"
#include "../ast/expression/non_standard/objective_expression.h"
#include "../ast/expression/non_standard/predicate_abstraction_expression.h"
#include "../ast/expression/non_standard/predicates_expression.h"
#include "../ast/expression/non_standard/problem_instance_expression.h"
#include "../ast/expression/non_standard/state_condition_expression.h"
#include "../ast/expression/non_standard/state_values_expression.h"
#include "../ast/expression/non_standard/states_values_expression.h"
#include "../ast/expression/non_standard/variable_value_expression.h"
#include "../ast/expression/special_cases/linear_expression.h"
#include "../ast/expression/special_cases/nary_expression.h"
#include "../ast/expression/array_access_expression.h"
#include "../ast/expression/array_constructor_expression.h"
#include "../ast/expression/array_value_expression.h"
#include "../ast/expression/constant_expression.h"
#include "../ast/expression/constant_value_expression.h"
#include "../ast/expression/distribution_sampling_expression.h"
#include "../ast/expression/expectation_expression.h"
#include "../ast/expression/filter_expression.h"
#include "../ast/expression/ite_expression.h"
#include "../ast/expression/path_expression.h"
#include "../ast/expression/qfied_expression.h"
#include "../ast/expression/unary_op_expression.h"

#include "../ast/type/array_type.h"
#include "../ast/type/basic_type.h"
#include "../ast/type/bounded_type.h"

AstElement* AstVisitor::cast_model(Model* model) { return model; }

AstVisitor::AstVisitor():
    replace_by_result(false)
    CONSTRUCT_IF_DEBUG(automatonIndex(AUTOMATON::invalid)) {
}

AstVisitor::~AstVisitor() = default;

void AstVisitor::visit(Action*) { /*Nothing*/ }

void AstVisitor::visit(CommentExpression*) { PLAJA_ABORT }

void AstVisitor::visit(Assignment* assignment) {
    JANI_ASSERT(assignment->get_ref())
    JANI_ASSERT(assignment->get_value() or (assignment->get_lower_bound() and assignment->get_upper_bound()))
    AST_ACCEPT_IF(assignment->get_ref, assignment->set_ref, LValueExpression)
    AST_ACCEPT_IF(assignment->get_value, assignment->set_value, Expression)
    AST_ACCEPT_IF(assignment->get_lower_bound, assignment->set_lower_bound, Expression)
    AST_ACCEPT_IF(assignment->get_upper_bound, assignment->set_upper_bound, Expression)
}

void AstVisitor::visit(Automaton* automaton) {
    STMT_IF_DEBUG(automatonIndex = automaton->get_index();)
    // variables:
    AST_ITERATOR(automaton->variableIterator(), VariableDeclaration)
    // restrict-initial:
    AST_ACCEPT_IF(automaton->get_restrictInitialExpression, automaton->set_restrictInitialExpression, Expression)
    // locations:
    AST_ITERATOR(automaton->locationIterator(), Location)
    // edges:
    AST_ITERATOR(automaton->edgeIterator(), Edge)
    //
    STMT_IF_DEBUG(automatonIndex = AUTOMATON::invalid;)
}

void AstVisitor::visit(Composition* sys) {
    AST_ITERATOR(sys->elementIterator(), CompositionElement)
    AST_ITERATOR(sys->syncIterator(), Synchronisation)
}

void AstVisitor::visit(CompositionElement*) { /*NOTHING*/ }

void AstVisitor::visit(ConstantDeclaration* const_decl) {
    JANI_ASSERT(const_decl->get_type())
    AST_ACCEPT_IF(const_decl->get_type, const_decl->set_declaration_type, DeclarationType)
    AST_ACCEPT_IF(const_decl->get_value, const_decl->set_value, Expression)
}

void AstVisitor::visit(Destination* destination) {
    AST_ACCEPT_IF(destination->get_probabilityExpression, destination->set_probabilityExpression, Expression)
    AST_ITERATOR(destination->assignmentIterator(), Assignment)
}

void AstVisitor::visit(Edge* edge) {
    AST_ACCEPT_IF(edge->get_rateExpression, edge->set_rateExpression, Expression)
    AST_ACCEPT_IF(edge->get_guardExpression, edge->set_guardExpression, Expression)
    AST_ITERATOR(edge->destinationIterator(), Destination)
}

void AstVisitor::visit(Location* loc) {
    AST_ACCEPT_IF(loc->get_timeProgressCondition, loc->set_timeProgressExpression, Expression)
    AST_ITERATOR(loc->transientValueIterator(), TransientValue)
}

void AstVisitor::visit(Metadata*) { /*NOTHING*/ }

void AstVisitor::visit(Model* model) {
    // actions:
    AST_ITERATOR(ModelIterator::actionIterator(*model), Action)
    // constants:
    AST_ITERATOR(ModelIterator::constantIterator(*model), ConstantDeclaration)
    // variables:
    AST_ITERATOR(ModelIterator::globalVariableIterator(*model), VariableDeclaration)
    // restrict-initial:
    AST_ACCEPT_IF(model->get_restrict_initial_expression, model->set_restrict_initial_expression, Expression)
    // properties:
    AST_ITERATOR(ModelIterator::propertyIterator(*model), Property)
    // automata:
    AST_ITERATOR(ModelIterator::automatonInstanceIterator(*model), Automaton)
    // system:
    JANI_ASSERT(model->get_system())
    AST_ACCEPT_IF(model->get_system, model->set_system, Composition)
}

void AstVisitor::visit(Property* property) {
    JANI_ASSERT(property->get_propertyExpression())
    AST_ACCEPT_IF(property->get_propertyExpression, property->set_propertyExpression, PropertyExpression)
}

void AstVisitor::visit(PropertyInterval* property_interval) {
    AST_ACCEPT_IF(property_interval->get_lower, property_interval->set_lower, Expression)
    AST_ACCEPT_IF(property_interval->get_upper, property_interval->set_upper, Expression)
}

void AstVisitor::visit(RewardBound* reward_bound) {
    JANI_ASSERT(reward_bound->get_accumulation_expression() && reward_bound->get_bounds())
    AST_ACCEPT_IF(reward_bound->get_accumulation_expression, reward_bound->set_accumulation_expression, Expression)
    AST_ACCEPT_IF(reward_bound->get_reward_accumulation, reward_bound->set_reward_accumulation, RewardAccumulation)
    AST_ACCEPT_IF(reward_bound->get_bounds, reward_bound->set_bounds, PropertyInterval)
}

void AstVisitor::visit(RewardInstant* reward_instant) {
    JANI_ASSERT(reward_instant->get_accumulation_expression() && reward_instant->get_instant())
    AST_ACCEPT_IF(reward_instant->get_accumulation_expression, reward_instant->set_accumulation_expression, Expression)
    AST_ACCEPT_IF(reward_instant->get_reward_accumulation, reward_instant->set_reward_accumulation, RewardAccumulation)
    AST_ACCEPT_IF(reward_instant->get_instant, reward_instant->set_instant, Expression)
}

void AstVisitor::visit(Synchronisation* /*sync*/) { /*NOTHING*/ }

void AstVisitor::visit(TransientValue* trans_val) {
    JANI_ASSERT(trans_val->get_ref() && trans_val->get_value())
    AST_ACCEPT_IF(trans_val->get_ref, trans_val->set_ref, LValueExpression)
    AST_ACCEPT_IF(trans_val->get_value, trans_val->set_value, Expression)
}

void AstVisitor::visit(VariableDeclaration* var_decl) {
    JANI_ASSERT(var_decl->get_type())
    AST_ACCEPT_IF(var_decl->get_type, var_decl->set_declarationType, DeclarationType)
    AST_ACCEPT_IF(var_decl->get_initialValue, var_decl->set_initialValue, Expression)
}

/* Non-standard. */

void AstVisitor::visit(FreeVariableDeclaration* var_decl) {
    JANI_ASSERT(var_decl->get_type())
    AST_ACCEPT_IF(var_decl->get_type, var_decl->set_type, DeclarationType)
}

void AstVisitor::visit(Properties* properties) { AST_ITERATOR(properties->propertyIterator(), Property) }

void AstVisitor::visit(RewardAccumulation* /*reward_acc*/) {}

/* Expressions. */

void AstVisitor::visit(ArrayAccessExpression* exp) {
    JANI_ASSERT(exp->get_accessedArray() && exp->get_index())
    AST_ACCEPT_IF(exp->get_accessedArray, exp->set_accessedArray, VariableExpression)
    AST_ACCEPT_IF(exp->get_index, exp->set_index, Expression)
}

void AstVisitor::visit(ArrayConstructorExpression* exp) {
    JANI_ASSERT(exp->get_length() && exp->get_evalExp())
    AST_ACCEPT_IF(exp->get_freeVar, exp->set_freeVar, FreeVariableDeclaration)
    AST_ACCEPT_IF(exp->get_length, exp->set_length, Expression)
    AST_ACCEPT_IF(exp->get_evalExp, exp->set_evalExp, Expression)
}

void AstVisitor::visit(ArrayValueExpression* exp) { AST_ITERATOR(exp->init_element_it(), Expression) }

void AstVisitor::visit(BinaryOpExpression* exp) {
    JANI_ASSERT(exp->get_left() && exp->get_right())
    AST_ACCEPT_IF(exp->get_left, exp->set_left, Expression)
    AST_ACCEPT_IF(exp->get_right, exp->set_right, Expression)
}

void AstVisitor::visit(BoolValueExpression*) { /*NOTHING*/ }

void AstVisitor::visit(ConstantExpression*) { /*NOTHING*/ }

void AstVisitor::visit(DerivativeExpression*) { /*NOTHING*/ }

void AstVisitor::visit(DistributionSamplingExpression* exp) { AST_ITERATOR(exp->argIterator(), Expression) }

void AstVisitor::visit(ExpectationExpression* exp) {
    JANI_ASSERT(exp->get_value())
    AST_ACCEPT_IF(exp->get_value, exp->set_value, Expression)
    AST_ACCEPT_IF(exp->get_reach, exp->set_reach, PropertyExpression)
    AST_ACCEPT_IF(exp->get_reward_accumulation, exp->set_reward_accumulation, RewardAccumulation)
    AST_ACCEPT_IF(exp->get_step_instant, exp->set_step_instant, Expression)
    AST_ACCEPT_IF(exp->get_time_instant, exp->set_time_instant, Expression)
    AST_ITERATOR(exp->rewardInstantIterator(), RewardInstant)
}

void AstVisitor::visit(FilterExpression* exp) {
    JANI_ASSERT(exp->get_values() && exp->get_states())
    AST_ACCEPT_IF(exp->get_values, exp->set_values, PropertyExpression)
    AST_ACCEPT_IF(exp->get_states, exp->set_states, PropertyExpression)
}

void AstVisitor::visit(FreeVariableExpression*) { /*NOTHING*/ }

void AstVisitor::visit(IntegerValueExpression*) { /*NOTHING*/ }

void AstVisitor::visit(ITE_Expression* exp) {
    JANI_ASSERT(exp->get_condition() && exp->get_consequence() && exp->get_alternative())
    AST_ACCEPT_IF(exp->get_condition, exp->set_condition, Expression)
    AST_ACCEPT_IF(exp->get_consequence, exp->set_consequence, Expression)
    AST_ACCEPT_IF(exp->get_alternative, exp->set_alternative, Expression)
}

void AstVisitor::visit(PathExpression* exp) {
    JANI_ASSERT(exp->get_right())
    if (!PathExpression::is_binary(exp->get_op())) {
        AST_ACCEPT_IF(exp->get_right, exp->set_right, PropertyExpression)
    } else {
        JANI_ASSERT(exp->get_left())
        AST_ACCEPT_IF(exp->get_left, exp->set_left, PropertyExpression)
        AST_ACCEPT_IF(exp->get_right, exp->set_right, PropertyExpression)
    }
    AST_ACCEPT_IF(exp->get_step_bounds, exp->set_step_bounds, PropertyInterval)
    AST_ACCEPT_IF(exp->get_time_bounds, exp->set_time_bounds, PropertyInterval)
    AST_ITERATOR(exp->rewardBoundIterator(), RewardBound)
}

void AstVisitor::visit(QfiedExpression* exp) {
    JANI_ASSERT(exp->get_path_formula())
    AST_ACCEPT_IF(exp->get_path_formula, exp->set_path_formula, PropertyExpression)
}

void AstVisitor::visit(RealValueExpression*) { /*NOTHING*/ }

void AstVisitor::visit(StatePredicateExpression*) { /*NOTHING*/ }

void AstVisitor::visit(UnaryOpExpression* exp) {
    JANI_ASSERT(exp->get_operand())
    AST_ACCEPT_IF(exp->get_operand, exp->set_operand, Expression)
}

void AstVisitor::visit(VariableExpression*) { /*NOTHING*/ }

/* Non-standard. */

void AstVisitor::visit(ActionOpTuple*) {}

void AstVisitor::visit(ConstantArrayAccessExpression* exp) { AST_ACCEPT_IF(exp->get_index, exp->set_index, Expression) }

void AstVisitor::visit(LetExpression* exp) {
    AST_ITERATOR(exp->init_free_variable_iterator(), FreeVariableDeclaration)
    AST_ACCEPT_IF(exp->get_expression, exp->set_expression, Expression)
}

void AstVisitor::visit(LocationValueExpression*) { /*NOTHING*/ }

void AstVisitor::visit(ObjectiveExpression* exp) {
    AST_ACCEPT_IF(exp->get_goal, exp->set_goal, StateConditionExpression)
    AST_ACCEPT_IF(exp->get_goal_potential, exp->set_goal_potential, Expression)
    AST_ACCEPT_IF(exp->get_step_reward, exp->set_step_reward, Expression)
    AST_ACCEPT_IF(exp->get_reward_accumulation, exp->set_reward_accumulation, RewardAccumulation)
}

void AstVisitor::visit(PAExpression* exp) {
    AST_ACCEPT_IF(exp->get_start, exp->set_start, Expression)
    AST_ACCEPT_IF(exp->get_reach, exp->set_reach, StateConditionExpression)
    AST_ACCEPT_IF(exp->get_objective, exp->set_objective, ObjectiveExpression)
    AST_ACCEPT_IF(exp->get_predicates, exp->set_predicates, PredicatesExpression)
}

void AstVisitor::visit(PredicatesExpression* exp) { AST_ITERATOR(exp->predicatesIterator(), Expression) }

void AstVisitor::visit(ProblemInstanceExpression* exp) {
    AST_ACCEPT_IF(exp->get_start_ptr, exp->set_start, Expression)
    AST_ACCEPT_IF(exp->get_reach_ptr, exp->set_reach, StateConditionExpression)
    AST_ACCEPT_IF(exp->get_predicates_ptr, exp->set_predicates, PredicatesExpression)
}

void AstVisitor::visit(StateConditionExpression* exp) {
    AST_ITERATOR(exp->init_loc_value_iterator(), LocationValueExpression)
    AST_ACCEPT_IF(exp->get_constraint, exp->set_constraint, Expression)
}

void AstVisitor::visit(StateValuesExpression* exp) {
    AST_ITERATOR(exp->init_loc_value_iterator(), LocationValueExpression)
    AST_ITERATOR(exp->init_var_value_iterator(), VariableValueExpression)
}

void AstVisitor::visit(StatesValuesExpression* exp) { AST_ITERATOR(exp->init_state_values_iterator(), StateValuesExpression) }

void AstVisitor::visit(VariableValueExpression* exp) {
    AST_ACCEPT_IF(exp->get_var, exp->set_var, VariableExpression)
    AST_ACCEPT_IF(exp->get_val, exp->set_val, Expression)
}

// special cases:

void AstVisitor::visit(LinearExpression* exp) { for (auto addend_it = exp->addendIterator(); !addend_it.end(); ++addend_it) { addend_it.var()->accept(this); } }

void AstVisitor::visit(NaryExpression* exp) { AST_ITERATOR(exp->iterator(), Expression) }

// types:

void AstVisitor::visit(ArrayType* type) {
    JANI_ASSERT(type->get_element_type())
    AST_ACCEPT_IF(type->get_element_type, type->set_element_type, DeclarationType)
}

void AstVisitor::visit(BoolType*) { /*NOTHING*/ }

void AstVisitor::visit(BoundedType* type) {
    JANI_ASSERT(type->get_base())
    AST_ACCEPT_IF(type->get_base, type->set_base, BasicType)
    AST_ACCEPT_IF(type->get_lower_bound, type->set_lower_bound, Expression)
    AST_ACCEPT_IF(type->get_upper_bound, type->set_upper_bound, Expression)
}

void AstVisitor::visit(ClockType*) { /*NOTHING*/ }

void AstVisitor::visit(ContinuousType*) { /*NOTHING*/ }

void AstVisitor::visit(IntType*) { /*NOTHING*/ }

void AstVisitor::visit(RealType*) { /*NOTHING*/ }

/* Non-standard. */

void AstVisitor::visit(LocationType*) { /*NOTHING*/ }