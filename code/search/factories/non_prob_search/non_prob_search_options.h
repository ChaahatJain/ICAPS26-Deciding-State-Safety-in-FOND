//
// This file is part of the PlaJA code base.
// Copyright (c) (2019 - 2022) Marcel Vinzent.
// See README.md in the top-level directory for licensing information.
//

#ifndef PLAJA_NON_PROB_SEARCH_OPTIONS_H
#define PLAJA_NON_PROB_SEARCH_OPTIONS_H

#include <string>

namespace PLAJA_OPTION {

    extern const std::string computeGoalPaths;
    extern const std::string suppressStateSpace;

}

namespace PLAJA_OPTION_DEFAULT {

    constexpr bool computeGoalPaths(false);
    constexpr bool suppressStateSpace(false);

}

#endif //PLAJA_NON_PROB_SEARCH_OPTIONS_H
