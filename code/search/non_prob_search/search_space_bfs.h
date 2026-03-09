//
// This file is part of the PlaJA code base (2019 - 2021).
//

#ifndef PLAJA_SEARCH_SPACE_BFS_H
#define PLAJA_SEARCH_SPACE_BFS_H

#include "../../utils/structs_utils/path.h"
#include "base/search_node_info_base.h"
#include "base/search_space_base.h"

class Model;

using BFSPath = Path<StateID_type>;

class SearchSpaceBFS final: public SearchSpace_<SearchNodeInfoBase> {
public:
    explicit SearchSpaceBFS(const ModelInformation& model_info);
    ~SearchSpaceBFS() override;
    DELETE_CONSTRUCTOR(SearchSpaceBFS)

    [[nodiscard]] BFSPath trace_path(StateID_type goal_state) const;
    [[nodiscard]] BFSPath dump_path(StateID_type goal_state, const Model* model) const;
};

#endif //PLAJA_SEARCH_SPACE_BFS_H
