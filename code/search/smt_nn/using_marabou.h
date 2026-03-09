//
// This file is part of the PlaJA code base.
// Copyright (c) (2019 - 2022) Marcel Vinzent.
// See README.md in the top-level directory for licensing information.
//

#ifndef PLAJA_USING_MARABOU_H
#define PLAJA_USING_MARABOU_H

#include "../../include/marabou_include/float_utils.h"
#include "../../parser/nn_parser/using_nn.h"
#include "../../assertions.h"
#include "../../using_plaja.h"

using MarabouVarIndex_type = unsigned int;
using MarabouInteger_type = int; // note Marabou itself uses double only
using MarabouFloating_type = double;
using BinaryVarIndex = unsigned int; // for MIP encodings

namespace MARABOU_IN_PLAJA {

    constexpr MarabouVarIndex_type noneVar = static_cast<MarabouVarIndex_type>(-1);
    constexpr double integerPrecision = PLAJA::integerPrecision;
    constexpr double floatingPrecision = PLAJA::floatingPrecision;
    constexpr double argMaxPrecision = PLAJA_NN::argMaxPrecision - (PLAJA::floatingPrecision * 0.1); // Account for default epsilon for comparison inside Marabou.
    // Must be smaller than predicateSplitPrecision (say at least one magnitude) & greater than Marabou's preprocessor bound tolerance.
    // Should be greater equal Marabou's "almost fixed"-tolerance.
    constexpr MarabouFloating_type strictnessPrecision = 1.0e-5;

}

#ifndef NDEBUG
#define CHECK_MARABOU_INF_BOUNDS(LB, UB) PLAJA_ASSERT(FloatUtils::gte(LB, FloatUtils::negativeInfinity())) PLAJA_ASSERT(FloatUtils::lte(UB, FloatUtils::infinity()))
#else
#define CHECK_MARABOU_INF_BOUNDS(LB, UB)
#endif

#endif //PLAJA_USING_MARABOU_H
