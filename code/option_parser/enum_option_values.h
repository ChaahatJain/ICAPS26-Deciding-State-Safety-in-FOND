//
// This file is part of the PlaJA code base.
// Copyright (c) (2019 - 2024) Marcel Vinzent.
// See README.md in the top-level directory for licensing information.
//

#ifndef PLAJA_ENUM_OPTION_VALUES_H
#define PLAJA_ENUM_OPTION_VALUES_H

#include <memory>
#include <string>
#include "../include/ct_config_const.h"

namespace PLAJA_OPTION {

    /**
     * Global enum for special option values.
     */
    enum class EnumOptionValue { // NOLINT(*-enum-size)

        None,

#ifdef USE_Z3
        Z3,
        Naive,
        Inc,
        Bs,
        Td,
        Sample,
        Biased,
        WP,
        AvoidCount,
#endif

#ifdef USE_MARABOU
        Marabou,
        /* Marabou solver configs. */
        Relaxed,
        BranchAndBound,
        Enumerate,
#endif

#ifdef BUILD_BMC
        BmcUnsafe,
        BmcLoopFree,
        BmcUnsafeLoopFree,
        BmcLoopFreeUnsafe,
#endif

#ifdef BUILD_PA
        PaReachabilityGlobal, // Only check state-reachability globally (default).
        PaOnlyReachabilityPerSrc, // Only check for state-reachability per source state, skipping redundant transition tests.
        PaOnlyReachabilityPerLabel, // Only check for state-reachability per label.
        PaOnlyReachabilityPerOp, // Only check for state-reachability per op.
        /* None -> check per update. */
#endif

#ifdef BUILD_TO_NUXMV
        /* NuXmv default configs. */
        Bmc,
        SBmc,
        Coi,
        IpaBmc,
        IpaCegar,
        Cegar,
        Ic3,
#endif

    };

    extern std::unique_ptr<EnumOptionValue> str_to_enum_option_value(const std::string& str);

    extern const std::string& enum_option_value_to_str(EnumOptionValue value);

    class EnumOptionValuesSet;

}

#endif //PLAJA_ENUM_OPTION_VALUES_H
