//
// This file is part of the PlaJA code base.
// Copyright (c) (2019 - 2019) Marcel Vinzent.
// See README.md in the top-level directory for licensing information.
//

#include <json.hpp>
#include "to_json_visitor.h"
#include "../../../utils/utils.h"

#include "../../ast/iterators/model_iterator.h"
#include "../../ast/non_standard/free_variable_declaration.h"
#include "../../ast/non_standard/properties.h"
#include "../../ast/non_standard/reward_accumulation.h"
#include "../../ast/action.h"
#include "../../ast/assignment.h"
#include "../../ast/automaton.h"
#include "../../ast/composition.h"
#include "../../ast/composition_element.h"
#include "../../ast/constant_declaration.h"
#include "../../ast/destination.h"
#include "../../ast/edge.h"
#include "../../ast/location.h"
#include "../../ast/metadata.h"
#include "../../ast/property.h"
#include "../../ast/property_interval.h"
#include "../../ast/reward_bound.h"
#include "../../ast/reward_instant.h"
#include "../../ast/transient_value.h"
#include "../../ast/synchronisation.h"
#include "../../ast/variable_declaration.h"

#include "../../ast/expression/non_standard/constant_array_access_expression.h"
#include "../../ast/expression/non_standard/let_expression.h"
#include "../../ast/expression/non_standard/location_value_expression.h"
#include "../../ast/expression/non_standard/objective_expression.h"
#include "../../ast/expression/non_standard/predicate_abstraction_expression.h"
#include "../../ast/expression/non_standard/predicates_expression.h"
#include "../../ast/expression/non_standard/problem_instance_expression.h"
#include "../../ast/expression/non_standard/state_condition_expression.h"
#include "../../ast/expression/non_standard/variable_value_expression.h"
#include "../../ast/expression/non_standard/state_values_expression.h"
#include "../../ast/expression/non_standard/states_values_expression.h"
#include "../../ast/expression/special_cases/linear_expression.h"
#include "../../ast/expression/special_cases/nary_expression.h"
#include "../../ast/expression/array_access_expression.h"
#include "../../ast/expression/array_constructor_expression.h"
#include "../../ast/expression/array_value_expression.h"
#include "../../ast/expression/bool_value_expression.h"
#include "../../ast/expression/constant_expression.h"
#include "../../ast/expression/derivative_expression.h"
#include "../../ast/expression/distribution_sampling_expression.h"
#include "../../ast/expression/expectation_expression.h"
#include "../../ast/expression/filter_expression.h"
#include "../../ast/expression/free_variable_expression.h"
#include "../../ast/expression/integer_value_expression.h"
#include "../../ast/expression/ite_expression.h"
#include "../../ast/expression/path_expression.h"
#include "../../ast/expression/qfied_expression.h"
#include "../../ast/expression/real_value_expression.h"
#include "../../ast/expression/state_predicate_expression.h"
#include "../../ast/expression/unary_op_expression.h"

#include "../../ast/type/array_type.h"
#include "../../ast/type/bool_type.h"
#include "../../ast/type/bounded_type.h"
#include "../../ast/type/clock_type.h"
#include "../../ast/type/continuous_type.h"
#include "../../ast/type/int_type.h"
#include "../../ast/type/real_type.h"

#include "../../jani_words.h"

std::unique_ptr<nlohmann::json> ToJSON_Visitor::to_json(const AstElement& ast_element, bool use_instances) {
    ToJSON_Visitor toJsonVisitor(use_instances);
    ast_element.accept(&toJsonVisitor);
    return std::move(toJsonVisitor.j_rlt);
}

std::string ToJSON_Visitor::to_string(const AstElement& ast_element, bool use_instances, bool intend) {
    return to_json(ast_element, use_instances)->dump(intend ? 4 : -1);
}

void ToJSON_Visitor::dump(const AstElement& ast_element, bool use_instances, bool intend) {
    std::cout << to_json(ast_element, use_instances)->dump(intend ? 4 : -1) << std::endl;
}

void ToJSON_Visitor::write_to_file(const std::string& filename, const AstElement& ast_element, bool use_instances, bool intend, bool trunc) {
    // inline functions above due to usage of targetDirectory
    ToJSON_Visitor to_json_visitor(use_instances);
    auto target_dir = PLAJA_UTILS::extract_directory_path(filename);
    to_json_visitor.targetDirectory = &target_dir;
    ast_element.accept(&to_json_visitor);
    PLAJA_UTILS::write_to_file(filename, to_json_visitor.j_rlt->dump(intend ? 4 : -1), trunc);
}

