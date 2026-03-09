//
// This file is part of the PlaJA code base.
// Copyright (c) (2019 - 2019) Marcel Vinzent.
// See README.md in the top-level directory for licensing information.
//

#include "parser.h"
#include "../option_parser/option_parser.h"

#include "ast/expression/bool_value_expression.h"
#include "ast/expression/integer_value_expression.h"
#include "ast/expression/lvalue_expression.h"
#include "ast/expression/real_value_expression.h"

#include "ast/type/array_type.h"
#include "ast/type/bool_type.h"
#include "ast/type/bounded_type.h"

#include "ast/non_standard/free_variable_declaration.h"
#include "ast/non_standard/reward_accumulation.h"
#include "ast/model.h"
#include "ast/action.h"
#include "ast/assignment.h"
#include "ast/automaton.h"
#include "ast/composition.h"
#include "ast/composition_element.h"
#include "ast/constant_declaration.h"
#include "ast/destination.h"
#include "ast/edge.h"
#include "ast/location.h"
#include "ast/metadata.h"
#include "ast/property.h"
#include "ast/property_interval.h"
#include "ast/reward_bound.h"
#include "ast/reward_instant.h"
#include "ast/transient_value.h"
#include "ast/synchronisation.h"
#include "ast/variable_declaration.h"
#include "parser_utils.h"

/* Routines for testing purposes. */

#ifndef NDEBUG

[[maybe_unused]] AutomatonIndex_type Parser::set_automaton_index(AutomatonIndex_type automaton_index) {
    const AutomatonIndex_type tmp = automaton_counter;
    this->automaton_counter = automaton_index;
    return tmp;
}

std::unique_ptr<ConstantDeclaration> Parser::parse_ConstDeclStr(const std::string& const_decl_str) {
    nlohmann::json input;
    std::stringstream(const_decl_str) >> input;
    return this->parse_ConstantDeclaration(input);
}

std::unique_ptr<VariableDeclaration> Parser::parse_VarDeclStr(const std::string& var_decl_str) {
    nlohmann::json input;
    std::stringstream(var_decl_str) >> input;
    return this->parse_VariableDeclaration(input);
}

std::unique_ptr<VariableDeclaration> Parser::parse_var_decl_str(const std::string& var_decl_str, VariableIndex_type index) {
    auto var = parse_VarDeclStr(var_decl_str);
    PLAJA_ASSERT(var)
    var->set_index(index);
    return var;
}

std::unique_ptr<Expression> Parser::parse_ExpStr(const std::string& exp_str) {
    nlohmann::json input;
    std::stringstream(exp_str) >> input;
    return this->parse_Expression(input);
}

#endif

void Parser::load_local_variables() {
    //
    localVars.clear();

    std::unordered_set<std::string> duplicates;
    size_t m, j;
    const size_t l = model_ref->get_number_automataInstances(); // if one uses several instances of one automaton, this naturally results in duplicates
    for (size_t i = 0; i < l; ++i) {
        PLAJA_ASSERT(i < std::numeric_limits<AutomatonIndex_type>::max())
        const Automaton* automaton = model_ref->get_automatonInstance(i); // NOLINT(cppcoreguidelines-narrowing-conversions)
        m = automaton->get_number_variables();
        for (j = 0; j < m; ++j) {
            const VariableDeclaration* varDecl = automaton->get_variable(j);
            if (localVars.count(varDecl->get_name())) { // ambiguous variable
                duplicates.insert(varDecl->get_name());
            } else {
                localVars.emplace(varDecl->get_name(), varDecl);
            }
        }
    }

    // erase duplicate names
    for (const std::string& name: duplicates) { localVars.erase(name); }
}

/**********************************************************************************************************************/

std::unique_ptr<Action> Parser::parse_Action(const nlohmann::json& input) {
    PARSER::check_object(input);

    std::unique_ptr<Action> action(new Action(PARSER::parse_name(input), PLAJA_UTILS::cast_numeric<ActionID_type>(actions.size())));

    // Global maintenance:
    PARSER::throw_parser_exception_if(actions.count(action->get_name()), PARSER::name_to_json(action->get_name()), input, PARSER::duplicate_name); // Check uniqueness.
    actions.emplace(action->get_name(), action.get());

    unsigned int object_size = 1;
    PARSER::parse_comment_if(input, *action, object_size);
    PARSER::check_for_unexpected_element(input, object_size);
    return action;
}

std::unique_ptr<Assignment> Parser::parse_Assignment(const nlohmann::json& input) {
    PARSER::check_object(input);
    std::unique_ptr<Assignment> assignment(new Assignment());

    // ref:
    PARSER::check_contains(input, JANI::REF);
    assignment->set_ref(parse_LValueExpression(input.at(JANI::REF)));
    unsigned int object_size = 1;

    if (input.contains(JANI::VALUE)) { // value:

        assignment->set_value(parse_Expression(input.at(JANI::VALUE)));
        ++object_size;

    } else { // non-deterministic:

        PARSER::check_contains(input, JANI::LOWER_BOUND);
        assignment->set_lower_bound(parse_Expression(input.at(JANI::LOWER_BOUND)));
        ++object_size;

        PARSER::check_contains(input, JANI::UPPER_BOUND);
        assignment->set_upper_bound(parse_Expression(input.at(JANI::UPPER_BOUND)));
        ++object_size;

    }

    // index:
    if (input.contains(JANI::INDEX)) {
        const nlohmann::json& index_j = input.at(JANI::INDEX);
        PARSER::throw_parser_exception_if(!(index_j.is_number_integer() && index_j.is_number_unsigned()), index_j, input, JANI::INDEX);
        assignment->set_index(index_j);
        ++object_size;
    }

    PARSER::parse_comment_if(input, *assignment, object_size);
    PARSER::check_for_unexpected_element(input, object_size);
    return assignment;
}

