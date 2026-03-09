//
// This file is part of the PlaJA code base.
// Copyright (c) (2019 - 2019) Marcel Vinzent.
// See README.md in the top-level directory for licensing information.
//

#include <unordered_set>
#include "semantics_checker.h"
#include "../ast/ast_element.h"
#include "../../exception/semantics_exception.h"
#include "extern/to_normalform.h"
#include "simple_checks_visitor.h"

#include "../ast/non_standard/free_variable_declaration.h"
#include "../ast/model.h"
#include "../ast/assignment.h"
#include "../ast/automaton.h"
#include "../ast/composition.h"
#include "../ast/composition_element.h"
#include "../ast/constant_declaration.h"
#include "../ast/destination.h"
#include "../ast/edge.h"
#include "../ast/location.h"
#include "../ast/metadata.h"
#include "../ast/property.h"
#include "../ast/property_interval.h"
#include "../ast/reward_bound.h"
#include "../ast/reward_instant.h"
#include "../ast/transient_value.h"
#include "../ast/synchronisation.h"
#include "../ast/variable_declaration.h"

#include "../ast/expression/non_standard/constant_array_access_expression.h"
#include "../ast/expression/non_standard/location_value_expression.h"
#include "../ast/expression/non_standard/objective_expression.h"
#include "../ast/expression/non_standard/predicate_abstraction_expression.h"
#include "../ast/expression/non_standard/predicates_expression.h"
#include "../ast/expression/non_standard/state_condition_expression.h"
#include "../ast/expression/non_standard/variable_value_expression.h"
#include "../ast/expression/non_standard/state_values_expression.h"
#include "../ast/expression/array_access_expression.h"
#include "../ast/expression/array_constructor_expression.h"
#include "../ast/expression/array_value_expression.h"
#include "../ast/expression/binary_op_expression.h"
#include "../ast/expression/bool_value_expression.h"
#include "../ast/expression/constant_expression.h"
#include "../ast/expression/derivative_expression.h"
#include "../ast/expression/expectation_expression.h"
#include "../ast/expression/filter_expression.h"
#include "../ast/expression/integer_value_expression.h"
#include "../ast/expression/ite_expression.h"
#include "../ast/expression/path_expression.h"
#include "../ast/expression/qfied_expression.h"
#include "../ast/expression/real_value_expression.h"
#include "../ast/expression/state_predicate_expression.h"
#include "../ast/expression/unary_op_expression.h"

#include "../ast/type/bool_type.h"
#include "../ast/type/bounded_type.h"
#include "../ast/type/int_type.h"
#include "../ast/type/real_type.h"
#include "../../exception/not_implemented_exception.h"
#include "../../exception/not_supported_exception.h"
#include "../jani_words.h"

SemanticsChecker::SemanticsChecker(const Model* model_ptr):
    model(model_ptr) { /* PLAJA_ASSERT(model) */ } // See check below.

SemanticsChecker::~SemanticsChecker() = default;

void SemanticsChecker::check_semantics(AstElement* ast_element) {
    PLAJA_ASSERT(ast_element)
    SemanticsChecker semantics_checker(PLAJA_UTILS::cast_ptr_if<Model>(ast_element)); // use model if given (non-model ast elements are allowed for testing purposes)
    ast_element->accept(&semantics_checker);
    auto* expr = PLAJA_UTILS::cast_ptr_if<PropertyExpression>(ast_element);
    if (expr) { expr->determine_type(); } // TODO for now quick fix to determine type
}

namespace SEMANTICS_CHECKER { void check_semantics(AstElement* ast_element) { SemanticsChecker::check_semantics(ast_element); } }

// structures:

