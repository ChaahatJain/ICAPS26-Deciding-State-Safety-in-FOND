//
// This file is part of the PlaJA code base.
// Copyright (c) (2019 - 2022) Marcel Vinzent.
// See README.md in the top-level directory for licensing information.
//

#ifndef PLAJA_BMC_OPTIONS_H
#define PLAJA_BMC_OPTIONS_H

#include <string>

namespace PLAJA_OPTION {

    extern const std::string bmc_mode;
    extern const std::string max_steps;
    extern const std::string find_shortest_path_only;
    extern const std::string constrain_no_start;
    extern const std::string constrain_loop_free;
    extern const std::string constrain_no_reach;
    extern const std::string bmc_solver;

}

namespace PLAJA_OPTION_DEFAULT {

    constexpr int max_steps(50);
    constexpr bool find_shortest_path_only(true);

}

#endif //PLAJA_BMC_OPTIONS_H