std::unique_ptr<Automaton> Parser::parse_Automaton(const nlohmann::json& input) {
    PARSER::check_object(input);
    std::unique_ptr<Automaton> automaton(new Automaton());
    currentAutomaton = automaton.get();
    // id:
    automaton->set_index(++automaton_counter);
    // name:
    automaton->set_name(PARSER::parse_name(input));
    unsigned int object_size = 1;
    // variables:
    localVars.clear();
    if (input.contains(JANI::VARIABLES)) {
        ++object_size;
        const nlohmann::json& variables_j = input.at(JANI::VARIABLES);
        PARSER::check_array(variables_j, PARSER::to_json(JANI::VARIABLES, variables_j));
        automaton->reserve(variables_j.size(), 0, 0, 0);
        for (const auto& var_j: variables_j) { automaton->add_variable(parse_VariableDeclaration(var_j)); }
    }
    // restrict-initial:
    if (input.contains(JANI::RESTRICT_INITIAL)) {
        ++object_size;
        const nlohmann::json& restrict_initial_j = input.at(JANI::RESTRICT_INITIAL);
        PARSER::check_object(restrict_initial_j, input);
        PARSER::check_contains(restrict_initial_j, JANI::EXP);
        automaton->set_restrictInitialExpression(parse_Expression(restrict_initial_j.at(JANI::EXP)));
        if (restrict_initial_j.contains(JANI::COMMENT)) {
            const auto& comment_j = restrict_initial_j.at(JANI::COMMENT);
            PARSER::check_string(comment_j, restrict_initial_j);
            automaton->set_restrictInitialComment(comment_j);
            PARSER::check_for_unexpected_element(restrict_initial_j, 2);
        } else {
            PARSER::check_for_unexpected_element(restrict_initial_j, 1);
        }
    }
    // locations:
    locations.clear();
    PARSER::check_contains(input, JANI::LOCATIONS);
    {
        ++object_size;
        const nlohmann::json& locations_j = input.at(JANI::LOCATIONS);
        PARSER::check_array(locations_j, PARSER::to_json(JANI::LOCATIONS, locations_j));
        const std::size_t l = locations_j.size();
        PARSER::throw_parser_exception_if(l < 1, locations_j, input, PARSER::at_least_one_element);
        automaton->reserve(0, l, 0, 0);
        for (const auto& loc_j: locations_j) { automaton->add_location(parse_Location(loc_j)); }
    }
    // initial-locations:
    PARSER::check_contains(input, JANI::INITIAL_LOCATIONS);
    {
        ++object_size;
        const nlohmann::json& init_locations_j = input.at(JANI::INITIAL_LOCATIONS);
        PARSER::check_array(init_locations_j, PARSER::to_json(JANI::INITIAL_LOCATIONS, init_locations_j));
        automaton->reserve(0, 0, init_locations_j.size(), 0);
        for (const auto& init_loc: init_locations_j) {
            PARSER::check_string(init_loc, PARSER::to_json(JANI::INITIAL_LOCATIONS, init_locations_j));
            PARSER::throw_parser_exception_if(!locations.count(init_loc), init_loc, PARSER::to_json(JANI::INITIAL_LOCATIONS, init_locations_j), PARSER::unknown_name);
            automaton->add_initialLocation(locations.at(init_loc)->get_locationValue());
        }
    }
    PARSER::check_contains(input, JANI::EDGES);
    {
        ++object_size;
        const nlohmann::json& edges_j = input.at(JANI::EDGES);
        PARSER::check_array(edges_j, input);
        automaton->reserve(0, 0, 0, edges_j.size());
        for (const auto& edge_j: edges_j) {
            // TODO quick fix for empty edge, should be removed as part of mandatory optimisation after parsing.
            auto tmp_ref = parse_Edge(edge_j);
            if (tmp_ref->get_number_destinations() == 0) { continue; }
            //
            automaton->add_edge(std::move(tmp_ref));
        }
    }

    PARSER::parse_comment_if(input, *automaton, object_size);
    PARSER::check_for_unexpected_element(input, object_size);

    // Global maintenance:
    PARSER::throw_parser_exception_if(automata.count(automaton->get_name()), PARSER::name_to_json(automaton->get_name()), input, PARSER::duplicate_name);
    automata.emplace(automaton->get_name(), automaton.get());    // add to respective mapping

    localVars.clear(); // to not be used outside automaton !!!

    currentAutomaton = nullptr;
    return automaton;
}

