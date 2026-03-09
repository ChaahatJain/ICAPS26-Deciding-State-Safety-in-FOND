//
// This file is part of the PlaJA code base.
// Copyright (c) (2019 - 2021) Marcel Vinzent.
// See README.md in the top-level directory for licensing information.
//

#include <forward_list>
#include "nn_explorer.h"
#include "../../exception/constructor_exception.h"
#include "../../stats/stats_base.h"
#include "../prob_search/state_space.h"
#include "base/search_node_info_base.h"
#include "base/search_space_base.h"

namespace NN_EXPLORER {

    struct SearchNodeInfo final: public SearchNodeInfoBase {

    private:
        bool inevitableGoalPathState;
        std::unordered_set<StateID_type> parentsUnderPolicy;

    public:
        SearchNodeInfo():
            SearchNodeInfoBase()
            , inevitableGoalPathState(false)
            , parentsUnderPolicy() {
        }

        ~SearchNodeInfo() override = default;

        DEFAULT_CONSTRUCTOR(SearchNodeInfo)

        inline void set_inevitable_goal_path_state() { inevitableGoalPathState = true; }

        inline void add_parent_under_policy(StateID_type state_id) { parentsUnderPolicy.insert(state_id); }

        [[nodiscard]] inline bool is_inevitable_goal_path_state() const { return inevitableGoalPathState; }

        inline const std::unordered_set<StateID_type>& parents_under_policy() { return parentsUnderPolicy; }

    };

    class SearchSpace final: public SearchSpace_<SearchNodeInfo> {
    public:
        explicit SearchSpace(const Model& model):
            SearchSpace_<SearchNodeInfo>(model) {}

        ~SearchSpace() override = default;
        DELETE_CONSTRUCTOR(SearchSpace)
    };

}

/**********************************************************************************************************************/

NN_Explorer::NN_Explorer(const PLAJA::Configuration& config):
    SearchEngineNonProb(config, false, true, false)
    , searchSpace(new NN_EXPLORER::SearchSpace(*model)) {

    if (not get_policy_restriction()) { throw ConstructorException("NN EXPLORER needs policy restriction."); }

    SearchEngineNonProb::set_search_space(searchSpace.get());

    if (searchStatistics) {
        searchStatistics->add_unsigned_stats(PLAJA::StatsUnsigned::InevitableGoalPathStates, "InevitableGoalPathStates", 0);
        searchStatistics->add_unsigned_stats(PLAJA::StatsUnsigned::InevitableGoalPathStatesStart, "InevitableGoalPathStatesStart", 0);
        searchStatistics->add_unsigned_stats(PLAJA::StatsUnsigned::PolicyGoalPathStates, "PolicyGoalPathStates", 0);
        searchStatistics->add_unsigned_stats(PLAJA::StatsUnsigned::PolicyGoalPathStatesStart, "PolicyGoalPathStatesStart", 0);
        searchStatistics->add_unsigned_stats(PLAJA::StatsUnsigned::EvitablePolicyGoalPathStates, "EvitablePolicyGoalPathStates", 0);
    }
}

NN_Explorer::~NN_Explorer() = default;

/* */

void NN_Explorer::add_parent_under_policy(SearchNode& node, StateID_type parent) const {
    node.cast_info<NN_EXPLORER::SearchNodeInfo>().add_parent_under_policy(parent);
}

SearchEngine::SearchStatus NN_Explorer::finalize() {
    compute_goal_path_states();
    compute_inevitable_goal_path_states();
    compute_policy_goal_path_states();
    return SearchStatus::FINISHED;
}

/***********************************************************************************************************************/

namespace NN_EXPLORER {

    inline void compute_inevitable(StateID_type id,
                                   std::forward_list<StateID_type>& states_stack,
                                   std::unordered_set<StateID_type>& known_states,
                                   segmented_vector::SegmentedVector<unsigned int>& number_successors,
                                   SearchSpace& search_space,
                                   const StateSpaceProb& state_space) {

        for (const StateID_type parent_id: state_space.get_state_information(id)._parents()) {

            PLAJA_ASSERT(not known_states.count(parent_id) or number_successors[parent_id] > 0)

            if (--number_successors[parent_id] == 0) {
                if (known_states.insert(parent_id).second) {
                    search_space.get_node_info_cast(parent_id).set_inevitable_goal_path_state();
                    states_stack.push_front(parent_id);
                }
            }

        }
    }

}

