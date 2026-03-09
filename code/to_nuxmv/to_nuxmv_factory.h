//
// This file is part of the PlaJA code base.
// Copyright (c) (2019 - 2021) Marcel Vinzent.
// See README.md in the top-level directory for licensing information.
//

#ifndef PLAJA_TO_NUXMV_FACTORY_H
#define PLAJA_TO_NUXMV_FACTORY_H

#include "../utils/default_constructors.h"
#include "../search/factories/search_engine_factory.h"

class ToNuxmvFactory final: public SearchEngineFactory {
private:
    void supports_model(const Model& model) const override;
    [[nodiscard]] bool supports_property_sub(const PropertyInformation& prop_info) const override;

public:
    ToNuxmvFactory();
    ~ToNuxmvFactory() override;
    DELETE_CONSTRUCTOR(ToNuxmvFactory)

    void add_options(PLAJA::OptionParser& option_parser) const override;
    void print_options() const override;

    [[nodiscard]] std::unique_ptr<SearchEngine> construct(const PLAJA::Configuration& config) const override;
};

#endif //PLAJA_TO_NUXMV_FACTORY_H
