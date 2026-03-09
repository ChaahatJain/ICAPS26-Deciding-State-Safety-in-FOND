//
// This file is part of the PlaJA code base.
// Copyright (c) (2019 - 2019) Marcel Vinzent.
// See README.md in the top-level directory for licensing information.
//

#include "ast_visitor_const.h"

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

const AstElement* AstVisitorConst::cast_model(const Model* model) { return model; }

AstVisitorConst::AstVisitorConst() = default;

AstVisitorConst::~AstVisitorConst() = default;

void AstVisitorConst::visit(const Action*) { /*Nothing*/ }

void AstVisitorConst::visit(const Assignment* assignment) {
    JANI_ASSERT(assignment->get_ref())
    JANI_ASSERT(assignment->get_value() or (assignment->get_lower_bound() and assignment->get_upper_bound()))
    AST_CONST_ACCEPT_IF(assignment->get_ref(), LValueExpression)
    AST_CONST_ACCEPT_IF(assignment->get_value(), Expression)
    AST_CONST_ACCEPT_IF(assignment->get_lower_bound(), Expression)
    AST_CONST_ACCEPT_IF(assignment->get_upper_bound(), Expression)
}

void AstVisitorConst::visit(const Automaton* automaton) {
    // variables:
    AST_CONST_ITERATOR(automaton->variableIterator(), VariableDeclaration)
    // restrict-initial:
    AST_CONST_ACCEPT_IF(automaton->get_restrictInitialExpression(), Expression)
    // locations:
    AST_CONST_ITERATOR(automaton->locationIterator(), Location)
    // edges:
    AST_CONST_ITERATOR(automaton->edgeIterator(), Edge)
}

void AstVisitorConst::visit(const Composition* sys) {
    AST_CONST_ITERATOR(sys->elementIterator(), CompositionElement)
    AST_CONST_ITERATOR(sys->syncIterator(), Synchronisation)
}

void AstVisitorConst::visit(const CompositionElement*) { /*Nothing*/ }

void AstVisitorConst::visit(const ConstantDeclaration* const_decl) {
    JANI_ASSERT(const_decl->get_type())
    AST_CONST_ACCEPT_IF(const_decl->get_type(), DeclarationType)
    AST_CONST_ACCEPT_IF(const_decl->get_value(), Expression)
}

void AstVisitorConst::visit(const Destination* destination) {
    AST_CONST_ACCEPT_IF(destination->get_probabilityExpression(), Expression)
    AST_CONST_ITERATOR(destination->assignmentIterator(), Assignment)
}

void AstVisitorConst::visit(const Edge* edge) {
    AST_CONST_ACCEPT_IF(edge->get_rateExpression(), Expression)
    AST_CONST_ACCEPT_IF(edge->get_guardExpression(), Expression)
    AST_CONST_ITERATOR(edge->destinationIterator(), Destination)
}

void AstVisitorConst::visit(const Location* loc) {
    AST_CONST_ACCEPT_IF(loc->get_timeProgressCondition(), Expression)
    AST_CONST_ITERATOR(loc->transientValueIterator(), TransientValue)
}

void AstVisitorConst::visit(const Metadata* /*metadata*/) { /*Nothing*/ }

void AstVisitorConst::visit(const Model* model) {
    // actions:
    AST_CONST_ITERATOR(ModelIterator::actionIterator(*model), Action)
    // constants:
    AST_CONST_ITERATOR(ModelIterator::constantIterator(*model), ConstantDeclaration)
    // variables:
    AST_CONST_ITERATOR(ModelIterator::globalVariableIterator(*model), VariableDeclaration)
    // restrict-initial:
    AST_CONST_ACCEPT_IF(model->get_restrict_initial_expression(), Expression)
    // properties:
    AST_CONST_ITERATOR(ModelIterator::propertyIterator(*model), Property)
    // automata:
    AST_CONST_ITERATOR(ModelIterator::automatonInstanceIterator(*model), Automaton)
    // system:
    JANI_ASSERT(model->get_system())
    AST_CONST_ACCEPT_IF(model->get_system(), Composition)
}

