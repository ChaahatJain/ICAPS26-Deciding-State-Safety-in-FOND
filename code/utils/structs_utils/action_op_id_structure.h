//
// This file is part of the PlaJA code base.
// Copyright (c) (2019 - 2021) Marcel Vinzent.
// See README.md in the top-level directory for licensing information.
//

#ifndef PLAJA_ACTION_OP_ID_STRUCTURE_H
#define PLAJA_ACTION_OP_ID_STRUCTURE_H

#include "../../search/using_search.h"

struct ActionOpIDStructure {
    SyncID_type syncID;
    ActionOpIndex_type actionOpIndex;
    ActionOpIDStructure(SyncID_type sync_id, ActionOpIndex_type action_op_index): syncID(sync_id), actionOpIndex(action_op_index) {}
    ~ActionOpIDStructure() = default;
};

#endif //PLAJA_ACTION_OP_ID_STRUCTURE_H