std::unique_ptr<Composition> Parser::parse_Composition(const nlohmann::json& input) {
    std::unique_ptr<Composition> system(new Composition());
    // elements:
    PARSER::check_contains(input, JANI::ELEMENTS);
    // else:
    size_t l_e; // needed for syncs
    {
        const nlohmann::json& elements_j = input.at(JANI::ELEMENTS);
        PARSER::check_array(elements_j, input);
        l_e = elements_j.size();
        system->reserve(l_e, 0);
        PARSER::throw_parser_exception_if(l_e < 1, elements_j, input, PARSER::at_least_one_element); // At least one automaton instance, however not strict JANI standard.
        for (const auto& element_j: elements_j) { system->add_element(parse_CompositionElement(element_j)); }
    }
    unsigned int object_size = 1;
    // syncs:
    if (input.contains(JANI::SYNCS)) {
        ++object_size;
        const nlohmann::json& syncs_j = input.at(JANI::SYNCS);
        PARSER::check_array(syncs_j, input);
        const std::size_t l_s = syncs_j.size();
        system->reserve(0, l_s);
        for (size_t i = 0; i < l_s; ++i) {
            system->add_synchronisation(parse_Synchronisation(syncs_j[i]));
            PARSER::throw_parser_exception_if(l_e != system->get_sync(i)->get_size_synchronise(), syncs_j[i], input, JANI::ELEMENTS); // Synchronise structures have same size as automaton.
        }
    }
    PARSER::parse_comment_if(input, *system, object_size);
    PARSER::check_for_unexpected_element(input, object_size);
    return system;
}

std::unique_ptr<CompositionElement> Parser::parse_CompositionElement(const nlohmann::json& input) const {
    // automaton:
    PARSER::check_object(input);
    PARSER::check_contains(input, JANI::AUTOMATON);
    const nlohmann::json& automaton_j = input.at(JANI::AUTOMATON);
    PARSER::check_string(automaton_j, input);
    std::unique_ptr<CompositionElement> compElem(new CompositionElement(automaton_j));
    // input-enable:
    PARSER::throw_parser_exception_if(input.contains(JANI::INPUT_ENABLE), input, JANI::INPUT_ENABLE);

    unsigned int object_size = 1;
    PARSER::parse_comment_if(input, *compElem, object_size);
    PARSER::check_for_unexpected_element(input, object_size);
    return compElem;
}

std::unique_ptr<ConstantDeclaration> Parser::parse_ConstantDeclaration(const nlohmann::json& input) {
    PARSER::check_object(input);
    std::unique_ptr<ConstantDeclaration> const_decl(new ConstantDeclaration(constants.size()));

    /* Name. */
    const_decl->set_name(PARSER::parse_name(input));

    /* Type. */
    PARSER::check_contains(input, JANI::TYPE);
    const nlohmann::json& type_j = input.at(JANI::TYPE);
    if (type_j.is_string()) { const_decl->set_declaration_type(parse_BasicType(type_j)); }
    else if (type_j.is_object() and type_j.contains(JANI::KIND)) { // Non-trivial type.
        const auto& kind_j = type_j.at(JANI::KIND);
        PARSER::check_string(kind_j, input);
        if (JANI::ARRAY == kind_j) { const_decl->set_declaration_type(parse_ArrayType(type_j)); }
        if (JANI::BOUNDED == kind_j) { const_decl->set_declaration_type(parse_BoundedType(type_j)); }
    } else {
        PARSER::throw_parser_exception(type_j, input, PARSER::invalid);
    }

    unsigned int object_size = 2;

    /* Value. */
    if (input.contains(JANI::VALUE)) {
        ++object_size;
        const_decl->set_value(parse_Expression(input.at(JANI::VALUE)));
    }

    if (optionParser.has_constant(const_decl->get_name())) { // Add commandline value.

        PLAJA_LOG_IF(doConstantsChecks and const_decl->get_value(), PLAJA_UTILS::to_red_string(PLAJA_UTILS::string_f("Warning: Overwriting default constant value for \"%s\" via commandline. This will not affect inlined functionality.", const_decl->get_name().c_str())))

        if (const_decl->get_type()->is_floating_type()) {
            const_decl->set_value(std::make_unique<RealValueExpression>(RealValueExpression::NONE_C, optionParser.get_constant<PLAJA::floating>(const_decl->get_name())));
        } else if (const_decl->get_type()->is_integer_type()) {
            const_decl->set_value(std::make_unique<IntegerValueExpression>(optionParser.get_constant<PLAJA::integer>(const_decl->get_name())));
        } else if (BoolType::assignable_from(*const_decl->get_type())) {
            const_decl->set_value(std::make_unique<BoolValueExpression>(optionParser.get_constant<bool>(const_decl->get_name())));
        }

    }

    // Else: moved missing-value-check to restrictions checker.

    PARSER::parse_comment_if(input, *const_decl, object_size);
    PARSER::check_for_unexpected_element(input, object_size);

    // Global maintenance:
    PARSER::throw_parser_exception_if(constants.count(const_decl->get_name()), PARSER::name_to_json(const_decl->get_name()), input, PARSER::duplicate_name); // Check uniqueness: ASSUMPTION: constants are parsed before any variable.
    constants.emplace(const_decl->get_name(), const_decl.get()); // add to respective mapping

    return const_decl;
}

