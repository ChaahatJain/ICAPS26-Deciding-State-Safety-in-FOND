//
// This file is part of the PlaJA code base.
// Copyright (c) (2019 - 2021) Marcel Vinzent.
// See README.md in the top-level directory for licensing information.
//

#include "search_engine_non_prob.h"
#include "../../../parser/ast/expression/expression.h"
#include "../../../parser/ast/model.h"
#include "../../../option_parser/plaja_options.h"
#include "../../../stats/stats_base.h"
#include "../../factories/non_prob_search/non_prob_search_options.h"
#include "../../factories/configuration.h"
#include "../../fd_adaptions/search_statistics.h"
#include "../../fd_adaptions/state.h"
#include "../../information/model_information.h"
#include "../../information/property_information.h"
#include "../../prob_search/state_space.h"
#include "../../successor_generation/successor_generator.h"
#include "../initial_states_enumerator.h"
#include "../policy/policy_restriction.h"
#include "search_node_info_base.h"
#include "search_space_base.h"

SearchEngineNonProb::SearchEngineNonProb(const PLAJA::Configuration& config, bool goal_path_search, bool init_state_space, bool only_policy_transitions):
    SearchEngine(config)
    , searchSpace(nullptr)
    , initialStatesEnumerator(propertyInfo->get_start() ? new InitialStatesEnumerator(config, *propertyInfo->get_start()) : nullptr)
    , successorGenerator(new SuccessorGenerator(config, *model))
    , policyRestriction(PolicyRestriction::construct_policy_restriction(config))
    , goal(propertyInfo->get_reach())
    , frontier()
    , stateSpace(not init_state_space and config.get_bool_option(PLAJA_OPTION::suppressStateSpace) ? nullptr : new StateSpaceProb())
    , currentStateInfo(nullptr)
    , searchPerStart(config.get_bool_option(PLAJA_OPTION::search_per_start))
    , goalPathSearch(goal_path_search)
    , onlyPolicyTransitions(only_policy_transitions) {
    SEARCH_STATISTICS::add_basic_stats(*searchStatistics);
}

SearchEngineNonProb::~SearchEngineNonProb() = default;

/* */

bool SearchEngineNonProb::check_goal(const StateBase& state) const {

    if (PLAJA_GLOBAL::enableTerminalStateSupport and not PLAJA_GLOBAL::reachMayBeTerminal) {
        if (successorGenerator->get_terminal_state_condition() and successorGenerator->is_terminal(state)) { return false; }
    }

    return goal and goal->evaluate_integer(state);

}

STMT_IF_DEBUG(bool notReached(false);)

SearchEngine::SearchStatus SearchEngineNonProb::initialize() {
    PLAJA_LOG("Initializing ...")

    /* Compute additional initial states. */
    PLAJA_ASSERT(propertyInfo)
    if (initialStatesEnumerator) {
        initialStatesEnumerator->initialize();
        if (not searchPerStart) {
            for (const auto& initial_state_values: initialStatesEnumerator->enumerate_states()) { searchSpace->add_initial_state(initial_state_values); }
            if constexpr (not PLAJA_GLOBAL::debug) { initialStatesEnumerator = nullptr; };
        }
    }

    PLAJA_ASSERT(not searchSpace->_initial_states().empty())

    for (const auto initial_id: searchSpace->_initial_states()) {
        STMT_IF_DEBUG(notReached = true;)
        if (initialize(initial_id) == SearchEngine::SOLVED) { return SearchEngine::SOLVED; }
        STMT_IF_DEBUG(notReached = false;)
    }

    return SearchEngine::IN_PROGRESS;

}

SearchEngine::SearchStatus SearchEngineNonProb::initialize(StateID_type initial_id) {
    const auto& initial_state = searchSpace->get_state(initial_id);
    searchStatistics->inc_attr_unsigned(PLAJA::StatsUnsigned::GENERATED_STATES);
    searchStatistics->inc_attr_unsigned(PLAJA::StatsUnsigned::START_STATES);

    auto initial_node = searchSpace->add_node(initial_id);
    PLAJA_ASSERT(not notReached or not initial_node.is_reached())
    if (initial_node.is_reached()) { return SearchStatus::IN_PROGRESS; } // TODO For now do not update start distance in node ...

    initial_node.set_reached_init();
    searchStatistics->inc_attr_unsigned(PLAJA::StatsUnsigned::REACHABLE_STATES);
    if (stateSpace) { stateSpace->add_state_information(initial_id); }

    /* Goal check. */
    if (check_goal(initial_state)) {
        PLAJA_LOG("Initial state is goal!")
        searchStatistics->inc_attr_unsigned(PLAJA::StatsUnsigned::GOAL_STATES);
        searchSpace->add_goal_path_frontier(initial_id);
        initial_node().set_goal();
        if (goalPathSearch) { return SearchEngine::SOLVED; }
    }

    /* Add to frontier. */
    frontier.push_back(initial_id);

    return SearchEngine::IN_PROGRESS;
}