// auxiliaries:

#define AST_CONST_ITERATOR(IT, AST_ELEMENT_TYPE) \
         for (auto it = IT; !it.end(); ++it) { it->accept(this); }

inline void ToJSON_Visitor::accept(const AstElement* element, const std::string& key, nlohmann::json& j) {
    PLAJA_ASSERT(element)
    element->accept(this);
    j.emplace(key, std::move(*j_rlt));
}

inline void ToJSON_Visitor::accept_if(const AstElement* element, const std::string& key, nlohmann::json& j) {
    if (element) { accept(element, key, j); }
}

void ToJSON_Visitor::accept_exp_comment_if(const AstElement* exp, const std::string& comment, const std::string& key, nlohmann::json& j) {
    if (exp) {
        nlohmann::json j_sub;
        accept(exp, JANI::EXP, j_sub);
        if (!comment.empty()) { j_sub.emplace(JANI::COMMENT, comment); }
        j.emplace(key, std::move(j_sub));
    }
}

inline void ToJSON_Visitor::accept_comment(const Commentable* element, nlohmann::json& j) {
    const std::string& comment = element->get_comment();
    if (!comment.empty()) { j.emplace(JANI::COMMENT, comment); } // this will ignore "empty comments"
}

namespace TO_JSON_VISITOR {

    struct ToJsonIterator {

        template<typename AstElement_t>
        static inline void accept_iterator(ToJSON_Visitor& visitor, AstConstIterator<AstElement_t> it, const std::string& key, nlohmann::json& j) {
            nlohmann::json j_sub;
            for (; !it.end(); ++it) {
                it->accept(&visitor);
                j_sub.push_back(std::move(*visitor.j_rlt));
            }
            j.emplace(key, std::move(j_sub));
        }

    };

}

// for better readability:
#define JSON_CONSTRUCT std::unique_ptr<nlohmann::json> j(new nlohmann::json());
#define JSON_EMPLACE(KEY, VALUE) j->emplace(KEY, VALUE);
#define JSON_ACCEPT_IF(ELEMENT, KEY) accept_if(ELEMENT, KEY, *j);
#define JSON_ACCEPT_EXP_COMMENT_IF(EXP, COMMENT, KEY) accept_exp_comment_if(EXP, COMMENT, KEY, *j);
#define JSON_ACCEPT_COMMENT(ELEMENT) accept_comment(ELEMENT, *j);
#define JSON_ITERATOR(ITERATOR, KEY) TO_JSON_VISITOR::ToJsonIterator::accept_iterator(*this, ITERATOR, KEY, *j);
#define JSON_ITERATOR_IF(ITERATOR, KEY, CONDITION) if (CONDITION) { JSON_ITERATOR(ITERATOR, KEY) }
#define JSON_RETURN j_rlt = std::move(j);

// instance:

ToJSON_Visitor::ToJSON_Visitor(bool use_instances):
    targetDirectory(nullptr)
    , useInstances(use_instances)
    , j_rlt()/*, model_ref(nullptr), automaton_ref(nullptr)*/ {}

ToJSON_Visitor::~ToJSON_Visitor() = default;

//

void ToJSON_Visitor::visit(const Action* action) {
    JSON_CONSTRUCT
    JSON_EMPLACE(JANI::NAME, action->get_name())
    JSON_ACCEPT_COMMENT(action)
    JSON_RETURN
}

void ToJSON_Visitor::visit(const Assignment* assignment) {
    JSON_CONSTRUCT
    JSON_ACCEPT_IF(assignment->get_ref(), JANI::REF)
    JSON_ACCEPT_IF(assignment->get_value(), JANI::VALUE)
    JSON_ACCEPT_IF(assignment->get_lower_bound(), JANI::LOWER_BOUND)
    JSON_ACCEPT_IF(assignment->get_upper_bound(), JANI::UPPER_BOUND)
    JSON_EMPLACE(JANI::INDEX, assignment->get_index()) // default 0 if not explicitly stated in the input
    JSON_ACCEPT_COMMENT(assignment)
    JSON_RETURN
}

