//
// This file is part of the PlaJA code base.
// Copyright (c) (2019 - 2021) Marcel Vinzent.
// See README.md in the top-level directory for licensing information.
//

#ifndef PLAJA_SEARCH_SPACE_BASE_H
#define PLAJA_SEARCH_SPACE_BASE_H

#include <list>
#include <memory>
#include "unordered_set"
#include "../../../utils/fd_imports/segmented_vector.h"
#include "../../../utils/structs_utils/forward_struct_utils.h"
#include "../../../utils/structs_utils/path.h"
#include "../../../utils/utils.h"
#include "../../information/forward_information.h"
#include "../../states/forward_states.h"
#include "../../using_search.h"
#include "search_node_info_base.h"

using StatePath = Path<StateID_type>;

class SearchSpaceBase {

private:
    const ModelInformation* modelInformation;
    std::unique_ptr<StateRegistry> stateRegistry;

    std::unique_ptr<State> initialState; // Primary initial state.
    std::unordered_set<StateID_type> initialStates;

    std::list<StateID_type> goalPathFrontier;

    unsigned int currentStep;

    [[nodiscard]] unsigned int compute_goal_path_states_recursively(StateID_type state_id, std::list<StateID_type>& stack);

protected:
    explicit SearchSpaceBase(const ModelInformation& model_info);
    explicit SearchSpaceBase(const class Model& model);

public:
    virtual ~SearchSpaceBase() = 0;
    DELETE_CONSTRUCTOR(SearchSpaceBase)

    /** Reset search space for incremental serch iteration. */
    void reset();

    /* Initial/start. */

    StateID_type add_initial_state(const StateValues& initial_values);

    [[nodiscard]] inline const State& get_initial_state() const { return *initialState; }

    [[nodiscard]] inline const std::unordered_set<StateID_type>& _initial_states() const { return initialStates; }

    /* Goal. */

    inline void add_goal_path_frontier(StateID_type state_id) { goalPathFrontier.push_back(state_id); }

    [[nodiscard]] inline bool goal_path_frontier_empty() const { return goalPathFrontier.empty(); }

    [[nodiscard]] inline const std::list<StateID_type>& get_goal_path_frontier() const {
        PLAJA_ASSERT(not goal_path_frontier_empty())
        return goalPathFrontier;
    }

    [[nodiscard]] inline StateID_type goal_frontier_state() const {
        PLAJA_ASSERT(not goal_path_frontier_empty())
        return *goalPathFrontier.cbegin();
    }

    /** Compute goal paths -- after search is completed. */
    unsigned int compute_goal_path_states();

    /* States. */

    [[nodiscard]] State get_state(StateID_type id) const;

    [[nodiscard]] std::unique_ptr<State> get_state_ptr(StateID_type id) const;

    [[nodiscard]] std::unique_ptr<State> add_state_ptr(const StateValues& state) const;

    [[nodiscard]] StateID_type add_state(const StateValues& state) const;

    /* Nodes. */

    /**
     * Add node to search space if necessary.
     * Can also be used as normal get_node.
     */
    [[nodiscard]] virtual SearchNodeInfoBase& add_node_info(StateID_type state_id) = 0;

    [[nodiscard]] virtual SearchNodeInfoBase& get_node_info(StateID_type state_id) = 0;

    [[nodiscard]] virtual const SearchNodeInfoBase& get_node_info(StateID_type state_id) const = 0;

    [[nodiscard]] inline SearchNode add_node(StateID_type state_id) { return { add_node_info(state_id), currentStep }; }

    [[nodiscard]] SearchNode get_node(StateID_type state_id) { return { get_node_info(state_id), currentStep }; }

    /* Aux. */

    [[nodiscard]] UpdateOpID_type compute_update_op(ActionOpID_type op, UpdateIndex_type update) const;

    [[nodiscard]] UpdateOpStructure compute_update_op(UpdateOpID_type update_op) const;

    /* Output. */
    [[nodiscard]] StatePath trace_path(StateID_type goal_state) const;
    [[nodiscard]] StatePath dump_path(StateID_type goal_state, const class Model* model) const;

    /* Stats. */
    void print_statistics() const;
    void stats_to_csv(std::ofstream& file) const;
    static void stat_names_to_csv(std::ofstream& file);

};

/**********************************************************************************************************************/

template<typename SearchNodeInfo_t>
class SearchSpace_: public SearchSpaceBase {
private:
    segmented_vector::SegmentedVector<SearchNodeInfo_t> searchInformation;  // Search information per state.

public:
    explicit SearchSpace_(const ModelInformation& model_info):
        SearchSpaceBase(model_info) {
    }

    explicit SearchSpace_(const class Model& model):
        SearchSpaceBase(model) {
    }

    ~SearchSpace_() override = default;

    DELETE_CONSTRUCTOR(SearchSpace_)

    [[nodiscard]] inline std::size_t information_size() const { return searchInformation.size(); } // mainly for debugging purposes

    // Get NODES:

    SearchNodeInfoBase& add_node_info(const StateID_type state_id) final {
        if (state_id >= searchInformation.size()) { searchInformation.resize(state_id + 1); }
        return searchInformation[state_id];
    }

    [[nodiscard]] SearchNodeInfoBase& get_node_info(StateID_type state_id) final {
        PLAJA_ASSERT(state_id < searchInformation.size())
        return searchInformation[state_id];
    }

    [[nodiscard]] const SearchNodeInfoBase& get_node_info(StateID_type state_id) const final {
        PLAJA_ASSERT(state_id < searchInformation.size())
        return searchInformation[state_id];
    }

    inline SearchNodeInfo_t& add_node_info_cast(const StateID_type state_id) {
        return PLAJA_UTILS::cast_ref<SearchNodeInfo_t>(add_node_info(state_id));
    }

    inline SearchNodeInfo_t& get_node_info_cast(StateID_type state_id) {
        return PLAJA_UTILS::cast_ref<SearchNodeInfo_t>(get_node_info(state_id));
    }

    inline const SearchNodeInfo_t& get_node_info_cast(StateID_type state_id) const {
        return PLAJA_UTILS::cast_ref<SearchNodeInfo_t>(get_node_info(state_id));
    }

};

#endif //PLAJA_SEARCH_SPACE_BASE_H
