//
// This file is part of the PlaJA code base.
// Copyright (c) (2019 - 2021) Marcel Vinzent.
// See README.md in the top-level directory for licensing information.
//

#ifndef PLAJA_SMT_BASE_FACTORY_H
#define PLAJA_SMT_BASE_FACTORY_H

#include "../search_engine_factory.h"

class SMTBaseFactory: public SearchEngineFactory {
protected:
    void add_options(PLAJA::OptionParser& option_parser) const override;
    static void print_options_base();

    [[nodiscard]] bool supports_property_sub(const PropertyInformation& prop_info) const override;

    explicit SMTBaseFactory(SearchEngineFactory::SearchEngineType engine_type);
public:
    ~SMTBaseFactory() override = 0;
    DELETE_CONSTRUCTOR(SMTBaseFactory)

    void supports_model(const Model& model) const override;
};

#endif //PLAJA_SMT_BASE_FACTORY_H
