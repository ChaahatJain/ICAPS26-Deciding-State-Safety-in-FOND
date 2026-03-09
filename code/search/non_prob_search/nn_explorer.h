//
// This file is part of the PlaJA code base.
// Copyright (c) (2019 - 2021) Marcel Vinzent.
// See README.md in the top-level directory for licensing information.
//

#ifndef PLAJA_NN_EXPLORER_H
#define PLAJA_NN_EXPLORER_H

#include "base/search_engine_non_prob.h"

namespace NN_EXPLORER { class SearchSpace; }

class NN_Explorer final: public SearchEngineNonProb {

private:
    std::unique_ptr<NN_EXPLORER::SearchSpace> searchSpace;

    void add_parent_under_policy(SearchNode& node, StateID_type parent) const override;
    SearchStatus finalize() override;
    void compute_inevitable_goal_path_states();
    void compute_policy_goal_path_states();

public:
    explicit NN_Explorer(const PLAJA::Configuration& config);
    ~NN_Explorer() override;
    DELETE_CONSTRUCTOR(NN_Explorer)

};

#endif //PLAJA_NN_EXPLORER_H
