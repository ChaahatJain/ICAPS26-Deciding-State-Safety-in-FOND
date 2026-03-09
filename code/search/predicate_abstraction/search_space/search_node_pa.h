//
// This file is part of the PlaJA code base.
// Copyright (c) (2019 - 2022) Marcel Vinzent.
// See README.md in the top-level directory for licensing information.
//

#ifndef PLAJA_SEARCH_NODE_PA_H
#define PLAJA_SEARCH_NODE_PA_H

#include <algorithm>
#include "../../../utils/utils.h"
#include "../../using_search.h"
#include "../pa_states/abstract_state.h"

namespace SEARCH_SPACE_PA {

    struct SearchNodeInfo final {
        friend struct SearchNode;

    private:
        std::vector<std::pair<StateID_type, UpdateOpID_type>> parents;
        StateID_type goalPathSuccessor;
        SearchDepth_type startDistance;
        HeuristicValue_type goalDistance;
        unsigned int currentStep; // NOLINT(*-use-default-member-init)

        inline void set_current_step(unsigned int current_step) { currentStep = current_step; }

        [[nodiscard]] inline unsigned int _current_step() const { return currentStep; }

    public:
        SearchNodeInfo():
            parents()
            , goalPathSuccessor(STATE::none)
            , startDistance(static_cast<SearchDepth_type>(-1))
            , goalDistance(HEURISTIC::unknown)
            , currentStep(0) {}

        ~SearchNodeInfo() = default;

        DEFAULT_CONSTRUCTOR_ONLY(SearchNodeInfo)
        DELETE_ASSIGNMENT(SearchNodeInfo)

        /* Getter. */

        // [[nodiscard]] inline StateID_type has_parent() const { PLAJA_ASSERT((parentID != STATE::none and parentUpdateOp != ACTION::noneUpdateOp) or (parentID == STATE::none and parentUpdateOp == ACTION::noneUpdateOp)) return parentID != STATE::none; }
        [[nodiscard]] inline bool has_parent() const { return not parents.empty(); }

        [[nodiscard]] inline StateID_type get_parent() const {
            PLAJA_ASSERT(has_parent())
            return parents.front().first;
        }

        [[nodiscard]] inline UpdateOpID_type get_parent_update_op() const {
            PLAJA_ASSERT(not parents.empty())
            return parents.front().second;
        }

        /**/

        [[nodiscard]] inline SearchDepth_type get_start_distance() const { return startDistance; }

        [[nodiscard]] inline bool is_start() const { return startDistance == 0; }

        /**/

        [[nodiscard]] inline HeuristicValue_type get_goal_distance() const { return goalDistance; }

        // [[nodiscard]] inline UpdateOpID_type get_goal_path_op() const { return goalPathOp; }

        [[nodiscard]] inline StateID_type get_goal_path_successor() const { return goalPathSuccessor; }

        [[nodiscard]] inline bool known_goal_distance() const { return goalDistance != HEURISTIC::unknown; }

        [[nodiscard]] inline bool is_goal() const { return goalDistance == 0; }

        [[nodiscard]] inline bool has_goal_path() const {
            PLAJA_ASSERT(goalDistance == HEURISTIC::unknown or goalDistance == HEURISTIC::deadEnd or (/*goalPathOp != ACTION::noneUpdateOp and*/ goalPathSuccessor != STATE::none) or is_goal())
            return goalDistance != HEURISTIC::unknown and goalDistance != HEURISTIC::deadEnd;
        }

        [[nodiscard]] inline bool is_dead_end() const { return goalDistance == HEURISTIC::deadEnd; }

        /* Setter. */

        inline void set_goal_distance(HeuristicValue_type goal_distance) {
            PLAJA_ASSERT_EXPECT(goalDistance == HEURISTIC::unknown or goal_distance < goalDistance)
            goalDistance = goal_distance;
        }

        // inline void set_parent(StateID_type parent_id, UpdateOpID_type parent_update_op) { parentID = parent_id; parentUpdateOp = parent_update_op; }
        inline void add_parent(StateID_type parent_id, UpdateOpID_type parent_update_op) {
            PLAJA_ASSERT(parent_id != STATE::none and parent_update_op != ACTION::noneUpdateOp)
            PLAJA_ASSERT(std::all_of(parents.cbegin(), parents.cend(), [parent_id](const std::pair<StateID_type, UpdateOpID_type> known_parent) { return parent_id != known_parent.first; }))
            // if (std::any_of(parents.cbegin(), parents.cend(), [parent_id, parent_update_op](const std::pair<StateID_type, UpdateOpID_type> known_parent) { return parent_id == known_parent.first and parent_update_op == known_parent.second; })) { return; }
            parents.emplace_back(parent_id, parent_update_op);
        }

