//
// This file is part of the PlaJA code base.
// Copyright (c) (2019 - 2020) Marcel Vinzent.
// See README.md in the top-level directory for licensing information.
//

#ifndef PLAJA_FRET_FACTORY_H
#define PLAJA_FRET_FACTORY_H

#include "prob_search_factory.h"

class FRET_Factory: public ProbSearchFactory {
private:
    SearchEngineFactory::SearchEngineType subEngineType;

public:
    explicit FRET_Factory(SearchEngineFactory::SearchEngineType sub_engine_type);
    ~FRET_Factory() override;
    DELETE_CONSTRUCTOR(FRET_Factory)

    [[nodiscard]] std::unique_ptr<SearchEngine> construct(const PLAJA::Configuration& config) const override;
};

#endif //PLAJA_FRET_FACTORY_H
