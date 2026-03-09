//
// This file is part of the PlaJA code base.
// Copyright (c) (2019 - 2020) Marcel Vinzent.
// See README.md in the top-level directory for licensing information.
//

#ifndef PLAJA_ACTION_OP_STRUCTURE_H
#define PLAJA_ACTION_OP_STRUCTURE_H

#include <list>
#include "../../parser/using_parser.h"

struct ActionOpStructure {
    SyncID_type syncID;
    std::list<std::pair<AutomatonIndex_type, EdgeID_type>> participatingEdges;
};

#endif //PLAJA_ACTION_OP_STRUCTURE_H
