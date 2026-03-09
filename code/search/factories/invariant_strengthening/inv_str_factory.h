//
// This file is part of the PlaJA code base.
// Copyright (c) (2019 - 2021) Marcel Vinzent.
// See README.md in the top-level directory for licensing information.
//

#ifndef PLAJA_INV_STR_FACTORY_H
#define PLAJA_INV_STR_FACTORY_H

#include "../smt_base/smt_base_factory.h"

class InvStrFactory: public SMTBaseFactory {
private:
    [[nodiscard]] bool supports_property_sub(const PropertyInformation& prop_info) const override;

public:
    InvStrFactory();
    ~InvStrFactory() override;
    DELETE_CONSTRUCTOR(InvStrFactory)

    void add_options(PLAJA::OptionParser& option_parser) const override;
    void print_options() const override;

    [[nodiscard]] std::unique_ptr<SearchEngine> construct(const PLAJA::Configuration& config) const override;
};

#endif //PLAJA_INV_STR_FACTORY_H
