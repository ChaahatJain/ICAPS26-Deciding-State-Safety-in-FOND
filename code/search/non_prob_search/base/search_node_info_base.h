//
// This file is part of the PlaJA code base.
// Copyright (c) (2019 - 2021) Marcel Vinzent.
// See README.md in the top-level directory for licensing information.
//

#ifndef PLAJA_SEARCH_NODE_INFO_BASE_H
#define PLAJA_SEARCH_NODE_INFO_BASE_H

#include <algorithm>
#include "../../../utils/utils.h"
#include "../../using_search.h"

struct SearchNodeInfoBase {

    friend struct SearchNode;

private:
    std::vector<std::pair<StateID_type, UpdateOpID_type>> parents;
    UpdateOpID_type goalPathOp;
    StateID_type goalPathSuccessor;
    SearchDepth_type startDistance;
    HeuristicValue_type goalDistance;
    unsigned int currentStep; // NOLINT(*-use-default-member-init)

public:
    SearchNodeInfoBase():
        parents()
        , goalPathOp(ACTION::noneUpdateOp)
        , goalPathSuccessor(STATE::none)
        , startDistance(SEARCH::noneDepth)
        , goalDistance(HEURISTIC::unknown)
        , currentStep(0) {
    }

    virtual ~SearchNodeInfoBase() = default;
    DEFAULT_CONSTRUCTOR(SearchNodeInfoBase)

    /* Getter. */

    [[nodiscard]] inline bool has_parent() const { return not parents.empty(); }

    [[nodiscard]] inline StateID_type get_parent() const {
        PLAJA_ASSERT(has_parent())
        return parents.front().first;
    }

    /**/

    [[nodiscard]] inline SearchDepth_type get_start_distance() const { return startDistance; }

    [[nodiscard]] inline bool is_start() const { return startDistance == 0; }

    /**/

    [[nodiscard]] inline HeuristicValue_type get_goal_distance() const { return goalDistance; }

    [[nodiscard]] inline UpdateOpID_type get_goal_path_op() const { return goalPathOp; }

    [[nodiscard]] inline StateID_type get_goal_path_successor() const { return goalPathSuccessor; }

    [[nodiscard]] inline bool known_goal_distance() const { return goalDistance != HEURISTIC::unknown; }

    [[nodiscard]] inline bool is_goal() const { return goalDistance == 0; }

    [[nodiscard]] inline bool has_goal_path() const {
        PLAJA_ASSERT(goalDistance == HEURISTIC::unknown or goalDistance == HEURISTIC::deadEnd or (goalPathOp != ACTION::noneUpdateOp and goalPathSuccessor != STATE::none) or is_goal())
        return goalDistance != HEURISTIC::unknown and goalDistance != HEURISTIC::deadEnd;
    }

    [[nodiscard]] inline bool is_dead_end() const { return goalDistance == HEURISTIC::deadEnd; }

    /* Setter. */

    inline void set_goal_distance(HeuristicValue_type goal_distance) {
        PLAJA_ASSERT_EXPECT(goalDistance == HEURISTIC::unknown or goal_distance < goalDistance)
        goalDistance = goal_distance;
    }

    inline void add_parent(StateID_type parent_id, UpdateOpID_type parent_update_op) {
        PLAJA_ASSERT(parent_id != STATE::none and parent_update_op != ACTION::noneUpdateOp)
        PLAJA_ASSERT(std::all_of(parents.cbegin(), parents.cend(), [parent_id](const std::pair<StateID_type, UpdateOpID_type> known_parent) { return parent_id != known_parent.first; }))
        parents.emplace_back(parent_id, parent_update_op);
    }

    /**/

    inline void set_goal() {
        PLAJA_ASSERT(not has_goal_path())
        PLAJA_ASSERT(goalDistance == HEURISTIC::unknown)
        goalPathOp = ACTION::noneUpdateOp;
        goalPathSuccessor = STATE::none;
        set_goal_distance(0);
    }