std::unique_ptr<Destination> Parser::parse_Destination(const nlohmann::json& input) {
    PARSER::check_object(input);
    PLAJA_ASSERT(currentAutomaton)
    std::unique_ptr<Destination> destination(new Destination(currentAutomaton));

    /* Location. */
    PARSER::check_contains(input, JANI::LOCATION);
    const nlohmann::json& location_j = input.at(JANI::LOCATION);
    PARSER::check_string(location_j, input);
    PARSER::throw_parser_exception_if(!locations.count(location_j), location_j, input, PARSER::unknown_name);
    destination->set_location(locations.at(location_j));
    unsigned int object_size = 1;

    /* Probability. */
    if (input.contains(JANI::PROBABILITY)) {
        const Model::ModelType model_type = model_ref->get_model_type();
        PARSER::throw_parser_exception_if(model_type == Model::LTS or model_type == Model::TA or model_type == Model::HA, input, JANI::PROBABILITY);
        ++object_size;
        const nlohmann::json& probability_j = input.at(JANI::PROBABILITY);
        PARSER::check_object(probability_j, input);
        PARSER::check_contains(probability_j, JANI::EXP);
        destination->set_probabilityExpression(parse_Expression(probability_j.at(JANI::EXP)));
        if (probability_j.contains(JANI::COMMENT)) {
            ++object_size;
            const auto& comment_j = probability_j.at(JANI::COMMENT);
            PARSER::check_string(comment_j, probability_j);
            destination->set_probabilityComment(comment_j);
        }
    }

    /* Assignments. */
    if (input.contains(JANI::ASSIGNMENTS)) {
        ++object_size;
        const nlohmann::json& assignments_j = input.at(JANI::ASSIGNMENTS);
        PARSER::check_array(assignments_j, input);
        // destination->reserve(assignments.size()); // assignments are stored w.r.t. index
        for (const auto& assignment_j: assignments_j) { destination->add_assignment(parse_Assignment(assignment_j)); }
    }

    PARSER::parse_comment_if(input, *destination, object_size);
    PARSER::check_for_unexpected_element(input, object_size);
    return destination;
}

std::unique_ptr<Edge> Parser::parse_Edge(const nlohmann::json& input) {
    PARSER::check_object(input);
    PLAJA_ASSERT(currentAutomaton)
    std::unique_ptr<Edge> edge(new Edge(currentAutomaton));

    /* Location. */
    {
        PARSER::check_contains(input, JANI::LOCATION);
        const nlohmann::json& location_j = input.at(JANI::LOCATION);
        PARSER::check_string(location_j, input);
        PARSER::throw_parser_exception_if(!locations.count(location_j), location_j, input, PARSER::unknown_name);
        edge->set_location(locations.at(location_j));
    }
    unsigned int object_size = 1;

    /* Action. */
    if (input.contains(JANI::ACTION)) {
        object_size++;
        const nlohmann::json& action_j = input.at(JANI::ACTION);
        PARSER::check_string(action_j, input);
        PARSER::throw_parser_exception_if(!actions.count(action_j), action_j, input, PARSER::unknown_name);
        edge->set_action(actions.at(action_j));
    }

    /* Rate. */
    PARSER::throw_parser_exception_if(input.contains(JANI::RATE), input, JANI::RATE);

    /* Guard. */
    if (input.contains(JANI::GUARD)) {
        ++object_size;
        const nlohmann::json& guard_j = input.at(JANI::GUARD);
        PARSER::check_object(guard_j, input);
        PARSER::check_contains(guard_j, JANI::EXP);
        edge->set_guardExpression(parse_Expression(guard_j.at(JANI::EXP)));
        if (guard_j.contains(JANI::COMMENT)) {
            const auto& comment_j = guard_j.at(JANI::COMMENT);
            PARSER::check_string(comment_j, guard_j);
            edge->set_guardComment(comment_j);
            PARSER::check_for_unexpected_element(guard_j, 2); // sub-size-check
        } else {
            PARSER::check_for_unexpected_element(guard_j, 1);
        }

    } /* else {
        edge->set_guardExpression(std::make_unique<BoolValueExpression>(true));
    } */

    /* Destinations: */
    PARSER::check_contains(input, JANI::DESTINATIONS);
    { // else:
        ++object_size;
        const nlohmann::json& destinations_j = input.at(JANI::DESTINATIONS);
        PARSER::check_array(destinations_j, input);
        auto l = destinations_j.size();
        // model type check:
        const Model::ModelType model_type = model_ref->get_model_type();
        PARSER::throw_parser_exception_if(l < 1 or ((model_type == Model::LTS or model_type == Model::TA or model_type == Model::HA) and l > 1), input, PARSER::not_supported_by_model_type);
        //
        edge->reserve(l);
        for (const auto& destination_j: destinations_j) {
            // No "if (tmp_ref->get_number_sequences() == 0) continue;", since there are no empty destinations (in contrast to edges as used above) due to location assignment.
            // We may remove 0-destinations: we implement this optimization via SetConstants (i.e., once all constant values are set).
            edge->add_destination(parse_Destination(destination_j));
        }
    }

    PARSER::parse_comment_if(input, *edge, object_size);
    PARSER::check_for_unexpected_element(input, object_size);
    return edge;
}

