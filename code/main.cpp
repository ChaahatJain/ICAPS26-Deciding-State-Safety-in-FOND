//
// This file is part of the PlaJA code base.
// Copyright (c) (2019 - 2019) Marcel Vinzent.
// See README.md in the top-level directory for licensing information.
//

#include <memory>
#include "exception/not_supported_exception.h"
#include "exception/option_parser_exception.h"
#include "exception/semantics_exception.h"
#include "option_parser/option_parser.h"
#include "option_parser/plaja_options.h"
#include "parser/ast/model.h"
#include "parser/ast/property.h"
#include "search/factories/search_engine_factory.h"
#include "search/factories/configuration.h"
#include "search/fd_adaptions/timer.h"
#include "search/fd_adaptions/search_engine.h"
#include "search/information/property_information.h"
#include "utils/fd_imports/system.h"
#include "globals.h"


// extern:
namespace PARSER { extern std::unique_ptr<Model> parse(const std::string* filename, const PLAJA::OptionParser* option_parser); }
namespace SEMANTICS_CHECKER { extern void check_semantics(AstElement* ast_element); }

// for timing statistics:
double pre_processing_time { 0 }; // set by main, read by run_engine

// To make main function more readable and to reduce code redundancy:
inline void run_engine(const SearchEngineFactory& engine_factory, const PropertyInformation& property_info) {
    auto offset = (*PLAJA_GLOBAL::timer)();

    engine_factory.supports_property(property_info);
    auto config = engine_factory.init_configuration(*PLAJA_GLOBAL::optionParser, &property_info);
    auto engine = engine_factory.construct_engine(*config);
    engine->set_construction_time((*PLAJA_GLOBAL::timer)() - offset);
    engine->set_preprocessing_time(pre_processing_time);

    // set globals:
    PLAJA_GLOBAL::currentEngine = engine.get();

    engine->search();
    engine->update_statistics();
    engine->print_statistics_frame(true);
    engine->generate_csv();

    PLAJA_GLOBAL::currentEngine = nullptr;
}

int main(int argc, char** argv) {

    utils::register_event_handlers();

    auto time_offset = (*PLAJA_GLOBAL::timer)();

    PLAJA_LOG("Parsing commandline ...")
    PLAJA::OptionParser option_parser;
    PLAJA_GLOBAL::optionParser = &option_parser; // set globally
    std::unique_ptr<SearchEngineFactory> engine_factory; // factory ptr
    try {
        option_parser.load_commandline(argc, argv);
        option_parser.pre_load_engine();
        engine_factory = SearchEngineFactory::construct_factory(option_parser);
        engine_factory->add_options(option_parser);
        option_parser.parse_options();
    } catch (OptionParserException& e) {
        PLAJA_LOG(e.what())
        PLAJA_LOG("Use --help to show usage information.")
        return 1;
    }
    PLAJA_LOG("... parsing commandline finished.\n")

    if (option_parser.is_flag_set(PLAJA_OPTION::help)) {
        PLAJA::OptionParser::get_help();
        // engine-specific option (if not using default engine):
        if (option_parser.has_value_option(PLAJA_OPTION::engine)) { engine_factory->print_options(); }
        else { PLAJA_LOG("Set --engine <engine_type> to additionally include engine-specific options.\n"); }
        return 0;
    }

    // Parsing ...
    PLAJA_LOG("Parsing model ...")
    std::unique_ptr<Model> model = PARSER::parse(nullptr, &option_parser);
    if (!model) {
        PLAJA_LOG("... parsing failed.\n")
        return 1; // parsing failed
    }
    PLAJA_LOG("... parsing model finished.\n")

    // Store model globally for easy runtime checks and (debugging) outputs.
    // This field should be updated w.r.t to the currently considered model, e.g., when using abstractions.
    PLAJA_GLOBAL::currentModel = model.get();

    PLAJA_LOG("Executing static semantic checks ...")
#ifdef NDEBUG
    try {
#endif
    SEMANTICS_CHECKER::check_semantics(model.get());

    // compute further model information
    model->compute_model_information();
#ifdef NDEBUG
    } catch (SemanticsException& e) {
        PLAJA_LOG(PLAJA_UTILS::string_f("%s\n", e.what()))
        return 1;
    } catch (NotSupportedException& e) { // some are easier to catch here than at parse time
        PLAJA_LOG(PLAJA_UTILS::string_f("%s\n", e.what()))
        return 1;
    }
#endif
    PLAJA_LOG("... semantic checks passed.\n")

    // engine supports model?:
    try { engine_factory->supports_model(*model); }
    catch (NotSupportedException& e) {
        PLAJA_LOG(PLAJA_UTILS::string_f("%s\n", e.what()))
        return 1;
    }

    pre_processing_time = (*PLAJA_GLOBAL::timer)() - time_offset;

    // Some engines do not consider properties ...
    if (model->get_number_properties() == 0 && !engine_factory->is_property_based()) {
        const auto dummy = PropertyInformation::construct_property_information(PropertyInformation::NONE, nullptr, model.get());
        try { run_engine(*engine_factory, *dummy); }
        catch (PlajaException& e) {
            PLAJA_LOG(PLAJA_UTILS::string_f("%s\n", e.what()))
            return 1;
        }

        return 0;
    }
    // else:

    // Iterate over properties:
    const std::size_t l = model->get_number_properties();
    PLAJA_LOG_IF(l == 0, "Warning: 0 properties to check")
    for (std::size_t i = 0; i < l; ++i) { // for each property ...
#ifdef NEDBUG
        try {
#endif
        if (option_parser.has_value_option(PLAJA_OPTION::prop) and option_parser.get_option_value<int>(PLAJA_OPTION::prop, 0) != i) {
            PLAJA_LOG(PLAJA_UTILS::string_f("Skipping property %s.", model->get_property(i)->get_name().c_str()))
            continue;
        }

        // Check property:
        JANI_ASSERT(model->get_property(i))
        const auto property_info = PropertyInformation::analyse_property(*model->get_property(i), *model);
        PLAJA_LOG(PLAJA_UTILS::string_f("Checking property: %s ...", model->get_property(i)->get_name().c_str()))

        run_engine(*engine_factory, *property_info);

        PLAJA_LOG(PLAJA_UTILS::emptyString)
#ifdef NEDBUG
        } catch (PlajaException& e) {
            PLAJA_LOG( PLAJA_UTILS::string_f("%s\n", e.what()) )
            continue; // just try next property
        }
#endif
    }

    return 0;

}
