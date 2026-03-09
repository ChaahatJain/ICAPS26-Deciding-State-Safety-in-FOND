//
// This file is part of the PlaJA code base (2019 - 2021).
//

#include "search_space_bfs.h"
#include "../fd_adaptions/state.h"

SearchSpaceBFS::SearchSpaceBFS(const ModelInformation& model_info):
    SearchSpace_<SearchNodeInfoBase>(model_info) {
}

SearchSpaceBFS::~SearchSpaceBFS() = default;

/**********************************************************************************************************************/

BFSPath SearchSpaceBFS::trace_path(StateID_type goal_state) const {
    BFSPath path;
    PLAJA_ASSERT(path.empty())
    auto current_state = get_state_ptr(goal_state);
    for (;;) {
        path.push_front(current_state->get_id_value());
        const auto& node_info = get_node_info(current_state->get_id_value());
        if (not node_info.has_parent()) { break; }
        current_state = get_state_ptr(node_info.get_parent());
    }
    return path;
}

BFSPath SearchSpaceBFS::dump_path(StateID_type goal_state, const Model* model) const {
    BFSPath path = trace_path(goal_state);
    for (const StateID_type state_id: path) { get_state(state_id).dump(model); }
    return path;
}