std::unique_ptr<Location> Parser::parse_Location(const nlohmann::json& input) {
    PARSER::check_object(input);
    std::unique_ptr<Location> location(new Location());
    // name:
    location->set_name(PARSER::parse_name(input));
    unsigned int object_size = 1;
    // time-progress:
    PARSER::throw_parser_exception_if(input.contains(JANI::TIME_PROGRESS), input, JANI::TIME_PROGRESS);
    // transient-values:
    if (input.contains(JANI::TRANSIENT_VALUES)) {
        ++object_size;
        const nlohmann::json& transient_values_j = input.at(JANI::TRANSIENT_VALUES);
        PARSER::check_array(transient_values_j, input);
        location->reserve(transient_values_j.size());
        for (const auto& transient_value_j: transient_values_j) { location->add_transient_value(parse_TransientValue(transient_value_j)); }
    }

    PARSER::parse_comment_if(input, *location, object_size);
    PARSER::check_for_unexpected_element(input, object_size);

    // Global maintenance:
    PARSER::throw_parser_exception_if(locations.count(location->get_name()), PARSER::to_json(JANI::NAME, location->get_name()), input, PARSER::duplicate_name);
    location->set_locationValue(PLAJA_UTILS::cast_numeric<PLAJA::integer>(locations.size())); // Set id (#locations, as 0 to #locations - 1 is already used).
    locations.emplace(location->get_name(), location.get()); // add to respective mapping

    return location;
}

