//
// This file is part of the PlaJA code base.
// Copyright (c) (2019 - 2023) Marcel Vinzent.
// See README.md in the top-level directory for licensing information.
//

#ifndef PLAJA_USING_PLAJA_H
#define PLAJA_USING_PLAJA_H

namespace PLAJA {

    using integer = int;
    using floating = double;

    constexpr PLAJA::floating integerPrecision = 1.0e-8; // This is one magnitude larger than Gurobi minimal distance.
    constexpr PLAJA::floating floatingPrecision = 1.0e-8;

    using Prob_type = PLAJA::floating;
    constexpr Prob_type probabilityPrecision = floatingPrecision;
    constexpr Prob_type noneProb = -1;

    using Cost_type = float;

}

#endif //PLAJA_USING_PLAJA_H
