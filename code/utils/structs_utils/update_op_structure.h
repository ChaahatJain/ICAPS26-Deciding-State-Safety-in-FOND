//
// This file is part of the PlaJA code base.
// Copyright (c) (2019 - 2020) Marcel Vinzent.
// See README.md in the top-level directory for licensing information.
//

#ifndef PLAJA_UPDATE_OP_STRUCTURE_H
#define PLAJA_UPDATE_OP_STRUCTURE_H

#include "../../search/using_search.h"

struct UpdateOpStructure {
    ActionOpID_type actionOpID;
    UpdateIndex_type updateIndex;

    UpdateOpStructure(ActionOpID_type action_op_id, UpdateIndex_type update_index): actionOpID(action_op_id), updateIndex(update_index) {}
    ~UpdateOpStructure() = default;
};

#endif //PLAJA_UPDATE_OP_STRUCTURE_H