/***The order in which the substructures of the model are parsed is crucial.***/
std::unique_ptr<Model> Parser::parse_Model(const nlohmann::json& input) {
    PARSER::check_object(input);
    // type:
    PARSER::check_contains(input, JANI::TYPE);
    const auto& type_j = input.at(JANI::TYPE);
    PARSER::check_string(type_j, input);
    PARSER::throw_not_supported_if(not Model::str_to_model_type(type_j), type_j, input, JANI::TYPE);
    /* Reasons to throw not_supported above: (i) I am not completely sure whether the parser works properly for other models (ii) not need to completely parse a model rejected anyway. */

    unsigned int object_size = 1;

    /* labels ********************************/
    if (input.contains(JANI::LABELS)) {
        parse_labels(input.at(JANI::LABELS));
        object_size += 1;
    }
    /* **************************************/

    std::unique_ptr<Model> model(new Model(*Model::str_to_model_type(type_j)));
    /***maintenance***/
    model_ref = model.get(); // set model
    /***maintenance***/
    // jani-version:
    PARSER::check_contains(input, JANI::JANI_VERSION);
    model->set_jani_version(input.at(JANI::JANI_VERSION));
    // name:
    model->set_name(PARSER::parse_name(input));
    object_size += 2;
    // metadata:
    PARSER::throw_parser_exception_if(input.contains(JANI::METADATA), input, JANI::METADATA);
    // features:
    if (input.contains(JANI::FEATURES)) {
        ++object_size;
        const nlohmann::json& model_features_j = input.at(JANI::FEATURES);
        PARSER::check_array(model_features_j, PARSER::to_json(JANI::FEATURES, model_features_j));
        const std::size_t l = model_features_j.size();
        model->reserve(l, 0, 0, 0, 0);
        for (const auto& model_feature_j: model_features_j) {
            PARSER::check_string(model_feature_j, model_features_j);
            PARSER::throw_parser_exception_if(not Model::str_to_model_feature(model_feature_j), input, JANI::FEATURES);
            model->add_model_feature(*Model::str_to_model_feature(model_feature_j));
        }
    }
    // actions:
    if (input.contains(JANI::ACTIONS)) {
        ++object_size;
        const nlohmann::json& actions_j = input.at(JANI::ACTIONS);
        PARSER::check_array(actions_j, PARSER::to_json(JANI::ACTIONS, actions_j));
        model->reserve(0, actions_j.size(), 0, 0, 0);
        for (const auto& action_j: actions_j) { model->add_action(parse_Action(action_j)); }
    }
    // constants:
    if (input.contains(JANI::CONSTANTS)) {
        ++object_size;
        const nlohmann::json& constants_j = input.at(JANI::CONSTANTS);
        PARSER::check_array(constants_j, PARSER::to_json(JANI::CONSTANTS, constants_j));
        model->reserve(0, 0, constants_j.size(), 0, 0);
        for (const auto& constant_j: constants_j) { model->add_constant(parse_ConstantDeclaration(constant_j)); }
    }
    // system:
    PARSER::check_contains(input, JANI::SYSTEM);
    ++object_size;
    model->set_system(parse_Composition(input.at(JANI::SYSTEM)));
    // variables:
    if (input.contains(JANI::VARIABLES)) {
        ++object_size;
        const nlohmann::json& variables_j = input.at(JANI::VARIABLES);
        PARSER::check_array(variables_j, PARSER::to_json(JANI::VARIABLES, variables_j));
        model->reserve(0, 0, 0, variables_j.size(), 0);
        for (const auto& var_j: variables_j) { model->add_variable(parse_VariableDeclaration(var_j)); }
    }
    // restrict-initial:
    if (input.contains(JANI::RESTRICT_INITIAL)) {
        ++object_size;
        const nlohmann::json& restrict_initial_j = input.at(JANI::RESTRICT_INITIAL);
        PARSER::check_object(restrict_initial_j, PARSER::to_json(JANI::RESTRICT_INITIAL, restrict_initial_j));
        PARSER::check_contains(restrict_initial_j, JANI::EXP);
        model->set_restrict_initial_expression(parse_Expression(restrict_initial_j.at(JANI::EXP)));
        if (restrict_initial_j.contains(JANI::COMMENT)) {
            const auto& comment_j = restrict_initial_j.at(JANI::COMMENT);
            PARSER::check_string(comment_j, restrict_initial_j);
            model->set_restrict_initial_comment(comment_j);
            PARSER::check_for_unexpected_element(restrict_initial_j, 2);
        } else {
            PARSER::check_for_unexpected_element(restrict_initial_j, 1);
        }
    }
    // automata:
    PARSER::check_contains(input, JANI::AUTOMATA);
    ++object_size;
    {
        const nlohmann::json& automata_j = input.at(JANI::AUTOMATA);
        PARSER::check_array(automata_j, PARSER::to_json(JANI::AUTOMATA, automata_j));
        const std::size_t l_a = automata_j.size();
        PARSER::throw_parser_exception_if(l_a < 1, automata_j, PARSER::to_json(JANI::AUTOMATA, automata_j), PARSER::at_least_one_element);
        // Parse only automata in the composition (thereby multiple occurrences are handled implicitly, i.e. each occurrence is parsed separately (potentially the same automata object several times))
        // TODO maybe change this to parsing automata descriptions, and instantiate with respect to system
        Composition* system = model->get_system();
        const std::size_t l_s = system->get_number_elements();
        // automata structures
        std::vector<int> automata_indexes(l_s, -1); // instance-to-description-mapping
        std::vector<std::string> system_names; // for error information
        system_names.reserve(l_s);
        std::vector<AutomatonIndex_type> automata_ids(l_a, AUTOMATON::invalid); // description-to-instance-mapping
        std::vector<std::string> automata_names; // for error information
        automata_names.reserve(l_a);
        // Check for existence and uniqueness
        for (AutomatonIndex_type instance_index = 0; instance_index < l_s; ++instance_index) {
            const std::string& name = system->get_element(instance_index)->get_elementAutomaton();
            system_names.push_back(name);
            for (std::size_t j = 0; j < l_a; ++j) {
                const auto& automaton_j = automata_j[j];
                PARSER::check_contains(automaton_j, JANI::NAME);
                automata_names.push_back(automaton_j.at(JANI::NAME));
                if (name == automaton_j.at(JANI::NAME)) {
                    PARSER::throw_parser_exception_if(automata_indexes[instance_index] != -1, automaton_j.at(JANI::NAME), automaton_j, PARSER::duplicate_name);
                    automata_indexes[instance_index] = PLAJA_UTILS::cast_numeric<int>(j);
                    automata_ids[j] = instance_index;
                }
            }
            PARSER::throw_parser_exception_if(automata_indexes[instance_index] == -1, nlohmann::json(name), system->to_string(false), PARSER::unknown_name);
        }
        // actual parsing of *instances*
        model->reserve_automata(l_a, l_s);
        for (AutomatonIndex_type instance_index = 0; instance_index < l_s; ++instance_index) {
            model->add_automatonInstance(parse_Automaton(automata_j[automata_indexes[instance_index]])); // only parsing
        }
        // now add model descriptions (un-instantiated automata will have no representing instance)
        for (std::size_t j = 0; j < l_a; ++j) {
            if (automata_ids[j] == AUTOMATON::invalid) { std::cout << "Warning: automaton \"" << automata_names[j] << "\" is not instantiated in the system. The json construct will not be parsed." << std::endl; }
            else { model->add_automaton(automata_ids[j]); }
        }
    }
    // properties:
    localVars.clear(); // property expressions only over global variables
    if (input.contains(JANI::PROPERTIES)) {
        ++object_size;
        const nlohmann::json& properties_j = input.at(JANI::PROPERTIES);
        PARSER::check_array(properties_j, PARSER::to_json(JANI::PROPERTIES, properties_j));
        model->reserve(0, 0, 0, 0, properties_j.size());
        for (const auto& prop_j: properties_j) { model->add_property(parse_Property(prop_j)); }
    }

    PARSER::check_for_unexpected_element(input, object_size);
    return model;
}

std::unique_ptr<Property> Parser::parse_Property(const nlohmann::json& input) {
    PARSER::check_object(input);
    std::unique_ptr<Property> property(new Property());
    // name:
    property->set_name(PARSER::parse_name(input));
    // expression:
    PARSER::check_contains(input, JANI::EXPRESSION);
    property->set_propertyExpression(parse_PropertyExpression(input.at(JANI::EXPRESSION), false));

    unsigned int object_size = 2;
    PARSER::parse_comment_if(input, *property, object_size);
    PARSER::check_for_unexpected_element(input, object_size);

    // Global maintenance:
    PARSER::throw_parser_exception_if(properties.count(property->get_name()), PARSER::name_to_json(property->get_name()), input, PARSER::duplicate_name);
    properties.emplace(property->get_name(), property.get()); // add to respective mapping

    return property;
}

