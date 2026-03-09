//
// This file is part of the PlaJA code base.
// Copyright (c) (2019 - 2020) Marcel Vinzent.
// See README.md in the top-level directory for licensing information.
//

/***
 * Defines primitive types used throughout the implementation.
 * Gives an overview + a convention.
 */

#ifndef PLAJA_USING_PARSER_H
#define PLAJA_USING_PARSER_H

#include "../using_plaja.h"

// ast IDs & indexes

//      (parser) internally set

using ActionID_type = int; // a unique ID representing the action in the model, negative ID for silent & null action (see below), otherwise >= 0 (+ tight indexing)

using AutomatonIndex_type = int; // the index of an automaton instance in the model, negative values to represent non-automaton index (see below), otherwise >= 0 (+ tight indexing)

using SequenceIndex_type = unsigned int; // just the sequence index on an assignment

using SyncIndex_type = unsigned int; // just the index of the synchronisation directive in the model

using VariableID_type = unsigned int; // a unique ID representing the variable instance in the model

using FreeVariableID_type = unsigned int;  // a unique ID representing the free variable instance in the model

using ConstantIdType = unsigned int; // a unique ID representing the constant in the model

//      externally set

using VariableIndex_type = unsigned int; // the state index of a variable (or automaton location)

using EdgeID_type = unsigned int; // ID of an edge within the respective automaton

using SyncID_type = unsigned int; // ID of a sync. element within the model

// special values

namespace VARIABLE {
    constexpr VariableID_type none = static_cast<VariableID_type>(-1);
}

namespace STATE_INDEX {
    constexpr VariableIndex_type none = static_cast<VariableIndex_type>(-1);
}

namespace AUTOMATON {
    constexpr AutomatonIndex_type invalid = -1;
    constexpr PLAJA::integer invalidLocation = -1;
}

namespace ACTION {
    constexpr ActionID_type silentAction = -1;
    constexpr ActionID_type nullAction = -2;
}

namespace EDGE {
    constexpr EdgeID_type nullEdge = 0;
}

namespace SYNCHRONISATION {
    constexpr SyncID_type nonSyncID = 0;
}

#endif //PLAJA_USING_PARSER_H
