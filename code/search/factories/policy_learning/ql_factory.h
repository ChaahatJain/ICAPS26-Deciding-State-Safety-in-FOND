//
// This file is part of the PlaJA code base.
// Copyright (c) (2019 - 2021) Marcel Vinzent.
// See README.md in the top-level directory for licensing information.
//

#ifndef PLAJA_QL_FACTORY_H
#define PLAJA_QL_FACTORY_H

#include "../search_engine_factory.h"

class QlFactory: public SearchEngineFactory {

private:
    [[nodiscard]] bool supports_property_sub(const PropertyInformation& prop_info) const override;
    
public:
    QlFactory();
    ~QlFactory() override;
    DELETE_CONSTRUCTOR(QlFactory)

    void add_options(PLAJA::OptionParser& option_parser) const override;
    void print_options() const override;
    void add_fa_options(PLAJA::OptionParser& option_parser) const;
    void print_fa_options() const;

    [[nodiscard]] std::unique_ptr<SearchEngine> construct(const PLAJA::Configuration& config) const override;
};

#endif //PLAJA_QL_FACTORY_H
