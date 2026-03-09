//
// This file is part of the PlaJA code base.
// Copyright (c) (2019 - 2020) Marcel Vinzent.
// See README.md in the top-level directory for licensing information.
//

#include "search_engine_factory.h"
#include "../../exception/option_parser_exception.h"
#include "../../exception/property_analysis_exception.h"
#include "../../option_parser/option_parser.h"
#include "../../option_parser/plaja_options.h"
#include "../fd_adaptions/search_engine.h"
#include "../information/property_information.h"
#include "configuration.h"
#include "safe_start_generator/safe_start_generator_factory.h"

#include <iostream>
#include <string>
#include <unordered_map>

#ifdef BUILD_PA

#include "predicate_abstraction/pa_factory.h"

#endif

#ifdef BUILD_PROB_SEARCH

#include "prob_search/fret_factory.h"
#include "prob_search/lrtdp_factory.h"

#endif

#ifdef BUILD_BMC

#include "bmc/bmc_factory.h"

#endif

#ifdef BUILD_INVARIANT_STRENGTHENING

#include "invariant_strengthening/inv_str_factory.h"

#endif

#ifdef BUILD_FAULT_ANALYSIS

#include "fault_analysis/fault_analysis_factory.h"

#endif

namespace SEARCH_ENGINE_FACTORY {

    const std::string searchEngineTypeToString[] {
        "BFS",
        "EXPLORER",
        "NN_EXPLORER",
        "LRTDP",
        "FRET/LRTDP",
        "PREDICATE_ABSTRACTION",
        "PA_PATH",
        "PA_CEGAR",
        "PROB_INST_PA",
        "BMC",
        "INVARIANT_STRENGTHENING",
        "QL_AGENT",
        "TO_NUXMV",
        "FAULT_ANALYSIS",
        "REDUCTION_AGENT",
        "SAFE_START_GENERATOR"}; // NOLINT(cert-err58-cpp)

    const std::unordered_map<std::string, SearchEngineFactory::SearchEngineType> stringToSearchEngineType {
        // NOLINT(cert-err58-cpp)
        {"EXPLORER", SearchEngineFactory::ExplorerType},
        {"BFS", SearchEngineFactory::BfsType},
        {"NN_EXPLORER", SearchEngineFactory::NnExplorerType}
#ifdef BUILD_PROB_SEARCH
        ,
        {"LRTDP", SearchEngineFactory::LrtdpType},
        {"FRET/LRTDP", SearchEngineFactory::FretLrtdpType}
#endif
#ifdef BUILD_PA
        ,
        {"PREDICATE_ABSTRACTION", SearchEngineFactory::PaType},
        {"PA_PATH", SearchEngineFactory::PaPathType},
        {"PA_CEGAR",
         SearchEngineFactory::
             PaCegarType} /* , { "PROB_INST_PA", SearchEngineFactory::ProblemInstanceCheckerPaType } */ // TODO: Latter is not implemented.
#endif
#ifdef BUILD_BMC
        ,
        {"BMC", SearchEngineFactory::BmcType}
#endif
#ifdef BUILD_INVARIANT_STRENGTHENING
        ,
        {"INV_STR", SearchEngineFactory::InvariantStrengtheningType}
#endif
#ifdef BUILD_POLICY_LEARNING
        , { "QL_AGENT", SearchEngineFactory::QlAgentType }, { "REDUCTION_AGENT", SearchEngineFactory::ReductionAgentType }
#endif
#ifdef BUILD_TO_NUXMV
        ,
        {"TO_NUXMV", SearchEngineFactory::ToNuxmvType}
#endif
#ifdef BUILD_FAULT_ANALYSIS
        ,
        {"FAULT_ANALYSIS", SearchEngineFactory::FaultAnalysisType}
#endif
#ifdef BUILD_SAFE_START_GENERATOR
        ,
        {"START_GEN", SearchEngineFactory::SafeStartGeneratorType}
#endif

    };

