//
// This file is part of PlaJA (2019 - 2024).
// Copyright (c) Marcel Vinzent (2024).
//

#ifndef PLAJA_CT_CONFIG_CONST_H
#define PLAJA_CT_CONFIG_CONST_H

#include "ct_config.h"
/* Special configs. */
#include "compiler_config_const.h"
#include "factory_config_const.h"
#include "marabou_config_const.h"
#include "pa_config_const.h"

namespace PLAJA_GLOBAL {

#ifdef ENABLE_APPLICABILITY_FILTERING
    constexpr bool enableApplicabilityFiltering(true);
#else
    constexpr bool enableApplicabilityFiltering(false);
#endif

#ifdef ENABLE_APPLICABILITY_CACHE
    constexpr bool enableApplicabilityCache(true);
#else
    constexpr bool enableApplicabilityCache(false);
#endif

    static_assert(enableApplicabilityFiltering or not enableApplicabilityCache);

#ifdef ENABLE_TERMINAL_STATE_SUPPORT
    constexpr bool enableTerminalStateSupport(true);
#else
    constexpr bool enableTerminalStateSupport(false);
#endif

#ifdef ENABLE_TERMINAL_STATE_SUPPORT

#define FIELD_IF_TERMINAL_STATE_SUPPORT(FCT) FCT
#define FCT_IF_TERMINAL_STATE_SUPPORT(FCT) FCT
#define STMT_IF_TERMINAL_STATE_SUPPORT(STMT) STMT;
#define CONSTRUCT_IF_TERMINAL_STATE_SUPPORT(CONSTRUCT) ,CONSTRUCT

#else

#define FIELD_IF_TERMINAL_STATE_SUPPORT(FCT)
#define FCT_IF_TERMINAL_STATE_SUPPORT(FCT)
#define STMT_IF_TERMINAL_STATE_SUPPORT(STMT)
#define CONSTRUCT_IF_TERMINAL_STATE_SUPPORT(CONSTRUCT)

#endif

    /**
     * We do not care whether reach states are terminal.
     * On the generic level this is intuitive.
     * When encoding a reach-avoid property (i.e., reach is "terminal" and avoid is "reach"),
     * this still is correct if reach & avoid are mutually exclusive (for reachable states).
     */

#ifdef REACH_MAY_BE_TERMINAL
    constexpr bool reachMayBeTerminal(true);
#else
    constexpr bool reachMayBeTerminal(false);
#endif

}

#endif //PLAJA_CT_CONFIG_CONST_H
