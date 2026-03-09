//
// This file is part of the PlaJA code base.
// Copyright (c) (2019 - 2020) Marcel Vinzent.
// See README.md in the top-level directory for licensing information.
//

/***
 * Defines primitive types used throughout the implementation.
 * Gives an overview + a convention.
 */

#ifndef PLAJA_USING_SEARCH_H
#define PLAJA_USING_SEARCH_H

#include <limits>
#include "../parser/using_parser.h"

/* Action. */

using ActionLabel_type = ActionID_type; // Label.

/* Operator. */
using ActionOpID_type = unsigned long int; // Unique id over *all* operators.
using ActionOpIndex_type = unsigned int; // Unique index *per label*.
using LocalOpIndex_type = int; // Local (source state specific) index of an operator; e.g., in the context of state space structures; negative to represent non-operator.

/* Transition/Update. */
using UpdateIndex_type = unsigned int; // Index of the update structure in the operator.
using UpdateOpID_type = unsigned long int; // id of action op id + update index.
using TransitionIndex_type = unsigned int; // (Source state specific) index of the transition (induced) by an update structure.

namespace ACTION {

    constexpr ActionLabel_type noneLabel = nullAction;
    constexpr ActionLabel_type silentLabel = silentAction;

    constexpr ActionOpID_type noneOp = static_cast<ActionOpID_type>(-1);
    constexpr LocalOpIndex_type noneLocalOp = static_cast<LocalOpIndex_type>(-1);

    constexpr UpdateIndex_type noneUpdate = static_cast<UpdateIndex_type>(-1);
    constexpr UpdateOpID_type noneUpdateOp = static_cast<UpdateOpID_type>(-1);
    constexpr TransitionIndex_type noneTransition = static_cast<TransitionIndex_type>(-1);

}

/* State. */

using StateID_type = unsigned long int;

using Range_type = unsigned int;

namespace STATE { constexpr StateID_type none = static_cast<StateID_type>(-1); }

/* Search values. */

using SearchDepth_type = int;

using StepIndex_type = unsigned int; // Like in transition steps, e.g., for SMT.

namespace SEARCH {

    constexpr SearchDepth_type noneDepth = -1;

    constexpr StepIndex_type noneStep = static_cast<StepIndex_type>(-1);

}

using HeuristicValue_type = int;

namespace HEURISTIC {

    constexpr HeuristicValue_type unknown = std::numeric_limits<HeuristicValue_type>::lowest();

    constexpr HeuristicValue_type deadEnd = std::numeric_limits<HeuristicValue_type>::max();

}

using QValue_type = float; // Crucial for DEAD-END handling, cf. ValueInitializer, OptimizationCriterion.

#endif //PLAJA_USING_SEARCH_H