std::unique_ptr<PropertyInterval> Parser::parse_PropertyInterval(const nlohmann::json& input) {
    PARSER::check_object(input);
    std::unique_ptr<PropertyInterval> propInterval(new PropertyInterval());
    unsigned int object_size = 0;
    // lower:
    if (input.contains(JANI::LOWER)) {
        ++object_size;
        propInterval->set_lower(parse_Expression(input.at(JANI::LOWER)));
    }
    if (input.contains(JANI::LOWER_EXCLUSIVE)) {
        PARSER::throw_parser_exception_if(!propInterval->get_lower(), PARSER::to_json(JANI::LOWER_EXCLUSIVE, input.at(JANI::LOWER_EXCLUSIVE)), input, PARSER::unexpected_element); // lower-exclusive must not be present if lower is not present
        ++object_size;
        propInterval->set_lowerExclusive(input.at(JANI::LOWER_EXCLUSIVE));
    } else { propInterval->set_lowerExclusive(false); }
    // upper:
    if (input.contains(JANI::UPPER)) {
        ++object_size;
        propInterval->set_upper(parse_Expression(input.at(JANI::UPPER)));
    }
    if (input.contains(JANI::UPPER_EXCLUSIVE)) {
        PARSER::throw_parser_exception_if(!propInterval->get_upper(), PARSER::to_json(JANI::UPPER_EXCLUSIVE, input.at(JANI::UPPER_EXCLUSIVE)), input, PARSER::unexpected_element);
        ++object_size;
        propInterval->set_upperExclusive(input.at(JANI::UPPER_EXCLUSIVE));
    } else { propInterval->set_upperExclusive(false); }

    // upper-exclusive must not be present if upper is not present
    PARSER::throw_parser_exception_if(not propInterval->get_lower() and not propInterval->get_upper(), input, (JANI::LOWER + PLAJA_UTILS::commaString + JANI::UPPER)); // At least one, lower or upper, must be present.

    PARSER::check_for_unexpected_element(input, object_size);
    return propInterval;
}

std::unique_ptr<RewardBound> Parser::parse_RewardBound(const nlohmann::json& input) {
    PARSER::check_object(input);
    std::unique_ptr<RewardBound> rewardBound(new RewardBound());
    PARSER::check_contains(input, JANI::EXP);
    rewardBound->set_accumulation_expression(parse_Expression(input.at(JANI::EXP)));
    PARSER::check_contains(input, JANI::ACCUMULATE);
    rewardBound->set_reward_accumulation(parse_reward_accumulation(input.at(JANI::ACCUMULATE)));
    PARSER::check_contains(input, JANI::BOUNDS);
    rewardBound->set_bounds(parse_PropertyInterval(input.at(JANI::BOUNDS)));

    PARSER::check_for_unexpected_element(input, 3);
    return rewardBound;
}

std::unique_ptr<RewardInstant> Parser::parse_RewardInstant(const nlohmann::json& input) {
    PARSER::check_object(input);
    std::unique_ptr<RewardInstant> rewardInstant(new RewardInstant());
    PARSER::check_contains(input, JANI::EXP);
    rewardInstant->set_accumulation_expression(parse_Expression(input.at(JANI::EXP)));
    PARSER::check_contains(input, JANI::ACCUMULATE);
    rewardInstant->set_reward_accumulation(parse_reward_accumulation(input.at(JANI::ACCUMULATE)));
    PARSER::check_contains(input, JANI::INSTANT);
    rewardInstant->set_accumulation_expression(parse_Expression(input.at(JANI::INSTANT)));

    PARSER::check_for_unexpected_element(input, 3);
    return rewardInstant;
}

std::unique_ptr<Synchronisation> Parser::parse_Synchronisation(const nlohmann::json& input) {
    PARSER::check_object(input);
    std::unique_ptr<Synchronisation> sync(new Synchronisation());
    // type:
    PARSER::check_contains(input, JANI::SYNCHRONISE);
    { // else:
        const nlohmann::json& synchronise_j = input.at(JANI::SYNCHRONISE);
        PARSER::check_array(synchronise_j, PARSER::to_json(JANI::SYNCHRONISE, synchronise_j));
        for (const auto& action_j: synchronise_j) {
            if (action_j.is_null()) {
                sync->add_sync(PLAJA_UTILS::emptyString, ACTION::nullAction); // the null action_j, thus any string, e.g., the empty string
                continue;
            }
            // else:
            PARSER::check_string(action_j, PARSER::to_json(JANI::SYNCHRONISE, synchronise_j));
            PARSER::throw_parser_exception_if(!actions.count(action_j), action_j, PARSER::to_json(JANI::SYNCHRONISE, synchronise_j), PARSER::unknown_name);
            sync->add_sync(actions.at(action_j)->get_name(), actions.at(action_j)->get_id());
        }
    }
    unsigned int object_size = 1;
    // result:
    if (input.contains(JANI::RESULT)) {
        ++object_size;
        const nlohmann::json& result_j = input.at(JANI::RESULT);
        PARSER::check_string(result_j, PARSER::to_json(JANI::RESULT, result_j));
        PARSER::throw_parser_exception_if(!actions.count(result_j), result_j, PARSER::to_json(JANI::RESULT, result_j), PARSER::unknown_name);
        sync->set_result(actions.at(result_j)->get_name());
        sync->set_resultID(actions.at(result_j)->get_id());
    }

    PARSER::parse_comment_if(input, *sync, object_size);
    PARSER::check_for_unexpected_element(input, object_size);
    return sync;
}

