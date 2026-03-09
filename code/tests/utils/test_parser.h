//
// This file is part of the PlaJA code base.
// Copyright (c) (2019 - 2020) Marcel Vinzent.
// See README.md in the top-level directory for licensing information.
//

#ifndef PLAJA_TEST_PARSER_H
#define PLAJA_TEST_PARSER_H

#include "../../option_parser/plaja_options.h"
#include "../../parser/ast/expression/expression.h"
#include "../../parser/ast_forward_declarations.h"
#include "../../search/factories/configuration.h"
#include "../../search/factories/search_engine_factory.h"
#include "../../search/fd_adaptions/search_engine.h"
#include "../../search/information/forward_information.h"
#include "../../globals.h"

class Parser;

namespace OPTION_PARSER { extern void set_suppress_known_option_check(bool value); }

namespace PLAJA { // Namespace is to be friend of PLAJA::OptionParser.

    struct ModelConstructorForTests {
        PLAJA::OptionParser optionParser;
        PLAJA::Configuration config;
        std::unique_ptr<Parser> cachedParser;
        std::unique_ptr<Model> constructedModel;
        std::unique_ptr<PropertyInformation> cachedPropertyInfo;
        // std::unique_ptr<PLAJA::Configuration> cachedConfig; // No need to cache config used during engine construction?

    public:

        ModelConstructorForTests();

        ~ModelConstructorForTests();

        DELETE_CONSTRUCTOR(ModelConstructorForTests)

        inline PLAJA::Configuration& get_config() { return config; }

        void set_constant(const std::string& name, const std::string& value);

        void set_option_parser(const SearchEngineFactory* factory);

        void construct_model(const std::string& filename);

        std::unique_ptr<PropertyInformation> analyze_property(std::size_t property_index) const;

        void load_property(std::size_t property_index);

        std::unique_ptr<SearchEngine> construct_instance(SearchEngineFactory::SearchEngineType search_engine, std::size_t property_index);

        template<typename Engine_t>
        inline std::unique_ptr<Engine_t> construct_instance_cast(SearchEngineFactory::SearchEngineType search_engine, std::size_t property_index) {
            return PLAJA_UTILS::cast_unique<Engine_t>(construct_instance(search_engine, property_index));
        }

        std::unique_ptr<SearchEngine> construct_instance(SearchEngineFactory::SearchEngineType search_engine, const Model& model, const Property& property);

        template<typename Engine_t>
        inline std::unique_ptr<Engine_t> construct_instance_cast(SearchEngineFactory::SearchEngineType search_engine, const Model& model, const Property& property) {
            return PLAJA_UTILS::cast_unique<Engine_t>(construct_instance(search_engine, model, property));
        }

        [[nodiscard]] std::unique_ptr<Expression> parse_exp_str(const std::string& exp_str) const;

    };

}

#endif //PLAJA_TEST_PARSER_H
