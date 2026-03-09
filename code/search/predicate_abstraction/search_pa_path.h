//
// This file is part of the PlaJA code base.
// Copyright (c) (2019 - 2021) Marcel Vinzent.
// See README.md in the top-level directory for licensing information.
//

#ifndef PLAJA_SEARCH_PA_PATH_H
#define PLAJA_SEARCH_PA_PATH_H

#include "base/search_pa_base.h"

class SearchPAPath: public SearchPABase {

public:
    explicit SearchPAPath(const SearchEngineConfigPA& config, bool witness_interval = false);
    ~SearchPAPath() override;

    HeuristicValue_type search_path(StateID_type src_id);
    HeuristicValue_type get_goal_distance(const AbstractState& pa_state) override;

};

#endif //PLAJA_SEARCH_PA_PATH_H