void ToJSON_Visitor::visit(const Automaton* automaton) {
    // automaton_ref = automaton;
    JSON_CONSTRUCT
    JSON_EMPLACE(JANI::NAME, automaton->get_name())
    // variables
    JSON_ITERATOR_IF(automaton->variableIterator(), JANI::VARIABLES, automaton->get_number_variables())
    // restrict-initial
    JSON_ACCEPT_EXP_COMMENT_IF(automaton->get_restrictInitialExpression(), automaton->get_restrictInitialComment(), JANI::RESTRICT_INITIAL)
    // LOCATIONS
    JSON_ITERATOR_IF(automaton->locationIterator(), JANI::LOCATIONS, automaton->get_number_locations())
    // initial locations
    if (automaton->get_number_initialLocations()) {
        nlohmann::json j_sub;
        for (const auto& loc_id: automaton->_initial_locations()) { j_sub.push_back(automaton->get_location(loc_id)->get_name()); }
        j->emplace(JANI::INITIAL_LOCATIONS, std::move(j_sub));
    }
    // edges
    JSON_ITERATOR_IF(automaton->edgeIterator(), JANI::EDGES, automaton->get_number_edges())
    //
    JSON_ACCEPT_COMMENT(automaton)
    JSON_RETURN
}

void ToJSON_Visitor::visit(const Composition* sys) {
    JSON_CONSTRUCT
    JSON_ITERATOR_IF(sys->elementIterator(), JANI::ELEMENTS, sys->get_number_elements())
    JSON_ITERATOR_IF(sys->syncIterator(), JANI::SYNCS, sys->get_number_syncs())
    JSON_ACCEPT_COMMENT(sys)
    JSON_RETURN
}

void ToJSON_Visitor::visit(const CompositionElement* sysEl) {
    JSON_CONSTRUCT
    JSON_EMPLACE(JANI::AUTOMATON, sysEl->get_elementAutomaton())
    // input-enable
    if (sysEl->get_number_inputEnables()) {
        nlohmann::json j_sub;
        for (const auto& input_enable: sysEl->_input_enables()) { j_sub.push_back(input_enable.first); }
        j->emplace(JANI::INPUT_ENABLE, std::move(j_sub));
    }
    // comment
    JSON_ACCEPT_COMMENT(sysEl)
    JSON_RETURN
}

void ToJSON_Visitor::visit(const ConstantDeclaration* const_decl) {
    JSON_CONSTRUCT
    JSON_EMPLACE(JANI::NAME, const_decl->get_name())
    JSON_ACCEPT_IF(const_decl->get_type(), JANI::TYPE)
    JSON_ACCEPT_IF(const_decl->get_value(), JANI::VALUE)
    JSON_ACCEPT_COMMENT(const_decl)
    JSON_RETURN
}

void ToJSON_Visitor::visit(const Destination* destination) {
    JSON_CONSTRUCT
    JSON_EMPLACE(JANI::LOCATION, destination->get_location_name())
    JSON_ACCEPT_EXP_COMMENT_IF(destination->get_probabilityExpression(), destination->get_comment(), JANI::PROBABILITY)
    JSON_ITERATOR_IF(destination->assignmentIterator(), JANI::ASSIGNMENTS, destination->get_number_assignments())
    JSON_ACCEPT_COMMENT(destination)
    JSON_RETURN
}

void ToJSON_Visitor::visit(const Edge* edge) {
    JSON_CONSTRUCT
    JSON_EMPLACE(JANI::LOCATION, edge->get_location_name())
    // action
    if (edge->get_action()) { JSON_EMPLACE(JANI::ACTION, edge->get_action_name()) }
    //
    JSON_ACCEPT_EXP_COMMENT_IF(edge->get_rateExpression(), edge->get_rateComment(), JANI::RATE)
    JSON_ACCEPT_EXP_COMMENT_IF(edge->get_guardExpression(), edge->get_guardComment(), JANI::GUARD)
    JSON_ITERATOR_IF(edge->destinationIterator(), JANI::DESTINATIONS, edge->get_number_destinations())
    JSON_ACCEPT_COMMENT(edge)
    JSON_RETURN
}

void ToJSON_Visitor::visit(const Location* loc) {
    JSON_CONSTRUCT
    JSON_EMPLACE(JANI::NAME, loc->get_name())
    JSON_ACCEPT_EXP_COMMENT_IF(loc->get_timeProgressCondition(), loc->get_timeProgressComment(), JANI::TIME_PROGRESS)
    JSON_ITERATOR_IF(loc->transientValueIterator(), JANI::TRANSIENT_VALUES, loc->get_number_transientValues())
    JSON_ACCEPT_COMMENT(loc)
    JSON_RETURN
}