    void print_supported_engines() {
        std::cout << "\tSupported engines:";
        for (const auto& engine_type_str: searchEngineTypeToString) {
            if (stringToSearchEngineType.count(engine_type_str)) {
                std::cout << PLAJA_UTILS::spaceString << engine_type_str;
            }
        }
        std::cout << PLAJA_UTILS::dotString << std::endl;
    }

#if 0 // deprecated for now ...
    void add_options(PLAJA::OptionParser& option_parser) {
#ifdef BUILD_PROB_SEARCH
            PROB_SEARCH_FACTORY::add_options(option_parser);
#endif
#ifdef BUILD_PA
            PA_FACTORY::add_options(option_parser);
#endif
#ifdef BUILD_BMC
            BMC_FACTORY::add_options(option_parser);
#endif
#ifdef BUILD_INVARIANT_STRENGTHENING
            INV_STR_FACTORY::add_options(option_parser);
#endif
#ifdef BUILD_POLICY_LEARNING
            QL_FACTORY::add_options(option_parser);
#endif
    }

    void print_options() {
#ifdef BUILD_PROB_SEARCH
        PROB_SEARCH_FACTORY::print_options();
#endif
#ifdef BUILD_PA
        PA_FACTORY::print_options();
#endif
#ifdef BUILD_BMC
        BMC_FACTORY::print_options();
#endif
#ifdef BUILD_INVARIANT_STRENGTHENING
        BMC_FACTORY::print_options();
#endif
#ifdef BUILD_POLICY_LEARNING
        QL_FACTORY::print_options();
#endif
    }
#endif

} // namespace SEARCH_ENGINE_FACTORY

SearchEngineFactory::SearchEngineFactory(SearchEngineType engine_type):
    engineType(engine_type) {}

SearchEngineFactory::~SearchEngineFactory() = default;

/* */

SearchEngineFactory::SearchEngineType SearchEngineFactory::determine_search_engine_type(
    const std::string& search_engine_type) {
    if (SEARCH_ENGINE_FACTORY::stringToSearchEngineType.count(search_engine_type) > 0) {
        return SEARCH_ENGINE_FACTORY::stringToSearchEngineType.at(search_engine_type);
    } else {
        throw OptionParserException("unknown search engine: " + search_engine_type);
    }
}

SearchEngineFactory::SearchEngineType SearchEngineFactory::determine_search_engine_type(
    const PLAJA::OptionParser& option_parser) {
    if (option_parser.has_value_option(PLAJA_OPTION::engine)) {
        return determine_search_engine_type(option_parser.get_value_option_string(PLAJA_OPTION::engine));
    } else {
        return SearchEngineType::ExplorerType;
    }
}

void SearchEngineFactory::print_construction_info() const {
    std::cout << "Constructing " << this->to_string() << " ..." << std::endl;
}

void SearchEngineFactory::add_options(PLAJA::OptionParser& /*option_parser*/) const {}

void SearchEngineFactory::print_options() const {}

const std::string& SearchEngineFactory::to_string() const {
    return SEARCH_ENGINE_FACTORY::searchEngineTypeToString[engineType];
}

//

std::unique_ptr<SearchEngineFactory> SearchEngineFactory::construct_factory(SearchEngineType engine_type) {

    switch (engine_type) {
        case SearchEngineType::ExplorerType: return SEARCH_ENGINE_FACTORY::construct_explorer_factory();
#ifdef BUILD_NON_PROB_SEARCH
        case SearchEngineType::BfsType: return SEARCH_ENGINE_FACTORY::construct_bfs_factory();
        case SearchEngineType::NnExplorerType: return SEARCH_ENGINE_FACTORY::construct_nn_explorer_factory();
#endif
#ifdef BUILD_PROB_SEARCH
        case SearchEngineType::LrtdpType: return std::make_unique<LRTDP_Factory>();
        case SearchEngineType::FretLrtdpType: return std::make_unique<FRET_Factory>(SearchEngineType::LrtdpType);
#endif
#ifdef BUILD_PA
        case SearchEngineType::PaType: return std::make_unique<PAFactory>();
        case SearchEngineType::PaPathType: return std::make_unique<SearchPAPathFactory>();
        case SearchEngineType::PaCegarType: return std::make_unique<PACegarFactory>();
        case SearchEngineType::ProblemInstanceCheckerPaType:
            return SEARCH_ENGINE_FACTORY::construct_problem_instance_checker_pa_factory();
#endif
#ifdef BUILD_BMC
        case SearchEngineType::BmcType: return std::make_unique<BMCFactory>();
#endif
#ifdef BUILD_INVARIANT_STRENGTHENING
        case SearchEngineType::InvariantStrengtheningType: return std::make_unique<InvStrFactory>();
#endif
#ifdef BUILD_POLICY_LEARNING
        case SearchEngineType::QlAgentType: return SEARCH_ENGINE_FACTORY::construct_ql_agent_factory();
        case SearchEngineType::ReductionAgentType: return SEARCH_ENGINE_FACTORY::construct_reduction_agent_factory();
#endif
#ifdef BUILD_TO_NUXMV
        case SearchEngineType::ToNuxmvType: return SEARCH_ENGINE_FACTORY::construct_to_nuxmv_factory();
#endif
#ifdef BUILD_FAULT_ANALYSIS
        case SearchEngineType::FaultAnalysisType: return std::make_unique<FaultAnalysisFactory>();
#endif
#ifdef BUILD_SAFE_START_GENERATOR
        case SafeStartGeneratorType: return SEARCH_ENGINE_FACTORY::construct_safe_start_generator_factory();
#endif
        default: PLAJA_ABORT
    }
}

