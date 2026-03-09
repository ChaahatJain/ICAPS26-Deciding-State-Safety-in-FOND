//
// This file is part of the PlaJA code base.
// Copyright (c) (2019 - 2021) Marcel Vinzent.
// See README.md in the top-level directory for licensing information.
//

#ifndef PLAJA_PA_FACTORY_H
#define PLAJA_PA_FACTORY_H

#include "../smt_base/smt_base_factory.h"

class PABaseFactory: public SMTBaseFactory {
protected:
    void add_options(PLAJA::OptionParser& option_parser) const override;
    void print_options() const override;

    [[nodiscard]] bool supports_property_sub(const PropertyInformation& prop_info) const override;

    std::unique_ptr<PLAJA::Configuration> init_configuration(const PLAJA::OptionParser& option_parser, const PropertyInformation* property_info) const override;

    explicit PABaseFactory(SearchEngineFactory::SearchEngineType engine_type);
public:
    ~PABaseFactory() override = 0;
    DELETE_CONSTRUCTOR(PABaseFactory)
};

class PAFactory: public PABaseFactory {
public:
    PAFactory();
    ~PAFactory() override;
    DELETE_CONSTRUCTOR(PAFactory)

    [[nodiscard]] std::unique_ptr<SearchEngine> construct(const PLAJA::Configuration& config) const override;
};

class SearchPAPathFactory: public PABaseFactory {
private:
    [[nodiscard]] bool supports_property_sub(const PropertyInformation& prop_info) const override;
public:
    SearchPAPathFactory();
    ~SearchPAPathFactory() override;
    DELETE_CONSTRUCTOR(SearchPAPathFactory)

    [[nodiscard]] std::unique_ptr<SearchEngine> construct(const PLAJA::Configuration& config) const override;
};

class PACegarFactory: public PABaseFactory {
private:
    [[nodiscard]] bool supports_property_sub(const PropertyInformation& prop_info) const override;
public:
    PACegarFactory();
    ~PACegarFactory() override;
    DELETE_CONSTRUCTOR(PACegarFactory)

    void supports_model(const Model& model) const override;

    void add_options(PLAJA::OptionParser& option_parser) const override;
    void print_options() const override;

    [[nodiscard]] std::unique_ptr<SearchEngine> construct(const PLAJA::Configuration& config) const override;
};

class ProblemInstanceCheckerPaFactory: public PABaseFactory {
private:
    [[nodiscard]] bool supports_property_sub(const PropertyInformation& prop_info) const override;

public:
    ProblemInstanceCheckerPaFactory();
    ~ProblemInstanceCheckerPaFactory() override;
    DELETE_CONSTRUCTOR(ProblemInstanceCheckerPaFactory)

    [[nodiscard]] std::unique_ptr<SearchEngine> construct(const PLAJA::Configuration& config) const override;
};

#endif //PLAJA_PA_FACTORY_H
