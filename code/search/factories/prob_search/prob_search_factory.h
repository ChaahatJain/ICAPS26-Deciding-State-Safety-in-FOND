//
// This file is part of the PlaJA code base.
// Copyright (c) (2019 - 2022) Marcel Vinzent.
// See README.md in the top-level directory for licensing information.
//

#ifndef PLAJA_PROB_SEARCH_FACTORY_H
#define PLAJA_PROB_SEARCH_FACTORY_H

#include "../search_engine_factory.h"

class ProbSearchFactory: public SearchEngineFactory {

protected:
    explicit ProbSearchFactory(SearchEngineType engine_type);

public:
    ~ProbSearchFactory() override;

    void supports_model(const Model& model) const override;

    void add_options(PLAJA::OptionParser& option_parser) const override;
    void print_options() const override;
};

#endif //PLAJA_PROB_SEARCH_FACTORY_H
