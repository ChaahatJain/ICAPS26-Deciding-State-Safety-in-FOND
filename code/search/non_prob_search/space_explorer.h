//
// This file is part of the PlaJA code base.
// Copyright (c) (2019 - 2020) Marcel Vinzent.
// See README.md in the top-level directory for licensing information.
//

#ifndef PLAJA_SPACE_EXPLORER_H
#define PLAJA_SPACE_EXPLORER_H

#include "base/search_engine_non_prob.h"

/** Implementation to construct the state space. */
class SpaceExplorer final: public SearchEngineNonProb {

private:
    std::unique_ptr<SearchSpaceNonProb> searchSpace;
    bool computeGoalPaths;

    SearchStatus finalize() override;

public:
    explicit SpaceExplorer(const PLAJA::Configuration& config);
    ~SpaceExplorer() override;
    DELETE_CONSTRUCTOR(SpaceExplorer)

    /* Override stats. */
    void print_statistics() const override;

};

#endif //PLAJA_SPACE_EXPLORER_H
