//
// This file is part of the PlaJA code base.
// Copyright (c) (2019 - 2021) Marcel Vinzent.
// See README.md in the top-level directory for licensing information.
//

#ifndef PLAJA_MARABOU_CONFIG_CONST_H
#define PLAJA_MARABOU_CONFIG_CONST_H

#pragma GCC diagnostic push
#pragma ide diagnostic ignored "Simplify"

#include "marabou_config.h"

namespace PLAJA_GLOBAL {

    constexpr bool marabouHandleNumericIssues = true;

#ifdef MARABOU_DNC_SUPPORT
    constexpr bool marabouDNCSupport = true;
#else
    constexpr bool marabouDncSupport = false;
#endif

#ifdef MARABOU_DIS_BASELINE_SUPPORT
    constexpr bool marabouDisBaselineSupport = true;
#else
    constexpr bool marabouDisBaselineSupport = false;
#endif

#ifdef MARABOU_IGNORE_ARRAY_LEGACY
    constexpr bool marabouIgnoreArrayLegacy = true;
#else
    constexpr bool marabouIgnoreArrayLegacy = false;
#endif

}

#pragma GCC diagnostic pop

#endif// PLAJA_MARABOU_CONFIG_CONST_H