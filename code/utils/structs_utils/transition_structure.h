//
// This file is part of the PlaJA code base.
// Copyright (c) (2019 - 2021) Marcel Vinzent.
// See README.md in the top-level directory for licensing information.
//

#ifndef PLAJA_TRANSITION_STRUCTURE_H
#define PLAJA_TRANSITION_STRUCTURE_H

#include "../../search/using_search.h"

struct TransitionStructure {
    ActionOpID_type actionOpID;
    TransitionIndex_type transitionIndex;

    TransitionStructure(ActionOpID_type action_op_id, TransitionIndex_type transition_index): actionOpID(action_op_id), transitionIndex(transition_index) {}
    ~TransitionStructure() = default;
};

#endif //PLAJA_TRANSITION_STRUCTURE_H