SearchEngine::SearchStatus SearchEngineNonProb::add_start_state() {

    PLAJA_ASSERT(frontier.empty())

    while (initialStatesEnumerator) {

        const auto start_state = initialStatesEnumerator->retrieve_state();

        if (not start_state) {
            initialStatesEnumerator = nullptr;
            return SearchStatus::SOLVED;
        }

        STMT_IF_DEBUG(const auto num_starts = searchSpace->_initial_states().size())
        const auto start_id = searchSpace->add_initial_state(*start_state);
        PLAJA_ASSERT(searchSpace->_initial_states().size() == num_starts + 1 or *start_state == searchSpace->get_initial_state())

        if (initialize(start_id) == SearchStatus::SOLVED) { return SearchStatus::SOLVED; };

        if (not frontier.empty()) { return SearchStatus::IN_PROGRESS; };

    }

    return SearchStatus::SOLVED;

}

SearchEngine::SearchStatus SearchEngineNonProb::step() {

    if (frontier.empty()) {
        const auto rlt = add_start_state();
        if (rlt == SearchEngine::SOLVED) {
            if (goalPathSearch and searchSpace->goal_path_frontier_empty()) { PLAJA_LOG("Goal unreachable!") }
            return SearchEngine::SOLVED;
        }
        PLAJA_ASSERT(not frontier.empty())
    }

    /* Expand state. */
    const StateID_type current_id = frontier.front();
    frontier.pop_front();
    searchStatistics->inc_attr_unsigned(PLAJA::StatsUnsigned::EXPANDED_STATES);

    /* Explore. */
    const State current_state = searchSpace->get_state(current_id);
    const auto& current_node_info = searchSpace->get_node_info(current_id);
    successorGenerator->explore(current_state);

    /* State space maintenance. */
    if (stateSpace) {
        currentStateInfo = &stateSpace->get_state_information(current_id);
        currentStateInfo->reserve(successorGenerator->_number_of_enabled_ops());
    }

    /* Terminal check. */
    bool is_terminal = true;
    int is_policy_terminal = -1;

    // Iterate
    for (auto action_it = successorGenerator->init_action_it_per_explore(); !action_it.end(); ++action_it) { // For each action ...
        const auto action_label = action_it.action_label();

        bool is_chosen_by_policy(false);

        // policy restriction
        if (policyRestriction and policyRestriction->is_pruned(current_state, action_label)) {
            is_policy_terminal = is_policy_terminal == -1 ? 1 : 0;
            if (onlyPolicyTransitions) { continue; } // Only explore for chosen actions ...
        } else {
            is_policy_terminal = 0;
            is_chosen_by_policy = true;
        }

        searchStatistics->inc_attr_unsigned(PLAJA::StatsUnsigned::ACTION_LABELS); // Applicable action, thus at least one transition.
        is_terminal = false;

        for (auto op_it = action_it.iterator(); !op_it.end(); ++op_it) {
            const ActionOpInit& action_op = *op_it;

            PLAJA_ASSERT(action_op.size() > 0) // At least one transition.
            searchStatistics->inc_attr_unsigned(PLAJA::StatsUnsigned::ACTION_OPS);
            searchStatistics->inc_attr_unsigned(PLAJA::StatsUnsigned::TRANSITIONS, PLAJA_UTILS::cast_numeric<unsigned>(action_op.size()));

            if (stateSpace) { currentStateInfo->add_operator(action_op._op_id(), action_op.size()); } // Maintain state space.

            for (auto transition_it = action_op.transitionIterator(); !transition_it.end(); ++transition_it) { // for each update of the operator
                std::unique_ptr<State> successor_state = successorGenerator->compute_successor(transition_it.transition(), current_state);
                searchStatistics->inc_attr_unsigned(PLAJA::StatsUnsigned::GENERATED_STATES);

                const StateID_type successor_id = successor_state->get_id_value();
                SearchNode successor_node = searchSpace->add_node(successor_id);

                if (stateSpace) {
                    currentStateInfo->add_successor_to_current_op(successor_id, transition_it.prob());

                    /* Policy. */
                    if (not onlyPolicyTransitions) {
                        if (is_chosen_by_policy) { add_parent_under_policy(successor_node, current_id); }
                    }

                    if (not stateSpace->add_parent(successor_id, current_id)) { continue; } // If parent already added, then state already reached via current state.
                }

                /* Have seen state in previous iterations, can reuse information? */
                if (successor_node.has_been_reached() and not successor_node.is_reached()) {
                    PLAJA_ABORT // TODO Unexpected on current benchmark set, since operations are invertible.

                    successor_node().add_parent(current_id, searchSpace->compute_update_op(action_op._op_id(), transition_it.update_index()));

                    if (successor_node().has_goal_path()) { searchSpace->add_goal_path_frontier(successor_id); }
                    else if (not successor_node().known_goal_distance()) { successor_node().set_dead_end(); } // Has been reached, but no goal state found -> so dead end.
                    else { PLAJA_ASSERT(successor_node().is_dead_end()) }

                    continue;
                }

                /* Reach: */
                if (not successor_node.is_reached()) {
                    successor_node.set_reached(current_id, searchSpace->compute_update_op(action_op._op_id(), transition_it.update_index()), current_node_info.get_start_distance() + 1);
                    searchStatistics->inc_attr_unsigned(PLAJA::StatsUnsigned::REACHABLE_STATES);

                    /* Goal check: */
                    if (check_goal(*successor_state)) {
                        searchStatistics->inc_attr_unsigned(PLAJA::StatsUnsigned::GOAL_STATES);
                        searchSpace->add_goal_path_frontier(successor_id);
                        successor_node().set_goal();
                        if (goalPathSearch) { return SearchEngine::SOLVED; }
                    }

                    frontier.push_back(successor_id);

                } else if (not goalPathSearch) {
                    /*
                     * Add_parent assumes that it is called for each source and successor state once.
                     * For non-goalPathSearch this is guaranteed via the "stateSpace->add_parent" call above.
                     */
                    PLAJA_ASSERT(stateSpace)
                    successor_node().add_parent(current_id, searchSpace->compute_update_op(action_op._op_id(), transition_it.update_index()));
                }

            }

        }

    }

    /* Terminal check. */
    if (is_terminal) {
        searchStatistics->inc_attr_unsigned(PLAJA::StatsUnsigned::TERMINAL_STATES);
        if (goalPathSearch) { searchStatistics->inc_attr_unsigned(PLAJA::StatsUnsigned::DEAD_END_STATES); } // Also a dead-end as goal states will not be expanded.
    }
    if (is_policy_terminal == 1) { searchStatistics->inc_attr_unsigned(PLAJA::StatsUnsigned::POLICY_TERMINAL_STATES); }

    return SearchEngine::IN_PROGRESS;
}

