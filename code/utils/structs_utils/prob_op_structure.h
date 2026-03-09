//
// This file is part of the PlaJA code base.
// Copyright (c) (2019 - 2021) Marcel Vinzent.
// See README.md in the top-level directory for licensing information.
//

#ifndef PLAJA_PROB_OP_STRUCTURE_H
#define PLAJA_PROB_OP_STRUCTURE_H

#include "../../search/using_search.h"

struct ProbOpStructure {
    ActionOpID_type actionOpID;
    PLAJA::Prob_type prob;

    ProbOpStructure(ActionOpID_type action_op_id, PLAJA::Prob_type prob_): actionOpID(action_op_id), prob(prob_) {}
    ~ProbOpStructure() = default;
};

#endif //PLAJA_PROB_OP_STRUCTURE_H
