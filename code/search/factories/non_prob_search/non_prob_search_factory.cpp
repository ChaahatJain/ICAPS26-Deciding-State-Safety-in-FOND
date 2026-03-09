//
// This file is part of the PlaJA code base.
// Copyright (c) (2019 - 2021) Marcel Vinzent.
// See README.md in the top-level directory for licensing information.
//

#include "non_prob_search_factory.h"
#include "../../../include/factory_config_const.h"
#include "../../../option_parser/option_parser_aux.h"
#include "../../../option_parser/plaja_options.h"
#include "../../non_prob_search/breadth_first_search.h"
#include "../../non_prob_search/space_explorer.h"
#include "../../non_prob_search/nn_explorer.h"
#include "../../non_prob_search/restrictions_checker_explicit.h"
#include "../../information/property_information.h"
#include "../configuration.h"
#include "non_prob_search_options.h"

#ifdef USE_Z3

namespace PLAJA_OPTION::SMT_SOLVER_Z3 {
    extern void add_options(PLAJA::OptionParser& option_parser);
    extern void print_options();
}

#endif

/**********************************************************************************************************************/

NonProbSearchFactoryBase::NonProbSearchFactoryBase(SearchEngineFactory::SearchEngineType search_engine):
    SearchEngineFactory(search_engine) {}

NonProbSearchFactoryBase::~NonProbSearchFactoryBase() = default;

void NonProbSearchFactoryBase::supports_model(const Model& model) const {
    RestrictionsCheckerExplicit::check_restrictions(&model, false);
}

void NonProbSearchFactoryBase::add_options(PLAJA::OptionParser& option_parser) const {
    OPTION_PARSER::add_value_option(option_parser, PLAJA_OPTION::savePath);
    OPTION_PARSER::add_value_option(option_parser, PLAJA_OPTION::saveStateSpace);
    OPTION_PARSER::add_bool_option(option_parser, PLAJA_OPTION::computeGoalPaths, PLAJA_OPTION_DEFAULT::computeGoalPaths);
    OPTION_PARSER::add_bool_option(option_parser, PLAJA_OPTION::suppressStateSpace, PLAJA_OPTION_DEFAULT::suppressStateSpace);
    PLAJA_OPTION::add_initial_state_enum(option_parser);
    OPTION_PARSER::add_bool_option(option_parser, PLAJA_OPTION::search_per_start, PLAJA_OPTION_DEFAULT::search_per_start);
#ifdef USE_Z3
    PLAJA_OPTION::SMT_SOLVER_Z3::add_options(option_parser);
#endif
}

void NonProbSearchFactoryBase::print_options() const {
    OPTION_PARSER::print_bool_option(PLAJA_OPTION::computeGoalPaths, PLAJA_OPTION_DEFAULT::computeGoalPaths, "Compute goal paths for all states (if supported).");
    PLAJA_OPTION::print_initial_state_enum();
    PLAJA_OPTION::print_search_per_start();
#ifdef USE_Z3
    PLAJA_OPTION::SMT_SOLVER_Z3::print_options();
#endif
}

/**********************************************************************************************************************/

ExplorerFactory::ExplorerFactory():
    NonProbSearchFactoryBase(SearchEngineFactory::ExplorerType) {}

ExplorerFactory::~ExplorerFactory() = default;

std::unique_ptr<SearchEngine> ExplorerFactory::construct(const PLAJA::Configuration& config) const { return std::make_unique<SpaceExplorer>(config); }

bool ExplorerFactory::is_property_based() const { return false; }

bool ExplorerFactory::supports_property_sub(const PropertyInformation& /*prop_info*/) const { return true; }

/**********************************************************************************************************************/

#ifdef BUILD_NON_PROB_SEARCH

BFSFactory::BFSFactory():
    NonProbSearchFactoryBase(SearchEngineFactory::BfsType) {}

BFSFactory::~BFSFactory() = default;

std::unique_ptr<SearchEngine> BFSFactory::construct(const PLAJA::Configuration& config) const { return std::make_unique<BFSearch>(config); }

bool BFSFactory::supports_property_sub(const PropertyInformation& prop_info) const { return prop_info.get_reach(); }

/**********************************************************************************************************************/

NNExplorerFactory::NNExplorerFactory():
    NonProbSearchFactoryBase(SearchEngineFactory::NnExplorerType) {}

NNExplorerFactory::~NNExplorerFactory() = default;

std::unique_ptr<SearchEngine> NNExplorerFactory::construct(const PLAJA::Configuration& config) const { return std::make_unique<NN_Explorer>(config); }

bool NNExplorerFactory::supports_property_sub(const PropertyInformation& prop_info) const { return prop_info.get_reach(); }

#endif

/**********************************************************************************************************************/

namespace SEARCH_ENGINE_FACTORY {

    std::unique_ptr<SearchEngineFactory> construct_explorer_factory() { return std::make_unique<ExplorerFactory>(); }

#ifdef BUILD_NON_PROB_SEARCH

    std::unique_ptr<SearchEngineFactory> construct_bfs_factory() { return std::make_unique<BFSFactory>(); }

    std::unique_ptr<SearchEngineFactory> construct_nn_explorer_factory() { return std::make_unique<NNExplorerFactory>(); }

#endif

}