void SearchEngineNonProb::add_parent_under_policy(SearchNode&, StateID_type) const { PLAJA_ABORT }

/**********************************************************************************************************************/

void SearchEngineNonProb::compute_goal_path_states() {
    PLAJA_ASSERT(get_status() == SearchEngine::SOLVED or get_status() == SearchEngine::FINISHED)
    if (searchStatistics) { searchStatistics->set_attr_unsigned(PLAJA::StatsUnsigned::GoalPathStates, searchSpace->compute_goal_path_states()); }
    else { searchSpace->compute_goal_path_states(); }
}

/* */

StateID_type SearchEngineNonProb::search_goal_path(const StateValues& state) {
    const auto id = searchSpace->add_state(state);

    auto node = searchSpace->add_node(id);

    if (node().known_goal_distance()) { return id; }

    if (node.has_been_reached()) { // Has been reached, but no goal path found. Hence, dead-end.
        node().set_dead_end();
        return id;
    }

    /* Search. */

    frontier.clear();
    searchSpace->reset();

    auto rlt = initialize(id);
    PLAJA_ASSERT_EXPECT(rlt == SearchStatus::IN_PROGRESS)

    while (SearchEngine::SearchStatus::IN_PROGRESS == rlt) { rlt = step(); }

    // Set goal information.
    if (searchSpace->goal_path_frontier_empty()) { searchSpace->get_node_info(id).set_dead_end(); } // IMPORTANT: Reload node since information ptr may have been invalidated due to reallocation.
    else {
        searchSpace->compute_goal_path_states();
        PLAJA_ASSERT(searchSpace->get_node_info(id).has_goal_path())
    }

    return id;
}

HeuristicValue_type SearchEngineNonProb::get_goal_distance(StateID_type state) const {
    return searchSpace->get_node_info(state).get_goal_distance();
}

UpdateOpID_type SearchEngineNonProb::get_goal_op(StateID_type state) const {
    return searchSpace->get_node_info(state).get_goal_path_op();
}

/* */

struct UpdateOpStructure SearchEngineNonProb::get_update_op(UpdateOpID_type update_op) const { return searchSpace->compute_update_op(update_op); }

ActionLabel_type SearchEngineNonProb::get_label(ActionOpID_type op_id) const {
    const auto& model_info = model->get_model_information();
    const auto op_structure = model_info.compute_action_op_id_structure(op_id);
    return ModelInformation::action_id_to_label(model_info.sync_id_to_action(op_structure.syncID));
}

FCT_IF_DEBUG(std::string SearchEngineNonProb::get_label_name(ActionLabel_type label) const { return model->get_action_name(ModelInformation::action_label_to_id(label)); })

/* Stats. */

void SearchEngineNonProb::print_statistics() const {
    searchStatistics->print_statistics();
    searchSpace->print_statistics();
}

void SearchEngineNonProb::stats_to_csv(std::ofstream& file) const {
    searchStatistics->stats_to_csv(file);
    searchSpace->stats_to_csv(file);
}

void SearchEngineNonProb::stat_names_to_csv(std::ofstream& file) const {
    searchStatistics->stat_names_to_csv(file);
    SearchSpaceBase::stat_names_to_csv(file);
}
