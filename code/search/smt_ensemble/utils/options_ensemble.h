//
// This file is part of the PlaJA code base (2019 - 2022).
//

#ifndef PLAJA_OPTIONS_VERITAS_H
#define PLAJA_OPTIONS_VERITAS_H

#include <string>
#include <unordered_map>

namespace OPTIONS_VERITAS {

    enum SolverType {
        RELUPLEX,
        SOI,
        MILP,
    };

    extern const std::unordered_map<std::string, SolverType> stringToSolverType; // extern usage

    [[nodiscard]] extern int get_dnc_mode();
    [[nodiscard]] extern int get_timeout();

    extern void parse_veritas_options();

}

#endif //PLAJA_OPTIONS_VERITAS_H