void AstVisitorConst::visit(const Property* property) {
    JANI_ASSERT(property->get_propertyExpression())
    AST_CONST_ACCEPT_IF(property->get_propertyExpression(), PropertyExpression)
}

void AstVisitorConst::visit(const PropertyInterval* property_interval) {
    AST_CONST_ACCEPT_IF(property_interval->get_lower(), Expression)
    AST_CONST_ACCEPT_IF(property_interval->get_upper(), Expression)
}

void AstVisitorConst::visit(const RewardBound* reward_bound) {
    JANI_ASSERT(reward_bound->get_accumulation_expression() && reward_bound->get_bounds())
    AST_CONST_ACCEPT_IF(reward_bound->get_accumulation_expression(), Expression)
    AST_CONST_ACCEPT_IF(reward_bound->get_reward_accumulation(), RewardAccumulation)
    AST_CONST_ACCEPT_IF(reward_bound->get_bounds(), PropertyInterval)
}

void AstVisitorConst::visit(const RewardInstant* reward_instant) {
    JANI_ASSERT(reward_instant->get_accumulation_expression() && reward_instant->get_instant())
    AST_CONST_ACCEPT_IF(reward_instant->get_accumulation_expression(), Expression)
    AST_CONST_ACCEPT_IF(reward_instant->get_reward_accumulation(), RewardAccumulation)
    AST_CONST_ACCEPT_IF(reward_instant->get_instant(), Expression)
}

void AstVisitorConst::visit(const Synchronisation* /*sync*/) { /*Nothing*/ }

void AstVisitorConst::visit(const TransientValue* trans_val) {
    JANI_ASSERT(trans_val->get_ref() && trans_val->get_value())
    AST_CONST_ACCEPT_IF(trans_val->get_ref(), LValueExpression)
    AST_CONST_ACCEPT_IF(trans_val->get_value(), Expression)
}

void AstVisitorConst::visit(const VariableDeclaration* var_decl) {
    JANI_ASSERT(var_decl->get_type())
    AST_CONST_ACCEPT_IF(var_decl->get_type(), DeclarationType)
    AST_CONST_ACCEPT_IF(var_decl->get_initialValue(), Expression)
}

/* Non-standard. */

void AstVisitorConst::visit(const FreeVariableDeclaration* var_decl) {
    JANI_ASSERT(var_decl->get_type())
    AST_CONST_ACCEPT_IF(var_decl->get_type(), DeclarationType)
}

void AstVisitorConst::visit(const Properties* properties) { AST_CONST_ITERATOR(properties->propertyIterator(), Property) }

void AstVisitorConst::visit(const RewardAccumulation* /*reward_acc*/) {}

/* Expressions. */

void AstVisitorConst::visit(const ArrayAccessExpression* exp) {
    JANI_ASSERT(exp->get_accessedArray() && exp->get_index())
    AST_CONST_ACCEPT_IF(exp->get_accessedArray(), VariableExpression)
    AST_CONST_ACCEPT_IF(exp->get_index(), Expression)
}

void AstVisitorConst::visit(const ArrayConstructorExpression* exp) {
    JANI_ASSERT(exp->get_length() && exp->get_evalExp())
    AST_CONST_ACCEPT_IF(exp->get_freeVar(), FreeVariableDeclaration)
    AST_CONST_ACCEPT_IF(exp->get_length(), Expression)
    AST_CONST_ACCEPT_IF(exp->get_evalExp(), Expression)
}

void AstVisitorConst::visit(const ArrayValueExpression* exp) { AST_CONST_ITERATOR(exp->init_element_it(), Expression) }

void AstVisitorConst::visit(const BinaryOpExpression* exp) {
    JANI_ASSERT(exp->get_left() && exp->get_right())
    AST_CONST_ACCEPT_IF(exp->get_left(), Expression)
    AST_CONST_ACCEPT_IF(exp->get_right(), Expression)
}

