//
// This file is part of the PlaJA code base.
// Copyright (c) (2019 - 2022) Marcel Vinzent.
// See README.md in the top-level directory for licensing information.
//

#ifndef PLAJA_OPTIONS_MARABOU_H
#define PLAJA_OPTIONS_MARABOU_H

#include <string>
#include <unordered_map>

namespace OPTIONS_MARABOU {

    enum SolverType {
        RELUPLEX,
        SOI,
        MILP,
    };

    extern const std::unordered_map<std::string, SolverType> stringToSolverType; // extern usage

    [[nodiscard]] extern int get_dnc_mode();
    [[nodiscard]] extern int get_timeout();

    extern void parse_marabou_options();

}

#endif //PLAJA_OPTIONS_MARABOU_H