void ToJSON_Visitor::visit(const Metadata* metadata) {
    JSON_CONSTRUCT
    JSON_EMPLACE(JANI::VERSION, metadata->getVersion())
    JSON_EMPLACE(JANI::AUTHOR, metadata->getAuthor())
    JSON_EMPLACE(JANI::DESCRIPTION, metadata->getDescription())
    JSON_EMPLACE(JANI::DOI, metadata->getDoi())
    JSON_EMPLACE(JANI::URL, metadata->getUrl())
    JSON_RETURN
}

void ToJSON_Visitor::visit(const Model* model) {
    // model_ref = model;
    JSON_CONSTRUCT
    JSON_EMPLACE(JANI::JANI_VERSION, model->get_jani_version())
    JSON_EMPLACE(JANI::NAME, model->get_name())
    JSON_ACCEPT_IF(model->get_metadata(), JANI::METADATA)
    JSON_EMPLACE(JANI::TYPE, Model::model_type_to_str(model->get_model_type()))
    // features
    if (model->get_number_model_features()) {
        nlohmann::json j_sub;
        for (const auto& feature: model->get_model_features()) { j_sub.push_back(Model::model_feature_to_str(feature)); }
        j->emplace(JANI::FEATURES, std::move(j_sub));
    }
    // actions
    JSON_ITERATOR_IF(ModelIterator::actionIterator(*model), JANI::ACTIONS, model->get_number_actions())
    // constants
    JSON_ITERATOR_IF(ModelIterator::constantIterator(*model), JANI::CONSTANTS, model->get_number_constants())
    // variables
    JSON_ITERATOR_IF(ModelIterator::globalVariableIterator(*model), JANI::VARIABLES, model->get_number_variables())
    // restrict initial
    JSON_ACCEPT_EXP_COMMENT_IF(model->get_restrict_initial_expression(), model->get_restrict_initial_comment(), JANI::RESTRICT_INITIAL)
    // properties
    JSON_ITERATOR_IF(ModelIterator::propertyIterator(*model), JANI::PROPERTIES, model->get_number_properties())
    // system
    JSON_ACCEPT_IF(model->get_system(), JANI::SYSTEM)
    // automata
    if (useInstances) {
        JSON_ITERATOR_IF(ModelIterator::automatonInstanceIterator(*model), JANI::AUTOMATA, model->get_number_automataInstances())
    } else {
        std::size_t l;
        if ((l = model->get_number_automata())) {
            nlohmann::json j_sub;
            for (std::size_t i = 0; i < l; ++i) {
                model->get_automaton(i)->accept(this);
                j_sub.push_back(std::move(*j_rlt));
            }
            j->emplace(JANI::AUTOMATA, std::move(j_sub));
        }
    }

    JSON_RETURN
}

void ToJSON_Visitor::visit(const Property* prop) {
    JSON_CONSTRUCT
    JSON_EMPLACE(JANI::NAME, prop->get_name())
    JSON_ACCEPT_IF(prop->get_propertyExpression(), JANI::EXPRESSION)
    JSON_ACCEPT_COMMENT(prop)
    JSON_RETURN
}

void ToJSON_Visitor::visit(const PropertyInterval* propertyInterval) {
    JSON_CONSTRUCT
    // lower
    if (propertyInterval->get_lower()) {
        propertyInterval->get_lower()->accept(this);
        j->emplace(JANI::LOWER, std::move(*j_rlt));
        j->emplace(JANI::LOWER_EXCLUSIVE, propertyInterval->is_lowerExclusive());
    }
    // upper
    if (propertyInterval->get_upper()) {
        propertyInterval->get_upper()->accept(this);
        j->emplace(JANI::UPPER, std::move(*j_rlt));
        j->emplace(JANI::UPPER_EXCLUSIVE, propertyInterval->is_upperExclusive());
    }
    JSON_RETURN
}

void ToJSON_Visitor::visit(const RewardBound* rewardBound) {
    JSON_CONSTRUCT
    JSON_ACCEPT_IF(rewardBound->get_accumulation_expression(), JANI::EXP)
    JSON_ACCEPT_IF(rewardBound->get_reward_accumulation(), JANI::ACCUMULATE)
    JSON_ACCEPT_IF(rewardBound->get_bounds(), JANI::BOUNDS)
    JSON_RETURN
}

void ToJSON_Visitor::visit(const RewardInstant* rewardInstant) {
    JSON_CONSTRUCT
    JSON_ACCEPT_IF(rewardInstant->get_accumulation_expression(), JANI::EXP)
    JSON_ACCEPT_IF(rewardInstant->get_reward_accumulation(), JANI::ACCUMULATE)
    JSON_ACCEPT_IF(rewardInstant->get_instant(), JANI::INSTANT)
    //
    JSON_RETURN
}