std::unique_ptr<TransientValue> Parser::parse_TransientValue(const nlohmann::json& input) {
    PARSER::check_object(input);
    std::unique_ptr<TransientValue> transient_value(new TransientValue());
    // ref:
    PARSER::check_contains(input, JANI::REF);
    transient_value->set_ref(parse_LValueExpression(input.at(JANI::REF)));
    // value:
    PARSER::check_contains(input, JANI::VALUE);
    transient_value->set_value(parse_Expression(input.at(JANI::VALUE)));
    unsigned int object_size = 2;

    PARSER::parse_comment_if(input, *transient_value, object_size);
    PARSER::check_for_unexpected_element(input, object_size);
    return transient_value;
}

std::unique_ptr<VariableDeclaration> Parser::parse_VariableDeclaration(const nlohmann::json& input) {
    PARSER::check_object(input);
    std::unique_ptr<VariableDeclaration> varDecl(new VariableDeclaration(variable_counter++)); // if construction fails, parsing fails, so no need to reset counter in that case
    // name:
    varDecl->set_name(PARSER::parse_name(input));
    // type:
    PARSER::check_contains(input, JANI::TYPE);
    varDecl->set_declarationType(parse_DeclarationType(input.at(JANI::TYPE)));
    unsigned int object_size = 2;
    // transient:
    if (input.contains(JANI::TRANSIENT)) {
        ++object_size;
        const nlohmann::json& transient_j = input.at(JANI::TRANSIENT);
        PARSER::throw_parser_exception_if(!transient_j.is_boolean(), transient_j, input, JANI::TRANSIENT);
        varDecl->set_transient(transient_j);
    }
    // initial-value:
    if (input.contains(JANI::INITIAL_VALUE)) {
        ++object_size;
        varDecl->set_initialValue(parse_Expression(input.at(JANI::INITIAL_VALUE)));
    } else {
        PARSER::throw_parser_exception_if(varDecl->is_transient(), input, (JANI::TRANSIENT + PLAJA_UTILS::commaString + JANI::INITIAL_VALUE)); // If transient, initial state is non-optional.
    }

    PARSER::parse_comment_if(input, *varDecl, object_size);
    PARSER::check_for_unexpected_element(input, object_size);

    // Global maintenance:
    const auto& name = varDecl->get_name();
    PARSER::throw_parser_exception_if(globalVars.count(name) or localVars.count(name) or constants.count(name), PARSER::name_to_json(name), input, PARSER::duplicate_name);
    if (automaton_counter <= AUTOMATON::invalid) {
        globalVars.emplace(name, varDecl.get()); // add to respective mapping
    } else { localVars.emplace(name, varDecl.get()); }

    return varDecl;
}

std::unique_ptr<FreeVariableDeclaration> Parser::parse_FreeVariableDeclaration(const nlohmann::json& input) {
    PARSER::check_object(input);
    std::unique_ptr<FreeVariableDeclaration> varDecl(new FreeVariableDeclaration(free_variable_counter++)); // if construction fails, parsing fails, so no need to reset counter in that case
    // name:
    varDecl->set_name(PARSER::parse_name(input));
    // type:
    PARSER::check_contains(input, JANI::TYPE);
    varDecl->set_type(parse_DeclarationType(input.at(JANI::TYPE)));
    unsigned int object_size = 2;

    PARSER::parse_comment_if(input, *varDecl, object_size);
    PARSER::check_for_unexpected_element(input, object_size);

    // Global maintenance:
    const auto& name = varDecl->get_name();
    PARSER::throw_parser_exception_if(globalVars.count(name) or localVars.count(name) or constants.count(name) or freeVars.count(name), PARSER::name_to_json(name), input, PARSER::duplicate_name); // Check uniqueness.
    freeVars.emplace(name, varDecl.get()); // add to respective mapping

    return varDecl;
}

std::unique_ptr<RewardAccumulation> Parser::parse_reward_accumulation(const nlohmann::json& input) {
    PARSER::check_array(input);
    PARSER::throw_parser_exception_if(input.empty(), input, PARSER::at_least_one_element);

    std::unique_ptr<RewardAccumulation> reward_accumulation(new RewardAccumulation());
    reward_accumulation->reserve(input.size());

    std::unordered_set<RewardAccumulation::RewardAccValue> cached_acc_val;
    for (const auto& acc_j: input) {
        PARSER::check_string(acc_j, input);
        auto reward_acc_val = RewardAccumulation::str_to_reward_acc_value(acc_j);
        PARSER::throw_parser_exception_if(not reward_acc_val, acc_j, input, PARSER::unknown_name);
        PARSER::throw_parser_exception_if(not cached_acc_val.insert(*reward_acc_val).second, acc_j, input, PARSER::duplicate_name);
        reward_accumulation->add_reward_accumulation_value(*reward_acc_val);
    }

    return reward_accumulation;
}