//
// This file is part of the PlaJA code base.
// Copyright (c) (2019 - 2022) Marcel Vinzent.
// See README.md in the top-level directory for licensing information.
//

#ifndef PLAJA_SEARCH_ENGINE_CONFIG_PA_H
#define PLAJA_SEARCH_ENGINE_CONFIG_PA_H

#include "../configuration.h"
#include "pa_options.h"

class SearchEngineConfigPA: public PLAJA::Configuration {
public:
    SearchEngineConfigPA(const PLAJA::OptionParser& option_parser);
    SearchEngineConfigPA();
    SearchEngineConfigPA(const SearchEngineConfigPA& other);
    ~SearchEngineConfigPA() override;
};

#endif //PLAJA_SEARCH_ENGINE_CONFIG_PA_H
