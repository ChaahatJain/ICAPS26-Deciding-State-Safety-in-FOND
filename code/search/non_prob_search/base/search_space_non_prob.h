//
// This file is part of the PlaJA code base.
// Copyright (c) (2019 - 2023) Marcel Vinzent.
// See README.md in the top-level directory for licensing information.
//

#ifndef PLAJA_SEARCH_SPACE_NON_PROB_H
#define PLAJA_SEARCH_SPACE_NON_PROB_H

#include "search_space_base.h"

class SearchSpaceNonProb final: public SearchSpace_<SearchNodeInfoBase> {
public:
    explicit SearchSpaceNonProb(const Model& model):
        SearchSpace_<SearchNodeInfoBase>(model) {}

    ~SearchSpaceNonProb() override = default;
    DELETE_CONSTRUCTOR(SearchSpaceNonProb)
};

#endif //PLAJA_SEARCH_SPACE_NON_PROB_H