std::unique_ptr<SearchEngineFactory> SearchEngineFactory::construct_factory(const PLAJA::OptionParser& option_parser) {
    return construct_factory(determine_search_engine_type(option_parser));
}

void SearchEngineFactory::adapt_configuration(PLAJA::Configuration& config, const PropertyInformation* property_info)
    const {
    if (property_info) {
        config.set_sharable_const(PLAJA::SharableKey::PROP_INFO, property_info);
        config.set_sharable_const(PLAJA::SharableKey::MODEL, property_info->get_model());
    }
}

std::unique_ptr<PLAJA::Configuration> SearchEngineFactory::init_configuration(
    const PLAJA::OptionParser& option_parser,
    const PropertyInformation* property_info) const {
    auto config = std::make_unique<PLAJA::Configuration>(option_parser);
    adapt_configuration(*config, property_info);
    return config;
}

std::unique_ptr<SearchEngine> SearchEngineFactory::construct_engine(const PLAJA::Configuration& configuration) const {
    print_construction_info();
    return construct(configuration);
}

void SearchEngineFactory::supports_model(const Model& /*model*/) const { /* NOTHING */ }

bool SearchEngineFactory::is_property_based() const { return true; } // so far only exception is EXPLORER

void SearchEngineFactory::supports_property(const PropertyInformation& property_info) const {
    if (property_info.is_non_property() and is_property_based()) {
        throw PropertyAnalysisException(property_info.get_property_name(), to_string(), PLAJA_UTILS::emptyString);
    }
    if (property_info.is_reach_property() and not supports_reachability_property()) {
        throw PropertyAnalysisException(property_info.get_property_name(), to_string(), PLAJA_UTILS::emptyString);
    }
    if (property_info.is_prob_property() and not supports_probabilistic_property()) {
        throw PropertyAnalysisException(property_info.get_property_name(), to_string(), PLAJA_UTILS::emptyString);
    }
    if (requires_policy() and not(property_info.get_interface())) {
        throw PropertyAnalysisException(property_info.get_property_name(), to_string(), "Policy required.");
    }
    if (not supports_property_sub(property_info)) {
        throw PropertyAnalysisException(property_info.get_property_name(), to_string(), PLAJA_UTILS::emptyString);
    }
}

// internal auxiliaries

bool SearchEngineFactory::supports_reachability_property() const {
    switch (engineType) {
        case SearchEngineType::BfsType:
        case SearchEngineType::ExplorerType:
#ifdef BUILD_PA
        case SearchEngineType::PaCegarType: //  cegar will use the goal for predicate generation
#endif
            return true;
        default: {
            return false;
        }
    }
}

bool SearchEngineFactory::supports_probabilistic_property() const {
    switch (engineType) {
        case SearchEngineType::ExplorerType:
#ifdef BUILD_PROB_SEARCH
        case SearchEngineType::LrtdpType:
        case SearchEngineType::FretLrtdpType:
#endif
#ifdef BUILD_PA
        case SearchEngineType::
            PaCegarType: // cegar will use the goal of FRET/LRTDP-properties we consider for predicate generation
#endif
            return true;
        default: {
            return false;
        }
    }
}

bool SearchEngineFactory::requires_policy() const {
    switch (engineType) {
        case SearchEngineType::NnExplorerType:
#ifdef BUILD_POLICY_LEARNING
        case SearchEngineType::QlAgentType:
        case SearchEngineType::ReductionAgentType:
#endif
            return true;
        default: {
            return false;
        }
    }
}

bool SearchEngineFactory::supports_property_sub(const PropertyInformation& /*prop_info*/) const { return false; }