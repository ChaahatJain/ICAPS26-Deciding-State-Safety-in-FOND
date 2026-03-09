//
// This file is part of the PlaJA code base.
// Copyright (c) (2023 - 2024) Chaahat Jain.
//

#ifndef PLAJA_USING_VERITAS_H
#define PLAJA_USING_VERITAS_H

#include "../../assertions.h"
#include "../../using_plaja.h"
#include "predicates/Equation.h"
#include "predicates/Tightening.h"
#include "addtree.hpp"

using VeritasVarIndex_type = veritas::FeatId;
using VeritasInteger_type = int;
using VeritasFloating_type = double;
using BinaryVarIndex = unsigned int; // for MIP encodings

namespace VERITAS_IN_PLAJA {

    constexpr VeritasVarIndex_type noneVar = static_cast<VeritasVarIndex_type>(-1);
    constexpr double integerPrecision = PLAJA::integerPrecision;
    constexpr double floatingPrecision = PLAJA::floatingPrecision;
    constexpr VeritasFloating_type strictnessPrecision = 1.0e-5;
    constexpr veritas::FloatT infinity = 100000;
    constexpr veritas::FloatT negative_infinity = -200000;
    constexpr veritas::FloatT offset = 0;

}

#ifndef NDEBUG
#define CHECK_VERITAS_INF_BOUNDS(LB, UB) PLAJA_ASSERT(LB >= VERITAS_IN_PLAJA::negative_infinity) PLAJA_ASSERT(UB <= VERITAS_IN_PLAJA::infinity)
#else
#define CHECK_VERITAS_INF_BOUNDS(LB, UB)
#endif

#endif //PLAJA_USING_VERITAS_H
