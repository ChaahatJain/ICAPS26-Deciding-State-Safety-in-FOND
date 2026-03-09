//
// This file is part of the PlaJA code base.
// Copyright (c) (2019 - 2021) Marcel Vinzent.
// See README.md in the top-level directory for licensing information.
//

#ifndef PLAJA_TRANSITION_INFO_H
#define PLAJA_TRANSITION_INFO_H

#include <utility>
#include "../../search/using_search.h"

using SuccessorProbPair = std::pair<StateID_type, PLAJA::Prob_type>; // this way we can reuse already implemented std::hash for pairs, cf. fret.cpp
using SuccessorUpdatePair = std::pair<StateID_type, UpdateIndex_type>;

#endif //PLAJA_TRANSITION_INFO_H
