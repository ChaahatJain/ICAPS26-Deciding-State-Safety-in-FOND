//
// This file is part of the PlaJA code base.
// Copyright (c) (2019 - 2019) Marcel Vinzent.
// See README.md in the top-level directory for licensing information.
//

#ifndef PLAJA_BREADTH_FIRST_SEARCH_H
#define PLAJA_BREADTH_FIRST_SEARCH_H

#include "base/search_engine_non_prob.h"

class BFSearch final: public SearchEngineNonProb {

private:
    std::unique_ptr<SearchSpaceNonProb> searchSpace;

public:
    explicit BFSearch(const PLAJA::Configuration& config);
    ~BFSearch() override;
    DELETE_CONSTRUCTOR(BFSearch)

    SearchStatus finalize() override;

};

#endif //PLAJA_BREADTH_FIRST_SEARCH_H
