//
// This file is part of the PlaJA code base.
// Copyright (c) (2019 - 2020) Marcel Vinzent.
// See README.md in the top-level directory for licensing information.
//

#ifndef PLAJA_SEARCH_ENGINE_FACTORY_H
#define PLAJA_SEARCH_ENGINE_FACTORY_H

#include <memory>
#include <unordered_map>
#include "../../include/factory_config_const.h" // Preprocessor flags.
#include "../../parser/ast/forward_ast.h"
#include "../../utils/default_constructors.h"
#include "../information/forward_information.h"
#include "forward_factories.h"
#include "../information/property_information.h"


class SearchEngine;

// extern:
namespace SEARCH_ENGINE_FACTORY { extern void print_supported_engines(); }

class SearchEngineFactory {

public:
    enum SearchEngineType {
        BfsType,
        ExplorerType,
        NnExplorerType,
        LrtdpType,
        FretLrtdpType,
        PaType,
        PaPathType,
        PaCegarType,
        ProblemInstanceCheckerPaType,
        BmcType,
        InvariantStrengtheningType,
        QlAgentType,
        ReductionAgentType,
        ToNuxmvType,
        FaultAnalysisType,
        SafeStartGeneratorType,
    };

private:
    static SearchEngineType determine_search_engine_type(const std::string& search_engine_type);
    static SearchEngineType determine_search_engine_type(const PLAJA::OptionParser& option_parser);
    void print_construction_info() const;

protected:
    const SearchEngineType engineType;

    /* Internal auxiliaries. */
    [[nodiscard]] bool supports_reachability_property() const;
    [[nodiscard]] bool supports_probabilistic_property() const;
    [[nodiscard]] bool requires_policy() const;
    [[nodiscard]] virtual bool supports_property_sub(const PropertyInformation& property_info) const; // internal virtual method to check whether the property is supported by the engine?

    [[nodiscard]] virtual std::unique_ptr<SearchEngine> construct(const PLAJA::Configuration& configuration) const = 0;

    explicit SearchEngineFactory(SearchEngineType engine_type);
public:
    virtual ~SearchEngineFactory() = 0;
    DELETE_CONSTRUCTOR(SearchEngineFactory)

    virtual void add_options(PLAJA::OptionParser& option_parser) const;
    virtual void print_options() const;
    [[nodiscard]] const std::string& to_string() const;

    static std::unique_ptr<SearchEngineFactory> construct_factory(SearchEngineType engine_type);
    static std::unique_ptr<SearchEngineFactory> construct_factory(const PLAJA::OptionParser& option_parser);

    virtual void adapt_configuration(PLAJA::Configuration& config, const PropertyInformation* property_info) const;
    virtual std::unique_ptr<PLAJA::Configuration> init_configuration(const PLAJA::OptionParser& option_parser, const PropertyInformation* property_info) const;
    [[nodiscard]] std::unique_ptr<SearchEngine> construct_engine(const PLAJA::Configuration& configuration) const;

    /* Support. */
    virtual void supports_model(const Model& model) const; // Is the model supported by the set engine?
    [[nodiscard]] virtual bool is_property_based() const; // Is the engine a property-based engine?
    void supports_property(const PropertyInformation& property_info) const; // Is the model supported by the property?


};

namespace SEARCH_ENGINE_FACTORY {

    [[nodiscard]] extern std::unique_ptr<SearchEngineFactory> construct_explorer_factory();

#ifdef BUILD_NON_PROB_SEARCH
    [[nodiscard]] extern std::unique_ptr<SearchEngineFactory> construct_nn_explorer_factory();
    [[nodiscard]] extern std::unique_ptr<SearchEngineFactory> construct_bfs_factory();
#endif

#ifdef BUILD_PA
    [[nodiscard]] extern std::unique_ptr<SearchEngineFactory> construct_problem_instance_checker_pa_factory();
#endif

#ifdef BUILD_POLICY_LEARNING
    [[nodiscard]] extern std::unique_ptr<SearchEngineFactory> construct_ql_agent_factory();
    [[nodiscard]] extern std::unique_ptr<SearchEngineFactory> construct_reduction_agent_factory();
#endif

#ifdef BUILD_TO_NUXMV
    [[nodiscard]] extern std::unique_ptr<SearchEngineFactory> construct_to_nuxmv_factory();
#endif
#ifdef BUILD_SAFE_START_GENERATOR
    [[nodiscard]] extern std::unique_ptr<SearchEngineFactory> construct_safe_start_generator_factory();
#endif


}

#endif //PLAJA_SEARCH_ENGINE_FACTORY_H
