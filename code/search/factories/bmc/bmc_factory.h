//
// This file is part of the PlaJA code base.
// Copyright (c) (2019 - 2021) Marcel Vinzent.
// See README.md in the top-level directory for licensing information.
//

#ifndef PLAJA_BMC_FACTORY_H
#define PLAJA_BMC_FACTORY_H

#include "../../../utils/default_constructors.h"
#include "../smt_base/smt_base_factory.h"

class BMCFactory: public SMTBaseFactory {
private:
    [[nodiscard]] bool supports_property_sub(const PropertyInformation& prop_info) const override;

public:
    BMCFactory();
    ~BMCFactory() override;
    DELETE_CONSTRUCTOR(BMCFactory)

    void add_options(PLAJA::OptionParser& option_parser) const override;
    void print_options() const override;

    [[nodiscard]] std::unique_ptr<SearchEngine> construct(const PLAJA::Configuration& config) const override;
};

#endif //PLAJA_BMC_FACTORY_H
