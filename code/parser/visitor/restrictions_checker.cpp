//
// This file is part of the PlaJA code base.
// Copyright (c) (2019 - 2020) Marcel Vinzent.
// See README.md in the top-level directory for licensing information.
//

#include <iostream>
#include "restrictions_checker.h"
#include "../../exception/not_supported_exception.h"
#include "../../utils/utils.h"
#include "../ast/ast_element.h"
#include "../jani_words.h"

#include "../ast/iterators/model_iterator.h"
#include "../ast/non_standard/free_variable_declaration.h"
#include "../ast/non_standard/reward_accumulation.h"
#include "../ast/automaton.h"
#include "../ast/composition_element.h"
#include "../ast/constant_declaration.h"
#include "../ast/edge.h"
#include "../ast/location.h"
#include "../ast/property.h"
#include "../ast/property_interval.h"
#include "../ast/reward_bound.h"
#include "../ast/reward_instant.h"
#include "../ast/variable_declaration.h"

#include "../ast/expression/non_standard/predicate_abstraction_expression.h"
#include "../ast/expression/array_constructor_expression.h"
#include "../ast/expression/constant_expression.h"
#include "../ast/expression/bool_value_expression.h"
#include "../ast/expression/derivative_expression.h"
#include "../ast/expression/distribution_sampling_expression.h"
#include "../ast/expression/expectation_expression.h"
#include "../ast/expression/free_variable_expression.h"
#include "../ast/expression/path_expression.h"
#include "../ast/expression/qfied_expression.h"

#include "../ast/type/array_type.h"
#include "../ast/type/bounded_type.h"

RestrictionsChecker::RestrictionsChecker(): hasFloatingVar(false) {}

RestrictionsChecker::~RestrictionsChecker() = default;

/* */

bool RestrictionsChecker::check_restrictions(const AstElement* ast_element, bool catch_exception) {
    RestrictionsChecker restrictions_checker;

	if (catch_exception) { // branch outside of try-catch to keep copy constructor of exception deleted
    	try { ast_element->accept(&restrictions_checker); }
        catch (NotSupportedException& e) { std::cout << e.what() << std::endl; }
	} else {
    	ast_element->accept(&restrictions_checker);
	}

    return true;
}

//

void RestrictionsChecker::visit(const Automaton* automaton) {
    // restrict-initial:
    if (automaton->get_restrictInitialExpression()) { throw NotSupportedException(automaton->to_string(), JANI::RESTRICT_INITIAL); }
    // locations:
    if (automaton->get_number_initialLocations() != 1) { throw NotSupportedException(automaton->to_string(), "several " + JANI::INITIAL_LOCATIONS); }
    // variables:
    if (hasFloatingVar and automaton->get_number_variables() > 0) { throw NotSupportedException(automaton->to_string(), "local vars not supported when using floating vars"); }
    //
    AstVisitorConst::visit(automaton);
}

void RestrictionsChecker::visit(const CompositionElement* sys_el) {
    if (sys_el->get_number_inputEnables() > 0) { throw NotSupportedException(sys_el->to_string(), JANI::INPUT_ENABLE); }
}

void RestrictionsChecker::visit(const ConstantDeclaration* const_decl) {
    if (!const_decl->get_value()) { throw NotSupportedException(const_decl->to_string(), JANI::VALUE); }
    AstVisitorConst::visit(const_decl);
}

void RestrictionsChecker::visit(const Edge* edge) {
    if (edge->get_rateExpression()) { throw NotSupportedException(edge->to_string(), JANI::RATE); }
    AstVisitorConst::visit(edge);
}

void RestrictionsChecker::visit(const Location* loc) {
    if (loc->get_timeProgressCondition()) { throw NotSupportedException(loc->to_string(), JANI::TIME_PROGRESS); }
    AstVisitorConst::visit(loc);
}