void SemanticsChecker::visit(Assignment* assignment) {
    AstVisitor::visit(assignment);

    auto* value = assignment->get_value();
    auto* lower_bound = assignment->get_lower_bound();
    auto* upper_bound = assignment->get_upper_bound();

    JANI_ASSERT(value or lower_bound or upper_bound)
    JANI_ASSERT(not value or (not lower_bound and not upper_bound))

    const auto* ref_type = assignment->get_ref()->determine_type();

    if (not(ref_type->is_integer_type() or ref_type->is_boolean_type() or ref_type->is_floating_type())) { throw NotSupportedException(assignment->to_string(), JANI::TYPE); }
    if (ref_type->get_kind() == DeclarationType::Clock and (SimpleChecksVisitor::contains_clocks(value) or not SimpleChecksVisitor::is_sampling_free(value))) { throw SemanticsException(assignment->to_string()); }

    if (assignment->is_deterministic()) {
        JANI_ASSERT(value)

        if (not ref_type->is_assignable_from(*value->determine_type())) { throw SemanticsException(assignment->to_string()); }

    } else {
        PLAJA_ASSERT(assignment->is_non_deterministic())
        JANI_ASSERT(lower_bound or upper_bound)

        if (not ref_type->is_integer_type() and not ref_type->is_floating_type()) { throw SemanticsException(assignment->to_string()); }
        if (lower_bound and not ref_type->is_assignable_from(*lower_bound->determine_type())) { throw SemanticsException(assignment->to_string()); }
        if (upper_bound and not ref_type->is_assignable_from(*upper_bound->determine_type())) { throw SemanticsException(assignment->to_string()); }

    }
}

void SemanticsChecker::visit(Automaton* automaton) {
    AstVisitor::visit(automaton);
    // restrict-initial: // only difference to base class implementation
    auto* restrict_initial = automaton->get_restrictInitialExpression();
    if (restrict_initial) {
        if (not restrict_initial->determine_type()->is_boolean_type() or SimpleChecksVisitor::contains_transients(restrict_initial, *model)) {
            throw SemanticsException(automaton->to_string());
        }
    }

}

void SemanticsChecker::visit(ConstantDeclaration* const_decl) {
    AstVisitor::visit(const_decl);
    auto* value = const_decl->get_value();
    if (value) {
        if (not(const_decl->get_type()->is_assignable_from(*value->determine_type()) and value->is_constant())) { throw SemanticsException(const_decl->to_string()); }
    }
}

void SemanticsChecker::visit(Destination* destination) {
    AstVisitor::visit(destination);
    auto* probability = destination->get_probabilityExpression();
    if (probability) {
        if (not RealType::assignable_from(*probability->determine_type())) { throw SemanticsException(destination->to_string()); }
    }
}

void SemanticsChecker::visit(Edge* edge) {
    AstVisitor::visit(edge);
    // rate:
    auto* tmp = edge->get_rateExpression();
    if (tmp) {
        if (not RealType::assignable_from(*tmp->determine_type())) { throw SemanticsException(edge->to_string()); }
    }
    // guard:
    tmp = edge->get_guardExpression();
    if (tmp and not tmp->determine_type()->is_boolean_type()) { throw SemanticsException(edge->to_string()); }
}

void SemanticsChecker::visit(Location* loc) {
    AstVisitor::visit(loc);
    auto* time_progress_cond = loc->get_timeProgressCondition();
    if (time_progress_cond) {
        if (not time_progress_cond->determine_type()->is_boolean_type()) { throw SemanticsException(loc->to_string()); }
    }
}

void SemanticsChecker::visit(Model* model_) {
    AstVisitor::visit(model_);
    // restrict-initial: // only difference to base class implementation
    auto* restrict_initial = model_->get_restrict_initial_expression();
    if (restrict_initial) {
        if (not restrict_initial->determine_type()->is_boolean_type() or SimpleChecksVisitor::contains_transients(restrict_initial, *model)) {
            throw SemanticsException(model_->to_string());
        }
    }
}

void SemanticsChecker::visit(PropertyInterval* property_interval) {
    AstVisitor::visit(property_interval);
    auto* tmp = property_interval->get_lower();
    if (tmp and not tmp->is_constant()) { throw SemanticsException(property_interval->to_string()); }
    tmp = property_interval->get_upper();
    if (tmp and not tmp->is_constant()) { throw SemanticsException(property_interval->to_string()); }
}

void SemanticsChecker::visit(RewardBound* /*rewardBound*/) { throw NotImplementedException(__PRETTY_FUNCTION__); }

void SemanticsChecker::visit(RewardInstant* /*rewardInstant*/) { throw NotImplementedException(__PRETTY_FUNCTION__); }