void ToJSON_Visitor::visit(const Synchronisation* sync) {
    JSON_CONSTRUCT
    // synchronise
    if (sync->get_size_synchronise()) {
        nlohmann::json j_sub;
        for (const auto& [name, id]: sync->_synchronise()) {
            if (id == ACTION::nullAction) { j_sub.push_back(nullptr); }
            else { j_sub.push_back(name); }
        }
        j->emplace(JANI::SYNCHRONISE, std::move(j_sub));
    }
    // result
    if (sync->get_resultID() != ACTION::silentAction) { JSON_EMPLACE(JANI::RESULT, sync->get_result()) }
    //
    JSON_ACCEPT_COMMENT(sync)
    JSON_RETURN
}

void ToJSON_Visitor::visit(const TransientValue* transVal) {
    JSON_CONSTRUCT
    JSON_ACCEPT_IF(transVal->get_ref(), JANI::REF)
    JSON_ACCEPT_IF(transVal->get_value(), JANI::VALUE)
    JSON_ACCEPT_COMMENT(transVal)
    JSON_RETURN
}

void ToJSON_Visitor::visit(const VariableDeclaration* varDecl) {
    JSON_CONSTRUCT
    JSON_EMPLACE(JANI::NAME, varDecl->get_name())
    JSON_ACCEPT_IF(varDecl->get_type(), JANI::TYPE)
    JSON_EMPLACE(JANI::TRANSIENT, varDecl->is_transient())
    JSON_ACCEPT_IF(varDecl->get_initialValue(), JANI::INITIAL_VALUE)
    JSON_ACCEPT_COMMENT(varDecl)
    JSON_RETURN
}

// non-standard:

void ToJSON_Visitor::visit(const FreeVariableDeclaration* var_decl) {
    JSON_CONSTRUCT
    JSON_EMPLACE(JANI::NAME, var_decl->get_name())
    JSON_ACCEPT_IF(var_decl->get_type(), JANI::TYPE)
    JSON_ACCEPT_COMMENT(var_decl)
    JSON_RETURN
}

void ToJSON_Visitor::visit(const Properties* properties) {
    JSON_CONSTRUCT
    JSON_ITERATOR(properties->propertyIterator(), JANI::PROPERTIES)
    JSON_RETURN
}

void ToJSON_Visitor::visit(const RewardAccumulation* reward_accumulation) {
    JSON_CONSTRUCT
    for (const auto& reward_acc_val: reward_accumulation->_reward_accumulation()) { j->push_back(RewardAccumulation::reward_acc_value_to_str(reward_acc_val)); }
    JSON_RETURN
}

// expressions:

void ToJSON_Visitor::visit(const ArrayAccessExpression* exp) {
    JSON_CONSTRUCT
    JSON_EMPLACE(JANI::OP, JANI::AA)
    JSON_ACCEPT_IF(exp->get_accessedArray(), JANI::EXP)
    JSON_ACCEPT_IF(exp->get_index(), JANI::INDEX)
    JSON_RETURN
}

void ToJSON_Visitor::visit(const ArrayConstructorExpression* exp) {
    JSON_CONSTRUCT
    JSON_EMPLACE(JANI::OP, JANI::AC)
    JSON_EMPLACE(JANI::VAR, exp->get_free_var_name())
    JSON_ACCEPT_IF(exp->get_length(), JANI::LENGTH)
    JSON_ACCEPT_IF(exp->get_evalExp(), JANI::EXP)
    JSON_RETURN
}

void ToJSON_Visitor::visit(const ArrayValueExpression* exp) {
    JSON_CONSTRUCT
    JSON_EMPLACE(JANI::OP, JANI::AV)
    JSON_ITERATOR_IF(exp->init_element_it(), JANI::ELEMENTS, exp->get_number_elements())
    JSON_RETURN
}

void ToJSON_Visitor::visit(const BinaryOpExpression* exp) {
    JSON_CONSTRUCT
    JSON_EMPLACE(JANI::OP, BinaryOpExpression::binary_op_to_str(exp->get_op()))
    JSON_ACCEPT_IF(exp->get_left(), JANI::LEFT)
    JSON_ACCEPT_IF(exp->get_right(), JANI::RIGHT)
    JSON_RETURN
}

void ToJSON_Visitor::visit(const BoolValueExpression* exp) { j_rlt = std::make_unique<nlohmann::json>(exp->get_value()); }

void ToJSON_Visitor::visit(const ConstantExpression* exp) { j_rlt = std::make_unique<nlohmann::json>(exp->get_name()); }

