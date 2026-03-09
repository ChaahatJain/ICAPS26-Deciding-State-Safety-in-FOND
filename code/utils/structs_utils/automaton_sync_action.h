//
// This file is part of the PlaJA code base.
// Copyright (c) (2019 - 2020) Marcel Vinzent.
// See README.md in the top-level directory for licensing information.
//

#ifndef PLAJA_LOCAL_SYNC_ACTION_H
#define PLAJA_LOCAL_SYNC_ACTION_H

#include "../../parser/using_parser.h"

struct AutomatonSyncAction {
    AutomatonIndex_type automatonIndex;
    ActionID_type actionID;
    AutomatonSyncAction(AutomatonIndex_type automaton_index, ActionID_type action_id): automatonIndex(automaton_index), actionID(action_id) {};
};

#endif //PLAJA_LOCAL_SYNC_ACTION_H