void SemanticsChecker::visit(TransientValue* trans_val) {
    PLAJA_ASSERT(model)
    AstVisitor::visit(trans_val);
    auto* l_value = trans_val->get_ref();
    // Check for array assignment
    const auto* index = l_value->get_array_index();
    if (index) { // array assignment
        if (SimpleChecksVisitor::contains_transients(index, *model) or SimpleChecksVisitor::contains_clocks(index) or SimpleChecksVisitor::contains_continuous(index)) { throw SemanticsException(trans_val->to_string()); }
    }
    // conversely the assigned variable must be transient:
    if (not model->get_variable_by_id(l_value->get_variable_id())->is_transient()) { throw SemanticsException(trans_val->to_string()); }
    // May not reference transient variables, or clocks, or continuous.
    auto* value = trans_val->get_value();
    if (SimpleChecksVisitor::contains_transients(value, *model) or SimpleChecksVisitor::contains_clocks(value) or SimpleChecksVisitor::contains_continuous(value)) { throw SemanticsException(trans_val->to_string()); }
    // Basic type check:
    if (not l_value->determine_type()->is_assignable_from(*value->determine_type())) { throw SemanticsException(trans_val->to_string()); }
}

void SemanticsChecker::visit(VariableDeclaration* var_decl) {
    AstVisitor::visit(var_decl);
    // type:
    auto* type = var_decl->get_type();
    if (var_decl->is_transient()) {
        if (SimpleChecksVisitor::contains_clocks(type) or SimpleChecksVisitor::contains_continuous(type)) { throw SemanticsException(var_decl->to_string()); }
    }
    // value
    auto* value = var_decl->get_initialValue();
    if (value) {
        if (not(type->is_assignable_from(*value->determine_type()) and value->is_constant())) { throw SemanticsException(var_decl->to_string()); }
    }
}

// expressions:

void SemanticsChecker::visit(ArrayAccessExpression* exp) {
    AstVisitor::visit(exp);
    if (DeclarationType::Array != exp->get_accessedArray()->determine_type()->get_kind()) { throw SemanticsException(exp->to_string()); }
    if (not IntType::assignable_from(*exp->get_index()->determine_type())) { throw SemanticsException(exp->to_string()); }

    /* If (model is given and) array index is constant we can check statically whether it violates the array bounds. */
    /* Note: this code relies on the restrictions to ArrayValueExpression. */
    if (model and exp->get_index()->is_constant()) {
        const PLAJA::integer index_val = exp->get_index()->evaluate_integer_const();
        const auto* var_decl = model->get_variable_by_id(exp->get_variable_id());
        const auto* init_exp = PLAJA_UTILS::cast_ptr<ArrayValueExpression>(var_decl->get_initialValue());
        auto array_size = init_exp->get_number_elements();
        if (index_val < 0 or index_val >= array_size) { throw SemanticsException(exp->to_string()); }
    }

}

void SemanticsChecker::visit(ArrayConstructorExpression* exp) {
    AstVisitor::visit(exp);
    if (not IntType::assignable_from(*exp->get_length()->determine_type())) { throw SemanticsException(exp->to_string()); }
}

void SemanticsChecker::visit(ArrayValueExpression* exp) {
    AstVisitor::visit(exp);
    exp->determine_type(); // determine_type checks if there is a common type
}

void SemanticsChecker::visit(BinaryOpExpression* exp) {
    AstVisitor::visit(exp);

    switch (exp->get_op()) {
        case BinaryOpExpression::OR:
        case BinaryOpExpression::AND:
        case BinaryOpExpression::IMPLIES: {
            if (not(exp->get_left()->determine_type()->is_boolean_type() and exp->get_right()->determine_type()->is_boolean_type())) { throw SemanticsException(exp->to_string()); }
            break;
        }
        case BinaryOpExpression::EQ:
        case BinaryOpExpression::NE: {
            if (not exp->get_left()->determine_type()->infer_common(*exp->get_right()->determine_type())) { throw SemanticsException(exp->to_string()); } // assignable to some common type
            // const auto* left_type = exp->get_left()->determine_type();
            // const auto* right_type = exp->get_right()->determine_type();
            // if (not ((exp->get_left()->determine_type()->is_boolean_type() and exp->get_right()->determine_type()->is_boolean_type()) or (left_type->is_integer_type() and right_type->is_integer_type()))) { throw NotSupportedException(exp->to_string()); } // equality on floating not supported
            break;
        }
        case BinaryOpExpression::LT:
        case BinaryOpExpression::LE:
        case BinaryOpExpression::GT:
        case BinaryOpExpression::GE: {
            const auto* left_type = exp->get_left()->determine_type();
            const auto* right_type = exp->get_right()->determine_type();
            if (not(left_type->is_numeric_type() and right_type->is_numeric_type())) { throw SemanticsException(exp->to_string()); }
            break;
        }
        case BinaryOpExpression::PLUS:
        case BinaryOpExpression::MINUS:
        case BinaryOpExpression::TIMES:
        case BinaryOpExpression::DIV:
        case BinaryOpExpression::POW:
        case BinaryOpExpression::LOG:
        case BinaryOpExpression::MIN:
        case BinaryOpExpression::MAX: {
            if (not(exp->get_left()->determine_type()->is_numeric_type() and exp->get_right()->determine_type()->is_numeric_type())) { throw SemanticsException(exp->to_string()); }
            break;
        }
        case BinaryOpExpression::MOD: {
            if (not(exp->get_left()->determine_type()->is_integer_type() and exp->get_right()->determine_type()->is_integer_type())) { throw SemanticsException(exp->to_string()); }
            break;
        }
    }

}

