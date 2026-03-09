//
// This file is part of the PlaJA code base.
// Copyright (c) (2019 - 2023) Marcel Vinzent.
// See README.md in the top-level directory for licensing information.
//

#include "test_parser.h"
#include "../../option_parser/enum_option_values_set.h"
#include "../../option_parser/option_parser.h"
#include "../../parser/ast/model.h"
#include "../../parser/visitor/semantics_checker.h"
#include "../../parser/visitor/set_variable_index.h"
#include "../../parser/parser.h"
#include "../../search/information/property_information.h"

PLAJA::ModelConstructorForTests::ModelConstructorForTests():
    config(optionParser) {
}

PLAJA::ModelConstructorForTests::~ModelConstructorForTests() = default;

void PLAJA::ModelConstructorForTests::set_constant(const std::string& name, const std::string& value) {
    optionParser.constants.emplace(name, value);
}

void PLAJA::ModelConstructorForTests::set_option_parser(const SearchEngineFactory* factory) {

    PLAJA_GLOBAL::optionParser = &optionParser; // Set globally.

    /* Clear options with default. */
    optionParser.flags.clear();
    optionParser.valueOptions.clear();
    optionParser.strOptions.clear();
    optionParser.intOptions.clear();
    optionParser.doubleOptions.clear();
    optionParser.enumOptions.clear();

    optionParser.add_options();
    if (factory) { factory->add_options(optionParser); }

    /* Customize options with default. */
    for (const auto& flag: config.flags) { optionParser.flags.insert(flag); }
    for (const auto& flag: config.disabledFlags) { optionParser.flags.erase(flag); }

    for (const auto& option: config.disabledOptions) {
        optionParser.valueOptions.erase(option);
        optionParser.strOptions.erase(option);
    }

    for (const auto& [option, value]: config.options) {
        if (optionParser.knownValueOptions.count(option)) { optionParser.valueOptions[option] = value; }
        else { optionParser.strOptions[option] = value; }
    }

    for (const auto& [option, value]: config.optionsInt) { optionParser.intOptions[option] = value; }
    for (const auto& [option, value]: config.optionsDouble) { optionParser.doubleOptions[option] = value; }
    for (const auto& [option, value]: config.optionsEnum) { optionParser.enumOptions[option] = std::make_unique<PLAJA_OPTION::EnumOptionValuesSet>(*value); }

}

void PLAJA::ModelConstructorForTests::construct_model(const std::string& filename) {
    set_option_parser(nullptr);
    optionParser.valueOptions.emplace(PLAJA_OPTION::model_file, filename);
    cachedParser = std::make_unique<Parser>(optionParser);
    constructedModel = cachedParser->parse();
    PLAJA_GLOBAL::currentModel = constructedModel.get();
    SEMANTICS_CHECKER::check_semantics(constructedModel.get());
    constructedModel->compute_model_information();
}

std::unique_ptr<PropertyInformation> PLAJA::ModelConstructorForTests::analyze_property(std::size_t property_index) const {
    return PropertyInformation::analyse_property(*constructedModel->get_property(property_index), *constructedModel);
}

void PLAJA::ModelConstructorForTests::load_property(std::size_t property_index) { cachedPropertyInfo = analyze_property(property_index); }

std::unique_ptr<SearchEngine> PLAJA::ModelConstructorForTests::construct_instance(SearchEngineFactory::SearchEngineType search_engine, const Model& model, const Property& property) {
    std::unique_ptr<SearchEngineFactory> engine_factory = SearchEngineFactory::construct_factory(search_engine);

    engine_factory->supports_model(model);
    cachedPropertyInfo = PropertyInformation::analyse_property(config, property, model);
    engine_factory->supports_property(*cachedPropertyInfo);

    set_option_parser(engine_factory.get());

    auto cachedConfig = engine_factory->init_configuration(optionParser, cachedPropertyInfo.get());

    return engine_factory->construct_engine(*cachedConfig);
}

std::unique_ptr<SearchEngine> PLAJA::ModelConstructorForTests::construct_instance(SearchEngineFactory::SearchEngineType search_engine, std::size_t property_index) {
    return construct_instance(search_engine, *constructedModel, *constructedModel->get_property(property_index));
}

std::unique_ptr<Expression> PLAJA::ModelConstructorForTests::parse_exp_str(const std::string& exp_str) const {
    auto expr = cachedParser->parse_ExpStr(exp_str);
    SEMANTICS_CHECKER::check_semantics(expr.get());
    SET_VARS::set_var_index_by_id(*constructedModel, *expr);
    return expr;
}