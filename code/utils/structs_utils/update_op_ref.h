//
// This file is part of the PlaJA code base.
// Copyright (c) (2019 - 2021) Marcel Vinzent.
// See README.md in the top-level directory for licensing information.
//

#ifndef PLAJA_UPDATE_OP_REF_H
#define PLAJA_UPDATE_OP_REF_H

#include "../../search/using_search.h"

// forward declaration
class ActionOp;

struct UpdateOpRef {
    const ActionOp* actionOp;
    UpdateIndex_type updateIndex;

    UpdateOpRef(const ActionOp* action_op, UpdateIndex_type update_index): actionOp(action_op), updateIndex(update_index) {}
    ~UpdateOpRef() = default;
};


#endif //PLAJA_UPDATE_OP_REF_H