void ToJSON_Visitor::visit(const DerivativeExpression* exp) {
    JSON_CONSTRUCT
    JSON_EMPLACE(JANI::OP, JANI::DER)
    JSON_EMPLACE(JANI::VAR, exp->get_name())
    JSON_RETURN
}

void ToJSON_Visitor::visit(const DistributionSamplingExpression* exp) {
    JSON_CONSTRUCT
    JSON_EMPLACE(JANI::DISTRIBUTION, DistributionSamplingExpression::distribution_type_to_str(exp->get_distribution_type()))
    JSON_ITERATOR_IF(exp->argIterator(), JANI::ARGS, exp->get_number_args())
    JSON_RETURN
}

void ToJSON_Visitor::visit(const ExpectationExpression* exp) {
    JSON_CONSTRUCT
    JSON_EMPLACE(JANI::OP, ExpectationExpression::exp_qualifier_to_str(exp->get_op()))
    JSON_ACCEPT_IF(exp->get_value(), JANI::EXP)
    JSON_ACCEPT_IF(exp->get_reward_accumulation(), JANI::ACCUMULATE)
    JSON_ACCEPT_IF(exp->get_reach(), JANI::REACH)
    JSON_ACCEPT_IF(exp->get_step_instant(), JANI::STEP_INSTANT)
    JSON_ACCEPT_IF(exp->get_time_instant(), JANI::TIME_INSTANT)
    JSON_ITERATOR_IF(exp->rewardInstantIterator(), JANI::REWARD_INSTANTS, exp->get_number_reward_instants())
    JSON_RETURN
}

void ToJSON_Visitor::visit(const FilterExpression* exp) {
    JSON_CONSTRUCT
    JSON_EMPLACE(JANI::OP, JANI::FILTER)
    JSON_EMPLACE(JANI::FUN, FilterExpression::filter_fun_to_str(exp->get_fun()))
    JSON_ACCEPT_IF(exp->get_values(), JANI::VALUES)
    JSON_ACCEPT_IF(exp->get_states(), JANI::STATES)
    JSON_RETURN
}

void ToJSON_Visitor::visit(const FreeVariableExpression* exp) {
    j_rlt = std::make_unique<nlohmann::json>(exp->get_name());
}

void ToJSON_Visitor::visit(const IntegerValueExpression* exp) {
    j_rlt = std::make_unique<nlohmann::json>(exp->get_value());
}

void ToJSON_Visitor::visit(const ITE_Expression* exp) {
    JSON_CONSTRUCT
    JSON_EMPLACE(JANI::OP, JANI::ITE)
    JSON_ACCEPT_IF(exp->get_condition(), JANI::IF)
    JSON_ACCEPT_IF(exp->get_consequence(), JANI::THEN)
    JSON_ACCEPT_IF(exp->get_alternative(), JANI::ELSE)
    JSON_RETURN
}

void ToJSON_Visitor::visit(const PathExpression* exp) {
    JSON_CONSTRUCT
    JSON_EMPLACE(JANI::OP, PathExpression::path_op_to_str(exp->get_op()))
    if (!PathExpression::is_binary(exp->get_op())) {
        JSON_ACCEPT_IF(exp->get_right(), JANI::EXP)
    } else {
        JANI_ASSERT(exp->get_left()) // just in case ...
        JSON_ACCEPT_IF(exp->get_left(), JANI::LEFT)
        JSON_ACCEPT_IF(exp->get_right(), JANI::RIGHT)
    }
    JSON_ACCEPT_IF(exp->get_step_bounds(), JANI::STEP_BOUNDS)
    JSON_ACCEPT_IF(exp->get_time_bounds(), JANI::TIME_BOUNDS)
    JSON_ITERATOR_IF(exp->rewardBoundIterator(), JANI::REWARD_BOUNDS, exp->get_number_reward_bounds())
    JSON_RETURN
}

void ToJSON_Visitor::visit(const QfiedExpression* exp) {
    JSON_CONSTRUCT
    JSON_EMPLACE(JANI::OP, QfiedExpression::qfier_to_str(exp->get_op()))
    JSON_ACCEPT_IF(exp->get_path_formula(), JANI::EXP)
    JSON_RETURN
}

void ToJSON_Visitor::visit(const RealValueExpression* exp) {
    if (exp->get_name() == RealValueExpression::NONE_C) {
        j_rlt = std::make_unique<nlohmann::json>(exp->get_value());
    } else {
        JSON_CONSTRUCT
        JSON_EMPLACE(JANI::CONSTANT, constantNameToString[exp->get_name()])
        JSON_RETURN
    }
}

