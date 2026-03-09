//
// This file is part of the PlaJA code base.
// Copyright (c) (2019 - 2022) Marcel Vinzent.
// See README.md in the top-level directory for licensing information.
//

#include "search_engine_config_pa.h"
#include "../../../include/ct_config_const.h"

SearchEngineConfigPA::SearchEngineConfigPA(const PLAJA::OptionParser& option_parser):
    PLAJA::Configuration(option_parser) {
    set_flag(PLAJA_OPTION::cacheWitnesses, false);
}

SearchEngineConfigPA::SearchEngineConfigPA():
    PLAJA::Configuration() {
    set_flag(PLAJA_OPTION::cacheWitnesses, false);
}

SearchEngineConfigPA::SearchEngineConfigPA(const SearchEngineConfigPA& other):
    PLAJA::Configuration(other) {
    set_flag(PLAJA_OPTION::cacheWitnesses, false);
}

SearchEngineConfigPA::~SearchEngineConfigPA() = default;