void RestrictionsChecker::visit(const Model* model) {
    // type:
    if (model->get_model_type() != Model::LTS && model->get_model_type() != Model::MDP) { throw NotSupportedException(model->to_string(), JANI::TYPE); }
    // features:
    for (const auto& model_feature: model->get_model_features()) {
        if (model_feature != Model::ARRAYS && model_feature != Model::DERIVED_OPERATORS && model_feature != Model::STATE_EXIT_REWARDS) {
            throw NotSupportedException(Model::model_feature_to_str(model_feature), JANI::FEATURES);
        }
    }
    // restrict-initial:
    if (model->get_restrict_initial_expression()) { throw NotSupportedException(model->to_string(), JANI::RESTRICT_INITIAL); }
    // variables:
    for (std::size_t i = 0; i < model->get_number_variables(); ++i) {
        auto* var = model->get_variable(i);
        if (var->get_type()->is_floating_type()) { hasFloatingVar = true; }
        else if (hasFloatingVar) { throw NotSupportedException(model->to_string(), "first discrete vars, then floating vars, no intermingling"); }
    }
    //
    AstVisitorConst::visit(model);
}

void RestrictionsChecker::visit(const Property* property) {
    try { AstVisitorConst::visit(property); }
    catch (NotSupportedException& e) {
        std::cout << "Property \"" << property->get_name() << "\" is not supported." << std::endl; // this property shall be skipped
    }
}

void RestrictionsChecker::visit(const PropertyInterval* property_interval) { throw NotSupportedException(property_interval->to_string(), JANI::BOUNDS); }

void RestrictionsChecker::visit(const RewardBound* reward_bound) { throw NotSupportedException(reward_bound->to_string(), JANI::REWARD_BOUNDS); }

void RestrictionsChecker::visit(const RewardInstant* reward_instant) { throw NotSupportedException(reward_instant->to_string(), JANI::REWARD_INSTANTS); }

void RestrictionsChecker::visit(const VariableDeclaration* var_decl) {
    AstVisitorConst::visit(var_decl);

    { /*CURRENTLY SUPPORTED*/
        const auto* type = var_decl->get_type();
        if (type->is_boolean_type() or (type->is_bounded_type() and (type->is_integer_type() or type->is_floating_type()))) {}
        else if (type->is_array_type()) {
            const auto* element_type = PLAJA_UTILS::cast_ptr<ArrayType>(type)->get_element_type();
            if (element_type->is_boolean_type() or (element_type->is_bounded_type() and (element_type->is_integer_type() or element_type->is_floating_type()))) {}
            else { throw NotSupportedException(var_decl->to_string(), JANI::TYPE); }
        }
        else { throw NotSupportedException(var_decl->to_string(), JANI::TYPE); }
    } /*CURRENTLY SUPPORTED*/

    if (!var_decl->get_initialValue()) { throw NotSupportedException(var_decl->to_string(), JANI::INITIAL_VALUE); }
}

// non-standard:

void RestrictionsChecker::visit(const FreeVariableDeclaration* var_decl) {
    AstVisitorConst::visit(var_decl);
    //
    const auto* type = var_decl->get_type();
    if (! (type->is_boolean_type() || (type->is_bounded_type() && type->is_integer_type())) ) { // trivial type supported: finite && discrete domain
        throw NotSupportedException(var_decl->to_string(), JANI::TYPE);
    }
}

void RestrictionsChecker::visit(const RewardAccumulation* reward_acc) {
    if (reward_acc->accumulate_time()) { throw NotSupportedException(reward_acc->to_string(), RewardAccumulation::reward_acc_value_to_str(RewardAccumulation::TIME)); }
}

// expressions:

void RestrictionsChecker::visit(const ArrayConstructorExpression* exp) { throw NotSupportedException(exp->to_string(), JANI::AC); }

void RestrictionsChecker::visit(const ConstantExpression* exp) { if (exp->get_type()->is_array_type()) { throw NotSupportedException(exp->to_string(), JANI::ARRAY); } }

void RestrictionsChecker::visit(const DerivativeExpression* exp) { throw NotSupportedException(exp->to_string(), JANI::DER); }