void ToJSON_Visitor::visit(const StatePredicateExpression* exp) {
    j_rlt = std::make_unique<nlohmann::json>(exp->get_name());
}

void ToJSON_Visitor::visit(const UnaryOpExpression* exp) {
    JSON_CONSTRUCT
    JSON_EMPLACE(JANI::OP, UnaryOpExpression::unary_op_to_str(exp->get_op()))
    JSON_ACCEPT_IF(exp->get_operand(), JANI::EXP)
    JSON_RETURN
}

void ToJSON_Visitor::visit(const VariableExpression* exp) {
    j_rlt = std::make_unique<nlohmann::json>(exp->get_name());
}

// non-standard

void ToJSON_Visitor::visit(const ConstantArrayAccessExpression* exp) {
    JSON_CONSTRUCT
    JSON_EMPLACE(JANI::OP, JANI::AA)
    JSON_ACCEPT_IF(exp->get_accessed_array(), JANI::EXP)
    JSON_ACCEPT_IF(exp->get_index(), JANI::INDEX)
    JSON_RETURN
}

void ToJSON_Visitor::visit(const ActionOpTuple* op) {
    JSON_CONSTRUCT
    JSON_EMPLACE(JANI::ID, op->get_op())
    JSON_EMPLACE(JANI::INDEX, op->get_update())
    JSON_RETURN
}

void ToJSON_Visitor::visit(const LetExpression* exp) {
    JSON_CONSTRUCT
    JSON_EMPLACE(JANI::OP, LetExpression::get_op_string())
    JSON_ITERATOR_IF(exp->init_free_variable_iterator(), JANI::VARIABLES, exp->get_number_of_free_variables())
    JSON_ACCEPT_IF(exp->get_expression(), JANI::EXPRESSION)
    JSON_RETURN
}

void ToJSON_Visitor::visit(const LocationValueExpression* exp) {
    JSON_CONSTRUCT
    JSON_EMPLACE(JANI::AUTOMATON, exp->get_automaton_name())
    JSON_EMPLACE(JANI::INDEX, exp->get_location_index())
    JSON_EMPLACE(JANI::LOCATION, exp->get_location_name())
    JSON_RETURN
}

void ToJSON_Visitor::visit(const ObjectiveExpression* exp) {
    JSON_CONSTRUCT
    JSON_EMPLACE(JANI::OP, ObjectiveExpression::get_op_string())
    JSON_ACCEPT_IF(exp->get_goal(), JANI::GOAL)
    JSON_ACCEPT_IF(exp->get_goal_potential(), JANI::GOAL_POTENTIAL)
    JSON_ACCEPT_IF(exp->get_step_reward(), JANI::STEP_REWARD)
    JSON_ACCEPT_IF(exp->get_reward_accumulation(), JANI::ACCUMULATE)
    JSON_RETURN
}

void ToJSON_Visitor::visit(const PAExpression* exp) {
    JSON_CONSTRUCT
    JSON_EMPLACE(JANI::OP, PAExpression::get_op_string())
    JSON_ACCEPT_IF(exp->get_start(), JANI::START)
    JSON_ACCEPT_IF(exp->get_reach(), JANI::REACH)
    JSON_ACCEPT_IF(exp->get_objective(), JANI::OBJECTIVE)
    if (exp->get_predicates()) { JSON_ITERATOR_IF(exp->get_predicates()->predicatesIterator(), JANI::PREDICATES, exp->get_predicates()->get_number_predicates()) }
    if (PLAJA_UTILS::file_exists(exp->get_nnFile())) {
        JSON_EMPLACE(JANI::FILE, targetDirectory ? PLAJA_UTILS::generate_relative_path(exp->get_nnFile(), *targetDirectory) : exp->get_nnFile())
    }
    JSON_RETURN
}

void ToJSON_Visitor::visit(const PredicatesExpression* exp) {
    JSON_CONSTRUCT
    JSON_ITERATOR(exp->predicatesIterator(), JANI::PREDICATES)
    JSON_RETURN
}