void SemanticsChecker::visit(DerivativeExpression* exp) {
    JANI_ASSERT(exp->determine_type())
    if (DeclarationType::Continuous != exp->determine_type()->get_kind()) { throw SemanticsException(exp->to_string()); }
}

void SemanticsChecker::visit(DistributionSamplingExpression* /*exp*/) { throw NotImplementedException(__PRETTY_FUNCTION__); }

void SemanticsChecker::visit(ExpectationExpression* exp) {
    AstVisitor::visit(exp);
    // value:
    auto* value = exp->get_value();
    JANI_ASSERT(value)
    if (not value->determine_type()->is_numeric_type()) { throw SemanticsException(exp->to_string()); }
    PLAJA_ASSERT(value->is_proposition())
    // accumulate: // Nothing to check.
    // reach:
    {
        auto* reach = exp->get_reach();
        if (reach) {
            const auto* type = reach->determine_type();
            if (not(type and type->is_boolean_type())) { throw SemanticsException(exp->to_string()); }
            if (not reach->is_proposition()) { throw NotImplementedException(__PRETTY_FUNCTION__); }
        } else { throw NotImplementedException(__PRETTY_FUNCTION__); }
    }
    // step-instant:
    // time-instant:
    // reward-instants:
    if (exp->get_step_instant() or exp->get_time_instant() or exp->get_number_reward_instants()) { throw NotImplementedException(__PRETTY_FUNCTION__); }

}

void SemanticsChecker::visit(FilterExpression* exp) {
    AstVisitor::visit(exp);
    // states:
    const auto* type = exp->get_states()->determine_type();
    if (not(type and type->is_boolean_type())) { throw SemanticsException(exp->to_string()); }
    // values:
    switch (exp->get_fun()) {
        case FilterExpression::MIN:
        case FilterExpression::MAX:
        case FilterExpression::SUM:
        case FilterExpression::AVG:
        case FilterExpression::ARGMIN:
        case FilterExpression::ARGMAX: {
            const auto* values_type = exp->get_values()->determine_type();
            if (values_type == nullptr or not RealType::assignable_from(*values_type)) { throw SemanticsException(exp->to_string()); }
            break;
        }
        case FilterExpression::COUNT:
        case FilterExpression::FORALL:
        case FilterExpression::EXISTS: {
            const auto* values_type = exp->get_values()->determine_type();
            if (not(values_type and values_type->is_boolean_type())) { throw SemanticsException(exp->to_string()); }
            break;
        }
        case FilterExpression::VALUES: {
            const auto* values_type = exp->get_values()->determine_type();
            if (not values_type) { throw SemanticsException(exp->to_string()); }
            if (not(RealType::assignable_from(*values_type) or values_type->is_boolean_type())) { throw SemanticsException(exp->to_string()); }
            break;
        }
    }

}

void SemanticsChecker::visit(ITE_Expression* exp) {
    AstVisitor::visit(exp);
    // condition:
    if (not exp->get_condition()->determine_type()->is_boolean_type()) { throw SemanticsException(exp->to_string()); }
    // consequence & alternative:
    const auto* con_type = exp->get_consequence()->determine_type();
    const auto* alt_type = exp->get_alternative()->determine_type();
    if (not(con_type->is_assignable_from(*alt_type) or alt_type->is_assignable_from(*con_type))) { throw SemanticsException(exp->to_string()); }
}

