//
// This file is part of the PlaJA code base.
// Copyright (c) (2019 - 2020) Marcel Vinzent.
// See README.md in the top-level directory for licensing information.
//

#ifndef PLAJA_LRTDP_FACTORY_H
#define PLAJA_LRTDP_FACTORY_H

#include "prob_search_factory.h"

class LRTDP_Factory: public ProbSearchFactory {
public:
    LRTDP_Factory();
    ~LRTDP_Factory() override;
    DELETE_CONSTRUCTOR(LRTDP_Factory)

    static std::unique_ptr<SearchEngine> construct_LRTDP(const PLAJA::Configuration& config);
    static std::unique_ptr<SearchEngine> construct_LRTDP(const PLAJA::Configuration& config, SearchEngine* parent);

    [[nodiscard]] std::unique_ptr<SearchEngine> construct(const PLAJA::Configuration& config) const override;
};

#endif //PLAJA_LRTDP_FACTORY_H