void ToJSON_Visitor::visit(const ProblemInstanceExpression* exp) {
    JSON_CONSTRUCT
    JSON_EMPLACE(JANI::OP, ProblemInstanceExpression::get_op_string())
    JSON_EMPLACE(JANI::step, exp->get_target_step())
    JSON_EMPLACE(JANI::POLICY, exp->get_policy_target_step())
    JSON_ITERATOR_IF(exp->init_op_path_it(), JANI::ops, exp->get_op_path_size())
    JSON_ACCEPT_IF(exp->get_start(), JANI::START)
    JSON_ACCEPT_IF(exp->get_reach(), JANI::REACH)
    JSON_EMPLACE(JANI::INITIAL_VALUE, exp->get_includes_init())
    if (exp->get_predicates()) { JSON_ITERATOR_IF(exp->get_predicates()->predicatesIterator(), JANI::PREDICATES, exp->get_predicates()->get_number_predicates()) }
    JSON_ITERATOR_IF(exp->init_pa_state_path_it(), JANI::paStates, exp->get_pa_state_path_size())
    JSON_RETURN
}

void ToJSON_Visitor::visit(const StateConditionExpression* exp) {
    JSON_CONSTRUCT
    JSON_EMPLACE(JANI::OP, StateConditionExpression::get_op_string())
    JSON_ITERATOR_IF(exp->init_loc_value_iterator(), JANI::LOCATIONS, exp->get_size_loc_values())
    JSON_ACCEPT_IF(exp->get_constraint(), JANI::EXP)
    JSON_RETURN
}

void ToJSON_Visitor::visit(const StateValuesExpression* exp) {
    JSON_CONSTRUCT
    JSON_ITERATOR_IF(exp->init_loc_value_iterator(), JANI::LOCATIONS, exp->get_size_loc_values())
    JSON_ITERATOR_IF(exp->init_var_value_iterator(), JANI::VARIABLES, exp->get_size_var_values())
    JSON_RETURN
}

void ToJSON_Visitor::visit(const StatesValuesExpression* exp) {
    JSON_CONSTRUCT
    JSON_EMPLACE(JANI::OP, StatesValuesExpression::get_op_string())
    JSON_ITERATOR_IF(exp->init_state_values_iterator(), JANI::VALUES, exp->get_size_states_values())
    JSON_RETURN
}

void ToJSON_Visitor::visit(const VariableValueExpression* exp) {
    JSON_CONSTRUCT
    JSON_ACCEPT_IF(exp->get_var(), JANI::VAR)
    JSON_ACCEPT_IF(exp->get_val(), JANI::VALUE)
    JSON_RETURN
}

// special cases:

void ToJSON_Visitor::visit(const LinearExpression* exp) { exp->to_standard()->accept(this); }

void ToJSON_Visitor::visit(const NaryExpression* exp) { exp->to_standard()->accept(this); }

// types:

void ToJSON_Visitor::visit(const ArrayType* type) {
    JSON_CONSTRUCT
    JSON_EMPLACE(JANI::KIND, JANI::ARRAY)
    JSON_ACCEPT_IF(type->get_element_type(), JANI::BASE)
    JSON_RETURN
}

void ToJSON_Visitor::visit(const BoolType* /*type*/) {
    j_rlt = std::make_unique<nlohmann::json>(DeclarationType::kind_to_str(DeclarationType::Bool));
}

void ToJSON_Visitor::visit(const BoundedType* type) {
    JSON_CONSTRUCT
    JSON_EMPLACE(JANI::KIND, JANI::BOUNDED)
    if (type->get_base()) { JSON_EMPLACE(JANI::BASE, type->get_base()->get_type_string()) }
    JSON_ACCEPT_IF(type->get_lower_bound(), JANI::LOWER_BOUND)
    JSON_ACCEPT_IF(type->get_upper_bound(), JANI::UPPER_BOUND)
    JSON_RETURN
}

void ToJSON_Visitor::visit(const ClockType* /*type*/) { j_rlt = std::make_unique<nlohmann::json>(DeclarationType::kind_to_str(DeclarationType::Clock)); }

void ToJSON_Visitor::visit(const ContinuousType* /*type*/) { j_rlt = std::make_unique<nlohmann::json>(DeclarationType::kind_to_str(DeclarationType::Continuous)); }

void ToJSON_Visitor::visit(const IntType* /*type*/) { j_rlt = std::make_unique<nlohmann::json>(DeclarationType::kind_to_str(DeclarationType::Int)); }

void ToJSON_Visitor::visit(const RealType* /*type*/) { j_rlt = std::make_unique<nlohmann::json>(DeclarationType::kind_to_str(DeclarationType::Real)); }

// non-standard:

void ToJSON_Visitor::visit(const LocationType* /*type*/) { j_rlt = std::make_unique<nlohmann::json>(DeclarationType::kind_to_str(DeclarationType::Location)); }