void SemanticsChecker::visit(PathExpression* exp) {
    AstVisitor::visit(exp);
    if (PathExpression::is_binary(exp->get_op())) {
        auto* left = exp->get_left();
        JANI_ASSERT(left)
        const auto* type = left->determine_type();
        if (not(type and type->is_boolean_type())) { throw SemanticsException(exp->to_string()); }
        if (not left->is_proposition()) { throw NotImplementedException(__PRETTY_FUNCTION__); }
    }
    auto* right = exp->get_right();
    JANI_ASSERT(right)
    const auto* type = right->determine_type();
    if (not(type and type->is_boolean_type())) { throw SemanticsException(exp->to_string()); }
    if (not right->is_proposition()) { throw NotImplementedException(__PRETTY_FUNCTION__); }
    // step-instant:
    // time-instant:
    // reward-instants:
    if (exp->get_step_bounds() or exp->get_time_bounds() or exp->get_number_reward_bounds()) { throw NotImplementedException(__PRETTY_FUNCTION__); }
}

void SemanticsChecker::visit(QfiedExpression* exp) {
    if (QfiedExpression::PMAX != exp->get_op() and QfiedExpression::PMIN != exp->get_op()) { throw NotImplementedException(__PRETTY_FUNCTION__); }
    AstVisitor::visit(exp);
    auto* path_formula = exp->get_path_formula();
    JANI_ASSERT(path_formula)
    const auto* type = path_formula->determine_type();
    if (not(type and type->is_boolean_type())) { throw SemanticsException(exp->to_string()); }
    if (not PLAJA_UTILS::is_derived_ptr_type<PathExpression>(path_formula)) { throw NotImplementedException(__PRETTY_FUNCTION__); }
}

void SemanticsChecker::visit(UnaryOpExpression* exp) {
    AstVisitor::visit(exp);
    exp->get_operand()->determine_type(); // currently we assume that at runtime resultType is set, i.p. for evaluateFloating in BinaryOpExpression
    switch (exp->get_op()) {
        case UnaryOpExpression::NOT: {
            if (not exp->get_operand()->determine_type()->is_boolean_type()) { throw SemanticsException(exp->to_string()); }
            break;
        }
        case UnaryOpExpression::FLOOR:
        case UnaryOpExpression::CEIL:
        case UnaryOpExpression::ABS:
        case UnaryOpExpression::SGN:
        case UnaryOpExpression::TRC: {
            if (not(exp->get_operand()->determine_type()->is_numeric_type())) { throw SemanticsException(exp->to_string()); }
            break;
        }
    }
}

// non-standard:

void SemanticsChecker::visit(ConstantArrayAccessExpression* exp) {
    AstVisitor::visit(exp);
    if (DeclarationType::Array != exp->get_accessed_array()->get_type()->get_kind()) { throw SemanticsException(exp->to_string()); }
    if (not IntType::assignable_from(*exp->get_index()->determine_type())) { throw SemanticsException(exp->to_string()); }

    /* If array index is constant we can check statically whether it violates the array bounds. */
    /* Note: this code relies on the restrictions to ArrayValueExpression. */
    if (exp->get_index()->is_constant()) {
        const PLAJA::integer index_val = exp->get_index()->evaluate_integer_const();
        const auto* init_exp = PLAJA_UTILS::cast_ptr<ArrayValueExpression>(exp->get_accessed_array()->get_value());
        if (index_val < 0 or init_exp->get_number_elements() < index_val) { throw SemanticsException(exp->to_string()); }
    }
}

void SemanticsChecker::visit(ObjectiveExpression* exp) {
    AstVisitor::visit(exp);
    auto* goal = exp->get_goal();
    if (goal) {
        PLAJA_ASSERT_EXPECT(not TO_NORMALFORM::is_states_values(*goal)) // Excluded at parse time. Should be support by explicit engines though ...
        const auto* type = goal->determine_type();
        if (not(type and type->is_boolean_type())) { throw SemanticsException(exp->to_string()); }
    }
    auto* goal_potential = exp->get_goal_potential();
    if (goal_potential) {
        const auto* reward_type = goal_potential->determine_type();
        if (not reward_type or (not reward_type->is_integer_type() and reward_type->get_kind() != DeclarationType::Real)) { throw SemanticsException(exp->to_string()); } // int or real
    }
    auto* step_reward = exp->get_step_reward();
    if (step_reward) {
        const auto* reward_type = step_reward->determine_type();
        if (not reward_type or (not reward_type->is_integer_type() and reward_type->get_kind() != DeclarationType::Real)) { throw SemanticsException(exp->to_string()); } // int or real
    }
}

