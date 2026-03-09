//
// This file is part of the PlaJA code base.
// Copyright (c) (2019 - 2021) Marcel Vinzent.
// See README.md in the top-level directory for licensing information.
//

#ifndef PLAJA_COMPILER_CONFIG_CONST_H
#define PLAJA_COMPILER_CONFIG_CONST_H

#include "compiler_config.h"

/** const expr in C++ for easier usage */

namespace PLAJA_GLOBAL {

#ifndef NDEBUG
    constexpr bool debug = true;
#else
    constexpr bool debug = false;
#endif

#ifndef RUNTIME_CHECKS
    constexpr bool runtimeChecks = true;
#else
    constexpr bool runtimeChecks = false;
#endif

#ifdef IS_LINUX
    constexpr bool isLinux = true;
#else
    constexpr bool isLinux = false;
#endif

#ifdef IS_OSX
    constexpr bool isOSX = true;
#else
    constexpr bool isOSX = false;
#endif

#ifdef IS_WINDOWS
    constexpr bool isWindows = true
#else
    constexpr bool isWindows = false;
#endif

#ifdef IS_GCC
    constexpr bool isGCC = true;
#else
    constexpr bool isGCC = false;
#endif

#ifdef IS_CC
    constexpr bool isCC = true;
#else
    constexpr bool isCC = false;
#endif

#ifdef IS_APPLE_CC
    constexpr bool isCCApple = true;
#else
    constexpr bool isCCApple = false;
#endif

#ifdef SUPPORT_GUROBI
    constexpr bool supportGurobi = true;
#else
    constexpr bool supportGurobi = false;
#endif

}

#endif //PLAJA_COMPILER_CONFIG_CONST_H
