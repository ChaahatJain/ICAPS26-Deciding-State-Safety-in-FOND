//
// This file is part of the PlaJA code base.
// Copyright (c) (2019 - 2020) Marcel Vinzent.
// See README.md in the top-level directory for licensing information.
//

#ifndef PLAJA_USING_PREDICATE_ABSTRACTION_H
#define PLAJA_USING_PREDICATE_ABSTRACTION_H

#include "../../include/ct_config_const.h"
#include "../../assertions.h"

#include "../using_search.h"
#include "../smt/using_smt.h"

class StateValues;

using PredicatesNumber_type = unsigned int;
using PredicateIndex_type = unsigned int;

namespace PREDICATE {

    constexpr PredicateIndex_type none = static_cast<PredicateIndex_type>(-1);

    constexpr PLAJA::floating predicateSplitPrecision = 1.0e-3;

}

namespace PA {

#ifdef LAZY_PA

    using TruthValueType = PLAJA::integer;
    using EntailmentValueType = TruthValueType;

    constexpr TruthValueType False = 0;
    constexpr TruthValueType True = 1;

    inline constexpr TruthValueType unknown_value() { return 2; }

    inline constexpr bool is_unknown(TruthValueType value) { return value == unknown_value(); }

#else

    using TruthValueType = bool;
    using EntailmentValueType = bool;

    constexpr TruthValueType False = false;
    constexpr TruthValueType True = true;

    inline TruthValueType unknown_value() {
        PLAJA_ABORT
        return false;
    }

    inline constexpr bool is_unknown(TruthValueType /*value*/) { return false; } // "return false" is a hard assumption throughout the code base

#endif

}

#endif //PLAJA_USING_PREDICATE_ABSTRACTION_H