void SemanticsChecker::visit(PAExpression* exp) {
    AstVisitor::visit(exp);

    {
        auto* start = exp->get_start();
        if (start and not TO_NORMALFORM::is_states_values(*start)) {
            // Additional check for state condition expression: location assignments must be *complete*.
            auto state_condition = StateConditionExpression::transform(start->deepCopy_Exp());
            PLAJA_ASSERT(model)
            if (state_condition->get_size_loc_values() != model->get_number_automataInstances()) {
                throw SemanticsException(start->to_string()); // TODO ? more informative, move somewhere else ?
            }

        }
    }

    if (exp->get_reach()) {
        auto* reach = exp->get_reach();
        PLAJA_ASSERT_EXPECT(not TO_NORMALFORM::is_states_values(*reach)) // Excluded at parse time. Should be support by explicit engines though ...
        const auto* type = reach->determine_type();
        if (not(type and type->is_boolean_type())) { throw SemanticsException(exp->to_string()); }
        PLAJA_ASSERT(reach->is_proposition()) // redundant check, any expression is a proposition
    }

    if (exp->get_objective()) { exp->get_objective()->determine_type(); }

}

void SemanticsChecker::visit(PredicatesExpression* exp) {
    AstVisitor::visit(exp);

    for (auto it = exp->predicatesIterator(); !it.end(); ++it) {
        const auto* type = it->determine_type();
        if (not(type and type->is_boolean_type())) { throw SemanticsException(exp->to_string()); }
        PLAJA_ASSERT(it->is_proposition()) // redundant check, any expression is a proposition
    }
}

void SemanticsChecker::visit(StateConditionExpression* exp) {
    AstVisitor::visit(exp);

    // automaton location values:
    std::unordered_set<AutomatonIndex_type> assigned_automata_locations; // to check uniqueness
    for (auto it = exp->init_loc_value_iterator(); !it.end(); ++it) {
        if (not assigned_automata_locations.insert(it->get_location_index()).second) { throw SemanticsException(exp->to_string()); }
    }
    // state variable constraint:
    if (exp->get_constraint()) {
        auto* constraint = exp->get_constraint();
        const auto* type = constraint->determine_type();
        if (not(type and type->is_boolean_type())) { throw SemanticsException(exp->to_string()); }
        PLAJA_ASSERT(constraint->is_proposition()) // redundant check, any expression is a proposition
    }
}

void SemanticsChecker::visit(VariableValueExpression* exp) {
    AstVisitor::visit(exp);

    // type inference:
    auto* var = exp->get_var();
    auto* value = exp->get_val();
    const auto* type_var = var->determine_type();
    const auto* type_val = value->determine_type();
    if (not type_var->is_assignable_from(*type_val)) { throw SemanticsException(exp->to_string()); }

    // constant value:
    if (not value->is_constant()) { throw SemanticsException(exp->to_string()); }
    // additional for arrays:
    const auto* index_exp = var->get_array_index();
    if (index_exp and not index_exp->is_constant()) { throw SemanticsException(exp->to_string()); }

    // support
    if (not(type_var->is_floating_type() or type_var->is_integer_type() or type_var->is_boolean_type())) { throw NotSupportedException(exp->to_string(), JANI::TYPE); }
}

void SemanticsChecker::visit(StateValuesExpression* exp) {
    AstVisitor::visit(exp);

    // location values:
    std::unordered_set<AutomatonIndex_type> assigned_locations; // to check uniqueness
    for (auto it = exp->init_loc_value_iterator(); !it.end(); ++it) {
        if (not assigned_locations.insert(it->get_location_index()).second) { throw SemanticsException(exp->to_string()); }
    }

    // variable values:
    std::unordered_set<const LValueExpression*> assigned_variables; // to check uniqueness
    for (auto it = exp->init_var_value_iterator(); !it.end(); ++it) {
        if (not assigned_variables.insert(it->get_var()).second) { throw SemanticsException(exp->to_string()); }
    }
}

// types:

void SemanticsChecker::visit(BoundedType* type) {
    AstVisitor::visit(type);

    auto* basic_type = type->get_base();
    JANI_ASSERT(basic_type)

    auto* lower_bound = type->get_lower_bound();
    if (lower_bound) {
        if (not basic_type->is_assignable_from(*lower_bound->determine_type()) or not lower_bound->is_constant()) {
            throw SemanticsException(type->to_string());
        }
    }

    auto* upper_bound = type->get_upper_bound();
    if (upper_bound) {
        if (not basic_type->is_assignable_from(*upper_bound->determine_type()) or not upper_bound->is_constant()) {
            throw SemanticsException(type->to_string());
        }
    }

    // lowerBound < upperBound is checked when computing model information, as constant may not have been initialised at the current point
}















