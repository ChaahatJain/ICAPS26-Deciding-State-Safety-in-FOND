//
// This file is part of the PlaJA code base.
// Copyright (c) (2019 - 2021) Marcel Vinzent.
// See README.md in the top-level directory for licensing information.
//

#ifndef PLAJA_FACTORY_CONFIG_CONST_H
#define PLAJA_FACTORY_CONFIG_CONST_H

#include "factory_config.h"

namespace PLAJA_GLOBAL {

#ifdef BUILD_PA
    constexpr bool buildPA = true;
#else
    constexpr bool buildPA = false;
#endif

#ifdef BUILD_PROB_SEARCH
    constexpr bool buildProbSearch = true;
#else
    constexpr bool buildProbSearch = false;
#endif

#ifdef BUILD_BMC
    constexpr bool buildBMC = true;
#else
    constexpr bool buildBMC = false;
#endif

#ifdef BUILD_INVARIANT_STRENGTHENING
    constexpr bool buildInvariantStrengthening = true;
#else
    constexpr bool buildInvariantStrengthening = false;
#endif

#ifdef BUILD_FAULT_ANALYSIS
    constexpr bool buildFaultAnalysis = true;
#else
    constexpr bool buildFaultAnalysis = false;
#endif

#ifdef BUILD_POLICY_LEARNING
    constexpr bool buildPolicyLearning = true;
#else
    constexpr bool buildPolicyLearning = false;
#endif

#ifdef BUILD_TO_NUXMV
    constexpr bool buildToNuxmv = true;
#else
    constexpr bool buildToNuxmv = false;
#endif
#ifdef BUILD_SAFE_START_GENERATOR
    constexpr bool buildSafeStartGenerator = true;
#else
    constexpr bool buildSafeStartGenerator = false;
#endif



//

#ifdef USE_Z3
    constexpr bool useZ3 = true;
#else
    constexpr bool useZ3 = false;
#endif

#ifdef USE_MARABOU
    constexpr bool useMarabou = true;
#else
    constexpr bool useMarabou = false;
#endif

#ifdef USE_TORCH
    constexpr bool useTorch = true;
#else
    constexpr bool useTorch = false;
#endif

#ifdef USE_VERITAS
    constexpr bool useVeritas = true;
#else
    constexpr bool useVeritas = false;
#endif

}

#endif //PLAJA_FACTORY_CONFIG_CONST_H
