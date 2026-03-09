//
// This file is part of the PlaJA code base.
// Copyright (c) (2019 - 2021) Marcel Vinzent.
// See README.md in the top-level directory for licensing information.
//

#ifndef PLAJA_NON_PROB_SEARCH_FACTORY_H
#define PLAJA_NON_PROB_SEARCH_FACTORY_H

#include "../search_engine_factory.h"

class NonProbSearchFactoryBase: public SearchEngineFactory {
protected:
    explicit NonProbSearchFactoryBase(SearchEngineFactory::SearchEngineType search_engine);
public:
    ~NonProbSearchFactoryBase() override;
    DELETE_CONSTRUCTOR(NonProbSearchFactoryBase)

    void supports_model(const Model& model) const override;

    void add_options(PLAJA::OptionParser& option_parser) const override;
    void print_options() const override;
};

class ExplorerFactory: public NonProbSearchFactoryBase {
private:
    [[nodiscard]] bool supports_property_sub(const PropertyInformation& prop_info) const override;
public:
    ExplorerFactory();
    ~ExplorerFactory() override;
    DELETE_CONSTRUCTOR(ExplorerFactory)

    [[nodiscard]] std::unique_ptr<SearchEngine> construct(const PLAJA::Configuration& config) const override;
    [[nodiscard]] bool is_property_based() const override;
};

#ifdef BUILD_NON_PROB_SEARCH

class BFSFactory: public NonProbSearchFactoryBase {
private:
    [[nodiscard]] bool supports_property_sub(const PropertyInformation& prop_info) const override;
public:
    BFSFactory();
    ~BFSFactory() override;
    DELETE_CONSTRUCTOR(BFSFactory)

    [[nodiscard]] std::unique_ptr<SearchEngine> construct(const PLAJA::Configuration& config) const override;
};

class NNExplorerFactory: public NonProbSearchFactoryBase {
private:
    [[nodiscard]] bool supports_property_sub(const PropertyInformation& prop_info) const override;
public:
    NNExplorerFactory();
    ~NNExplorerFactory() override;
    DELETE_CONSTRUCTOR(NNExplorerFactory)

    [[nodiscard]] std::unique_ptr<SearchEngine> construct(const PLAJA::Configuration& config) const override;
};

#endif

#endif //PLAJA_NON_PROB_SEARCH_FACTORY_H