        /**/

        inline void set_goal() {
            PLAJA_ASSERT(not has_goal_path())
            PLAJA_ASSERT(goalDistance == HEURISTIC::unknown)
            // goalPathOp = ACTION::noneUpdateOp;
            goalPathSuccessor = STATE::none;
            set_goal_distance(0);
        }

        inline void set_goal_path_successor(/*UpdateOpID_type goal_path_op,*/ StateID_type goal_path_successor, HeuristicValue_type goal_distance) {
            // PLAJA_ASSERT(goal_path_op != ACTION::noneUpdateOp)
            PLAJA_ASSERT(goal_path_successor != STATE::none)
            PLAJA_ASSERT(goal_distance != HEURISTIC::unknown)
            PLAJA_ASSERT(goal_distance != HEURISTIC::deadEnd)
            // goalPathOp = goal_path_op;
            goalPathSuccessor = goal_path_successor;
            set_goal_distance(goal_distance);
        }

        inline void set_dead_end() {
            PLAJA_ASSERT(not has_goal_path())
            PLAJA_ASSERT(goalDistance == HEURISTIC::unknown)
            // goalPathOp = ACTION::noneUpdateOp;
            goalPathSuccessor = STATE::none;
            goalDistance = HEURISTIC::deadEnd;
        }

        /* Iterator. */

        class ParentsIterator {
            friend SEARCH_SPACE_PA::SearchNodeInfo;

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

        [[nodiscard]] inline SEARCH_SPACE_PA::SearchNodeInfo::ParentsIterator _parents() const { return ParentsIterator(parents); }

#ifndef NDEBUG

        void dump() const {
            for (auto& [p_state, p_op]: parents) { PLAJA_LOG(PLAJA_UTILS::string_f("Parent: %i %i\n", p_state, p_op)) }
            PLAJA_LOG(PLAJA_UTILS::string_f("Goal path successor: %i\n", goalPathSuccessor))
            PLAJA_LOG(PLAJA_UTILS::string_f("Start distance: %i\n", startDistance))
            PLAJA_LOG(PLAJA_UTILS::string_f("Goal distance: %i\n", goalDistance))
            PLAJA_LOG(PLAJA_UTILS::string_f("Current step: %i\n", currentStep))
        }

#endif

    };

/**********************************************************************************************************************/

    struct SearchNode {

    protected:
        SearchNodeInfo* info;
        unsigned currentStep;
        std::unique_ptr<AbstractState> paState;

    public:
        SearchNode(SearchNodeInfo& info_ptr, unsigned current_step, std::unique_ptr<AbstractState> pa_state):
            info(&info_ptr)
            , currentStep(current_step)
            , paState(std::move(pa_state)) {
            PLAJA_ASSERT(currentStep >= 1)
        }

        ~SearchNode() = default;
        DELETE_CONSTRUCTOR(SearchNode)

        /* Getter. */

        [[nodiscard]] inline const SearchNodeInfo& operator()() const { return *info; }

        [[nodiscard]] inline SearchNodeInfo& operator()() { return *info; }

        [[nodiscard]] inline bool is_reached() const {
            PLAJA_ASSERT(info->_current_step() <= currentStep)
            return info->_current_step() >= currentStep;
        }

        FCT_IF_DEBUG([[nodiscard]] inline static bool is_reached(const SearchNodeInfo& info, unsigned int current_step) { return info.currentStep >= current_step; })

        [[nodiscard]] inline bool has_been_reached() const {
            PLAJA_ASSERT(info->_current_step() <= currentStep)
            return info->_current_step() > 0;
        }

        FCT_IF_DEBUG([[nodiscard]] inline static bool has_been_reached(const SearchNodeInfo& info) { return is_reached(info, 0); })

        [[nodiscard]] inline const AbstractState& get_state() const { return *paState; }

        [[nodiscard]] inline StateID_type get_id() const { return paState->get_id_value(); }

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

        /* */

        STMT_IF_DEBUG(void dump() const {
            PLAJA_LOG_DEBUG(PLAJA_UTILS::string_f("Search Node: %i (current step global(%i))", get_id(), currentStep))
            info->dump();
        })

        /******************************************************************************************************************/

        /* Directly access information of extensions. */

        template<typename SearchNodeInfoT>
        [[nodiscard]] const SearchNodeInfoT& cast_info() const { return *PLAJA_UTILS::cast_ptr<SearchNodeInfoT>(info); }

        template<typename SearchNodeInfoT>
        [[nodiscard]] SearchNodeInfoT& cast_info() { return *PLAJA_UTILS::cast_ptr<SearchNodeInfoT>(info); }

    };

}

#endif //PLAJA_SEARCH_NODE_PA_H