void AstVisitorConst::visit(const BoolValueExpression*) { /*Nothing*/ }

void AstVisitorConst::visit(const ConstantExpression* exp) { AST_CONST_ACCEPT_IF(exp->get_value(), ConstantValueExpression) }

void AstVisitorConst::visit(const DerivativeExpression*) { /*Nothing*/ }

void AstVisitorConst::visit(const DistributionSamplingExpression* exp) { AST_CONST_ITERATOR(exp->argIterator(), Expression) }

void AstVisitorConst::visit(const ExpectationExpression* exp) {
    JANI_ASSERT(exp->get_value())
    AST_CONST_ACCEPT_IF(exp->get_value(), Expression)
    AST_CONST_ACCEPT_IF(exp->get_reach(), PropertyExpression)
    AST_CONST_ACCEPT_IF(exp->get_reward_accumulation(), RewardAccumulation)
    AST_CONST_ACCEPT_IF(exp->get_step_instant(), Expression)
    AST_CONST_ACCEPT_IF(exp->get_time_instant(), Expression)
    AST_CONST_ITERATOR(exp->rewardInstantIterator(), RewardInstant)
}

void AstVisitorConst::visit(const FilterExpression* exp) {
    JANI_ASSERT(exp->get_values() && exp->get_states())
    AST_CONST_ACCEPT_IF(exp->get_values(), PropertyExpression)
    AST_CONST_ACCEPT_IF(exp->get_states(), PropertyExpression)
}

void AstVisitorConst::visit(const FreeVariableExpression*) { /*Nothing*/ }

void AstVisitorConst::visit(const IntegerValueExpression*) { /*Nothing*/ }

void AstVisitorConst::visit(const ITE_Expression* exp) {
    JANI_ASSERT(exp->get_condition() && exp->get_consequence() && exp->get_alternative())
    AST_CONST_ACCEPT_IF(exp->get_condition(), Expression)
    AST_CONST_ACCEPT_IF(exp->get_consequence(), Expression)
    AST_CONST_ACCEPT_IF(exp->get_alternative(), Expression)
}

void AstVisitorConst::visit(const PathExpression* exp) {
    JANI_ASSERT(exp->get_right())
    if (!PathExpression::is_binary(exp->get_op())) {
        AST_CONST_ACCEPT_IF(exp->get_right(), PropertyExpression)
    } else {
        JANI_ASSERT(exp->get_left())
        AST_CONST_ACCEPT_IF(exp->get_left(), PropertyExpression)
        AST_CONST_ACCEPT_IF(exp->get_right(), PropertyExpression)
    }
    AST_CONST_ACCEPT_IF(exp->get_step_bounds(), PropertyInterval)
    AST_CONST_ACCEPT_IF(exp->get_time_bounds(), PropertyInterval)
    AST_CONST_ITERATOR(exp->rewardBoundIterator(), RewardBound)
}

void AstVisitorConst::visit(const QfiedExpression* exp) {
    JANI_ASSERT(exp->get_path_formula())
    AST_CONST_ACCEPT_IF(exp->get_path_formula(), PropertyExpression)
}

void AstVisitorConst::visit(const RealValueExpression*) { /*Nothing*/ }

void AstVisitorConst::visit(const StatePredicateExpression*) { /*Nothing*/ }

void AstVisitorConst::visit(const UnaryOpExpression* exp) {
    JANI_ASSERT(exp->get_operand())
    AST_CONST_ACCEPT_IF(exp->get_operand(), Expression)
}

void AstVisitorConst::visit(const VariableExpression*) { /*Nothing*/ }

/* Non-standard. */

void AstVisitorConst::visit(const ActionOpTuple*) {}

void AstVisitorConst::visit(const ConstantArrayAccessExpression* exp) { AST_CONST_ACCEPT_IF(exp->get_index(), Expression) }

void AstVisitorConst::visit(const CommentExpression*) { PLAJA_ABORT }

