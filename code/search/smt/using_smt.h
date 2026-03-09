//
// This file is part of the PlaJA code base.
// Copyright (c) (2019 - 2022) Marcel Vinzent.
// See README.md in the top-level directory for licensing information.
//

#ifndef PLAJA_USING_SMT_H
#define PLAJA_USING_SMT_H

#include "utils/forward_z3.h"

namespace Z3_IN_PLAJA {

    using VarId_type = unsigned int;
    constexpr VarId_type noneVar = static_cast<VarId_type>(-1);

    class Context;

    class StateVarsInZ3;

    class StateIndexes;

    class SMTSolver;

    class MarabouToZ3Vars;

    constexpr unsigned int floatingPrecision = 9;

    constexpr double samplePrecision(1.0e-1);

    constexpr double enumerationPrecision = 1.0e-3;

    constexpr unsigned sampleTrials(10);

}

#endif //PLAJA_USING_SMT_H