void NN_Explorer::compute_inevitable_goal_path_states() {

    const auto& goal_states = searchSpace->get_goal_path_frontier();

    segmented_vector::SegmentedVector<unsigned int> number_successors; // Array of states by id (including unreachable states).
    const std::size_t state_space_size = access_state_space().information_size();
    for (StateID_type state_id = 0; state_id < state_space_size; ++state_id) {
        number_successors.resize(state_id + 1, access_state_space().get_state_information(state_id).number_of_successors());
    }

    std::unordered_set<StateID_type> inevitable_goal_path_states_set(goal_states.cbegin(), goal_states.cend()); // immediately add goal states
    std::forward_list<StateID_type> inevitable_goal_path_states;

    /* Goal states. */
    for (const StateID_type goal_state: goal_states) {
        NN_EXPLORER::compute_inevitable(goal_state, inevitable_goal_path_states, inevitable_goal_path_states_set, number_successors, *searchSpace, access_state_space());
    }

    /* Goal path states. */
    while (not inevitable_goal_path_states.empty()) {
        const StateID_type goal_path_state = inevitable_goal_path_states.front();
        inevitable_goal_path_states.pop_front();
        NN_EXPLORER::compute_inevitable(goal_path_state, inevitable_goal_path_states, inevitable_goal_path_states_set, number_successors, *searchSpace, access_state_space());
    }

    /* Inevitable goal path initial states. */
    unsigned int num_inevitable_goal_path_initial_states = 0;
    for (const StateID_type state_id: searchSpace->_initial_states()) {
        if (inevitable_goal_path_states_set.count(state_id)) { ++num_inevitable_goal_path_initial_states; }
    }

    if (searchStatistics) {
        searchStatistics->set_attr_unsigned(PLAJA::StatsUnsigned::InevitableGoalPathStates, inevitable_goal_path_states_set.size());
        searchStatistics->set_attr_unsigned(PLAJA::StatsUnsigned::InevitableGoalPathStatesStart, num_inevitable_goal_path_initial_states);
    }

}

/***********************************************************************************************************************/

namespace NN_EXPLORER {

    inline unsigned int compute_policy_goal_path_states(StateID_type id,
                                                        std::forward_list<StateID_type>& states_stack,
                                                        std::unordered_set<StateID_type>& known_states,
                                                        SearchSpace& search_space) {

        unsigned num_evitable_policy_goal_path_states(0);

        for (const StateID_type parent_id: search_space.get_node_info_cast(id).parents_under_policy()) {
            if (known_states.insert(parent_id).second) {
                auto& parent_node_info = search_space.get_node_info_cast(parent_id);
                if (not parent_node_info.is_inevitable_goal_path_state()) { ++num_evitable_policy_goal_path_states; }
                states_stack.push_front(parent_id);
            }
        }

        return num_evitable_policy_goal_path_states;

    }

}

void NN_Explorer::compute_policy_goal_path_states() {

    const auto& goal_states = searchSpace->get_goal_path_frontier();

    std::unordered_set<StateID_type> policy_goal_path_states_set(goal_states.cbegin(), goal_states.cend()); // Immediately add goal states.
    std::forward_list<StateID_type> policy_goal_path_states;
    unsigned int num_evitable_policy_goal_path_states = 0;

    /* Goal states. */
    for (const StateID_type goal_state: goal_states) {
        num_evitable_policy_goal_path_states += NN_EXPLORER::compute_policy_goal_path_states(goal_state, policy_goal_path_states, policy_goal_path_states_set, *searchSpace);
    }

    /* Goal path states. */
    while (not policy_goal_path_states.empty()) {
        const StateID_type goal_path_state = policy_goal_path_states.front();
        policy_goal_path_states.pop_front();
        num_evitable_policy_goal_path_states += NN_EXPLORER::compute_policy_goal_path_states(goal_path_state, policy_goal_path_states, policy_goal_path_states_set, *searchSpace);
    }

    /* Policy goal path initial states. */
    unsigned int num_policy_goal_path_initial_states = 0;
    for (const StateID_type state_id: searchSpace->_initial_states()) {
        if (policy_goal_path_states_set.count(state_id)) { ++num_policy_goal_path_initial_states; }
    }

    if (searchStatistics) {
        searchStatistics->set_attr_unsigned(PLAJA::StatsUnsigned::PolicyGoalPathStates, policy_goal_path_states_set.size());
        searchStatistics->set_attr_unsigned(PLAJA::StatsUnsigned::PolicyGoalPathStatesStart, num_policy_goal_path_initial_states);
        searchStatistics->set_attr_unsigned(PLAJA::StatsUnsigned::EvitablePolicyGoalPathStates, num_evitable_policy_goal_path_states); // User service.
    }
}