void AstVisitorConst::visit(const LetExpression* exp) {
    AST_CONST_ITERATOR(exp->init_free_variable_iterator(), FreeVariableDeclaration)
    AST_CONST_ACCEPT_IF(exp->get_expression(), Expression)
}

void AstVisitorConst::visit(const LocationValueExpression*) { /*Nothing*/ }

void AstVisitorConst::visit(const ObjectiveExpression* exp) {
    AST_CONST_ACCEPT_IF(exp->get_goal(), StateConditionExpression)
    AST_CONST_ACCEPT_IF(exp->get_goal_potential(), Expression)
    AST_CONST_ACCEPT_IF(exp->get_step_reward(), Expression)
    AST_CONST_ACCEPT_IF(exp->get_reward_accumulation(), RewardAccumulation)
}

void AstVisitorConst::visit(const PAExpression* exp) {
    AST_CONST_ACCEPT_IF(exp->get_start(), Expression)
    AST_CONST_ACCEPT_IF(exp->get_reach(), StateConditionExpression)
    AST_CONST_ACCEPT_IF(exp->get_objective(), ObjectiveExpression)
    AST_CONST_ACCEPT_IF(exp->get_predicates(), PredicatesExpression)
}

void AstVisitorConst::visit(const PredicatesExpression* exp) { AST_CONST_ITERATOR(exp->predicatesIterator(), Expression) }

void AstVisitorConst::visit(const ProblemInstanceExpression* exp) {
    AST_CONST_ACCEPT_IF(exp->get_start(), Expression)
    AST_CONST_ACCEPT_IF(exp->get_reach(), Expression)
    AST_CONST_ACCEPT_IF(exp->get_predicates(), PredicatesExpression)
}

void AstVisitorConst::visit(const StateConditionExpression* exp) {
    AST_CONST_ITERATOR(exp->init_loc_value_iterator(), LocationValueExpression)
    AST_CONST_ACCEPT_IF(exp->get_constraint(), Expression)
}

void AstVisitorConst::visit(const StateValuesExpression* exp) {
    AST_CONST_ITERATOR(exp->init_loc_value_iterator(), LocationValueExpression)
    AST_CONST_ITERATOR(exp->init_var_value_iterator(), VariableValueExpression)
}

void AstVisitorConst::visit(const StatesValuesExpression* exp) { AST_CONST_ITERATOR(exp->init_state_values_iterator(), StateValuesExpression) }

void AstVisitorConst::visit(const VariableValueExpression* exp) {
    AST_CONST_ACCEPT_IF(exp->get_var(), VariableExpression)
    AST_CONST_ACCEPT_IF(exp->get_val(), Expression)
}

/* Special cases. */

void AstVisitorConst::visit(const LinearExpression* exp) { for (auto addend_it = exp->addendIterator(); !addend_it.end(); ++addend_it) { addend_it.var()->accept(this); } }

void AstVisitorConst::visit(const NaryExpression* exp) { AST_CONST_ITERATOR(exp->iterator(), Expression) }

/* Types. */

void AstVisitorConst::visit(const ArrayType* type) {
    JANI_ASSERT(type->get_element_type())
    AST_CONST_ACCEPT_IF(type->get_element_type(), DeclarationType)
}

void AstVisitorConst::visit(const BoolType*) { /*Nothing*/ }

void AstVisitorConst::visit(const BoundedType* type) {
    JANI_ASSERT(type->get_base())
    AST_CONST_ACCEPT_IF(type->get_base(), BasicType)
    AST_CONST_ACCEPT_IF(type->get_lower_bound(), Expression)
    AST_CONST_ACCEPT_IF(type->get_upper_bound(), Expression)
}

void AstVisitorConst::visit(const ClockType*) { /*Nothing*/ }

void AstVisitorConst::visit(const ContinuousType*) { /*Nothing*/ }

void AstVisitorConst::visit(const IntType*) { /*Nothing*/ }

void AstVisitorConst::visit(const RealType*) { /*Nothing*/ }

/* Non-standard. */

void AstVisitorConst::visit(const LocationType*) { /*Nothing*/ }