    inline void set_goal_path_successor(UpdateOpID_type goal_path_op, StateID_type goal_path_successor, HeuristicValue_type goal_distance) {
        PLAJA_ASSERT(goal_path_op != ACTION::noneUpdateOp)
        PLAJA_ASSERT(goal_path_successor != STATE::none)
        PLAJA_ASSERT(goal_distance != HEURISTIC::unknown)
        PLAJA_ASSERT(goal_distance != HEURISTIC::deadEnd)
        goalPathOp = goal_path_op;
        goalPathSuccessor = goal_path_successor;
        set_goal_distance(goal_distance);
    }

    inline void set_dead_end() {
        PLAJA_ASSERT(not has_goal_path())
        PLAJA_ASSERT(goalDistance == HEURISTIC::unknown)
        goalPathOp = ACTION::noneUpdateOp;
        goalPathSuccessor = STATE::none;
        goalDistance = HEURISTIC::deadEnd;
    }

    /* Iterator. */

    class ParentsIterator {
        friend SearchNodeInfoBase;

    private:
        std::vector<std::pair<StateID_type, UpdateOpID_type>>::const_iterator it;
        std::vector<std::pair<StateID_type, UpdateOpID_type>>::const_iterator itEnd;

        explicit ParentsIterator(const std::vector<std::pair<StateID_type, UpdateOpID_type>>& parents):
            it(parents.cbegin())
            , itEnd(parents.cend()) {}

    public:
        ~ParentsIterator() = default;

        DELETE_CONSTRUCTOR(ParentsIterator)

        inline void operator++() { ++it; }

        [[nodiscard]] inline bool end() const { return it == itEnd; }

        [[nodiscard]] inline StateID_type parent_state() const { return it->first; }

        [[nodiscard]] inline UpdateOpID_type parent_update_op() const { return it->second; }

    };

    [[nodiscard]] inline SearchNodeInfoBase::ParentsIterator init_parents_iterator() const { return ParentsIterator(parents); }

};

struct SearchNode final {

private:
    SearchNodeInfoBase* info;
    unsigned int currentStep;

public:
    SearchNode(SearchNodeInfoBase& info, unsigned int current_step):
        info(&info)
        , currentStep(current_step) {
    }

    ~SearchNode() = default;
    DEFAULT_CONSTRUCTOR(SearchNode)

    /* Getter. */

    [[nodiscard]] inline const SearchNodeInfoBase& operator()() const { return *info; }

    [[nodiscard]] inline SearchNodeInfoBase& operator()() { return *info; }

    [[nodiscard]] inline bool is_reached() const {
        PLAJA_ASSERT(operator()().currentStep <= currentStep)
        return operator()().currentStep >= currentStep;
    }

    [[nodiscard]] inline bool has_been_reached() const {
        PLAJA_ASSERT(operator()().currentStep <= currentStep)
        return operator()().currentStep >= 1;
    }

    /* Setter. */

    inline void set_reached_init() {
        PLAJA_ASSERT(not is_reached())
        PLAJA_ASSERT(not operator()().has_parent() or has_been_reached())
        operator()().currentStep = currentStep;
        operator()().startDistance = 0;
    }

    inline void set_reached(StateID_type parent_id, UpdateOpID_type parent_update_op, SearchDepth_type start_distance) {
        PLAJA_ASSERT(not is_reached())
        PLAJA_ASSERT(not operator()().has_parent() or has_been_reached())
        operator()().currentStep = currentStep;
        operator()().add_parent(parent_id, parent_update_op);
        operator()().startDistance = start_distance;
    }

#ifndef NDEBUG

    inline void set_unreached() {
        PLAJA_ASSERT_EXPECT(is_reached())
        operator()().currentStep = 0;
        operator()().parents.clear();
        operator()().startDistance = SEARCH::noneDepth;
    }

#endif

    /******************************************************************************************************************/

    /* Directly access information of extensions. */

    template<typename SearchNodeInfoT>
    [[nodiscard]] const SearchNodeInfoT& cast_info() const { return *PLAJA_UTILS::cast_ptr<SearchNodeInfoT>(info); }

    template<typename SearchNodeInfoT>
    [[nodiscard]] SearchNodeInfoT& cast_info() { return *PLAJA_UTILS::cast_ptr<SearchNodeInfoT>(info); }

};

#endif //PLAJA_SEARCH_NODE_INFO_BASE_H
