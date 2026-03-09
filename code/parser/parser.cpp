//
// This file is part of the PlaJA code base.
// Copyright (c) (2019 - 2019) Marcel Vinzent.
// See README.md in the top-level directory for licensing information.
//

#include <iostream>
#include "parser.h"
#include "../exception/parser_exception.h"
#include "../option_parser/option_parser.h"
#include "ast/automaton.h"
#include "ast/model.h"
#include "ast/property.h"
#include "visitor/restrictions_checker.h"
#include "parser_utils.h"

// extern:
namespace PLAJA_OPTION {
    extern const std::string help;
    extern const std::string additional_properties;
    extern const std::string allow_locals;
}

Parser::Parser(const PLAJA::OptionParser& option_parser):
    optionParser(option_parser)
    , model_ref(nullptr)
    , variable_counter(0)
    , free_variable_counter(0)
    , automaton_counter(AUTOMATON::invalid)
    , currentAutomaton(nullptr)
    , current_directory_path()
    , doConstantsChecks(true) {
}

Parser::~Parser() = default;

std::unique_ptr<Model> Parser::parse(const std::string* filename) {

    nlohmann::json input_j;
    filename = filename ? filename : &optionParser.get_filename();
    if (!PARSER::parse_json_file(*filename, input_j)) { return nullptr; }
    current_directory_path = PLAJA_UTILS::extract_directory_path(*filename);

    // parse to ast
#ifdef NEDBUG
    try {
#endif
    std::unique_ptr<Model> model = parse_Model(input_j);
    parse_additional_properties(model.get(), optionParser.is_flag_set(PLAJA_OPTION::allow_locals));
    check_constants();
    RestrictionsChecker::check_restrictions(model.get(), false); // check not supported fragment
    return model;
#ifdef NEDBUG
    } catch (PlajaException& e) {
        std::cout << "Error while parsing ..." << std::endl;
        std::cout << e.what() << std::endl;
    }
#endif
    return nullptr;
}

/** labels & externals ************************************************************************************************/

void Parser::parse_labels(const nlohmann::json& input) {
    PARSER::check_array(input);
    for (const auto& label_j: input) {
        // parse label definition
        PARSER::check_object(label_j, input);
        PARSER::check_contains(label_j, JANI::NAME);
        const auto& name_j = label_j.at(JANI::NAME);
        PARSER::check_string(name_j, label_j);
        //
        PARSER::check_contains(label_j, JANI::ELEMENT);
        //
        PARSER::check_for_unexpected_element(label_j, 2);
        //
        PARSER::throw_parser_exception_if(labels.count(name_j), label_j, input, PARSER::duplicate_name);
        labels.emplace(name_j, &label_j.at(JANI::ELEMENT));
    }

    PARSER::check_for_unexpected_element(input, 1);
}

void Parser::parse_external(const nlohmann::json& filename) {
    nlohmann::json input_j;
    if (!PARSER::parse_json_file(filename, input_j)) { throw ParserException("Error while parsing external file."); }
    externalStructures.emplace(filename, new nlohmann::json(input_j));
}

const nlohmann::json& Parser::load_label(const nlohmann::json& input) {
    PARSER::check_object(input);
    PARSER::check_contains(input, JANI::LABEL);
    const auto& label_j = input.at(JANI::LABEL);
    PARSER::check_string(label_j, input);
    PARSER::throw_parser_exception_if(labels.count(label_j), label_j, input, PARSER::unknown_name);
    PARSER::check_for_unexpected_element(input, 1);
    return *labels.at(label_j);
}

const nlohmann::json& Parser::load_external(const nlohmann::json& input) {
    PARSER::check_object(input);
    PARSER::check_contains(input, JANI::FILE);
    const auto& file_j = input.at(JANI::FILE);
    PARSER::check_string(file_j, input);
    PARSER::check_for_unexpected_element(input, 1);
    const std::string absolute_path = current_directory_path + static_cast<std::string>(file_j);
    if (not externalStructures.count(absolute_path)) { parse_external(absolute_path); }
    return *externalStructures.at(absolute_path);
}

const nlohmann::json& Parser::load_label_or_external_if(const nlohmann::json& input) {
    if (input.is_object() and input.contains(JANI::LABEL)) { return load_label(input); }
    if (input.is_object() and input.contains(JANI::FILE)) { return load_external(input); }
    return input;
}

/**********************************************************************************************************************/

void Parser::check_constants() {

    if (not doConstantsChecks) { return; }

    /* Option parser is aware of model's constants? */
    for (const auto& constant: optionParser.get_constants()) {
        if (not constants.count(constant.first)) { throw ParserException(PLAJA_UTILS::string_f("Unknown constant \"%s\" provided via commandline", constant.first.c_str())); }
    }

}

void Parser::parse_additional_properties(Model* model, bool allow_locals) {
    if (!optionParser.has_value_option(PLAJA_OPTION::additional_properties)) { return; }

    const std::string& properties_file = optionParser.get_value_option_string(PLAJA_OPTION::additional_properties);
    nlohmann::json input_j;
    if (!PARSER::parse_json_file(properties_file, input_j)) { return; }
    current_directory_path = PLAJA_UTILS::extract_directory_path(properties_file);

    // additional handling if local variables are free to be used

    PLAJA_ASSERT(localVars.empty())
    if (allow_locals) { load_local_variables(); }

    // parse and add to model
#ifdef NDEBUG
    try {
#endif
    unsigned int object_size = 1;
    PARSER::check_object(input_j);
    /* labels ********************************/
    if (input_j.contains(JANI::LABELS)) {
        parse_labels(input_j.at(JANI::LABELS));
        object_size += 1;
    }
    /* **************************************/
    PARSER::check_for_unexpected_element(input_j, object_size);
    PARSER::check_contains(input_j, JANI::PROPERTIES);
    const nlohmann::json& properties_j = input_j.at(JANI::PROPERTIES);
    PARSER::check_array(properties_j, input_j);
    model->reserve(0, 0, 0, 0, model->get_number_properties() + properties_j.size());
    for (const auto& property_j: properties_j) { model->add_property(parse_Property(property_j)); }
#ifdef NDEBUG
    } catch (PlajaException& e) {
        std::cout << "Error while parsing additional properties file, some additional properties may have not been added:" << std::endl;
        std::cout << e.what() << std::endl;
    }
#endif

}

namespace PARSER {

    std::unique_ptr<Model> parse(const std::string* filename, const PLAJA::OptionParser* option_parser, bool check_constants) {
        Parser parser(option_parser ? *option_parser : *PLAJA_GLOBAL::optionParser);
        parser.set_constants_checks(check_constants);
        return parser.parse(filename);
    }

    std::unique_ptr<Model> parse(const std::string* filename, const PLAJA::OptionParser* option_parser) { return parse(filename, option_parser, true); }

}