void RestrictionsChecker::visit(const DistributionSamplingExpression* exp) { throw NotSupportedException(exp->to_string(), JANI::DISTRIBUTION); }

void RestrictionsChecker::visit(const ExpectationExpression* exp) {
    if (exp->get_step_instant()) { throw NotSupportedException(exp->to_string(), JANI::STEP_INSTANT); }
    if (exp->get_time_instant()) { throw NotSupportedException(exp->to_string(), JANI::TIME_INSTANT); }
    if (exp->get_number_reward_instants() > 0) { throw NotSupportedException(exp->to_string(), JANI::REWARD_INSTANTS); }
    //
    AstVisitorConst::visit(exp);
    // value:
    JANI_ASSERT(exp->get_value())
    PLAJA_ASSERT(exp->get_value()->is_proposition())
    // accumulate:
    if (exp->get_reward_accumulation() and (exp->get_reward_accumulation()->size() != 1 or not exp->get_reward_accumulation()->accumulate_exit())) {
       throw NotSupportedException(exp->to_string(), JANI::ACCUMULATE);
    }
    // reach:
    if (not exp->get_reach() or not exp->get_reach()->is_proposition() or not PLAJA_UTILS::is_derived_ptr_type<Expression>(exp->get_reach())) { throw NotSupportedException(exp->to_string(), JANI::REACH); }
}

void RestrictionsChecker::visit(const FreeVariableExpression* exp) { throw NotSupportedException(exp->to_string(), JANI::AC); }

void RestrictionsChecker::visit(const PathExpression* exp) {
    if (exp->get_step_bounds()) { throw NotSupportedException(exp->to_string(), JANI::STEP_BOUNDS); }
    if (exp->get_time_bounds()) { throw NotSupportedException(exp->to_string(), JANI::TIME_BOUNDS); }
    if (exp->get_number_reward_bounds() > 0) { throw NotSupportedException(exp->to_string(), JANI::REWARD_BOUNDS); }
    //
    AstVisitorConst::visit(exp);
    //
    if (PathExpression::is_binary(exp->get_op())) {
        if (exp->get_op() != PathExpression::UNTIL) { throw NotSupportedException(exp->to_string(), JANI::OP); }
        if (not exp->get_left()->is_proposition() or not PLAJA_UTILS::is_derived_ptr_type<BoolValueExpression>(exp->get_left())) { throw NotSupportedException(exp->to_string(), JANI::LEFT); }
        const auto* left_value = PLAJA_UTILS::cast_ptr<Expression>(exp->get_left());
        if (!left_value->is_constant() || !left_value->evaluate_integer_const()) { throw NotSupportedException(exp->to_string(), JANI::LEFT); }
    } else {
        if (exp->get_op() != PathExpression::EVENTUALLY) { throw NotSupportedException(exp->to_string(), JANI::OP); }
    }
    JANI_ASSERT(exp->get_right())
    if (not exp->get_right()->is_proposition() or not PLAJA_UTILS::is_derived_ptr_type<Expression>(exp->get_right())) { throw NotSupportedException(exp->to_string(), JANI::RIGHT); }
}

void RestrictionsChecker::visit(const QfiedExpression* exp) {
    if (QfiedExpression::PMAX != exp->get_op() && QfiedExpression::PMIN != exp->get_op()) { throw NotSupportedException(exp->to_string(), JANI::OP); }
    //
    AstVisitorConst::visit(exp);
    //
    const auto* path_formula = exp->get_path_formula();
    JANI_ASSERT(path_formula)
    if (not PLAJA_UTILS::is_derived_ptr_type<PathExpression>(path_formula)) { throw NotSupportedException(exp->to_string(), JANI::EXPRESSION); }
}

// types

void RestrictionsChecker::visit(const BoundedType* type_) {
    if (not type_->get_lower_bound()) { throw NotSupportedException(type_->to_string(), JANI::LOWER_BOUND); }
    if (not type_->get_upper_bound()) { throw NotSupportedException(type_->to_string(), JANI::UPPER_BOUND); }
    //
    AstVisitorConst::visit(type_);
}



