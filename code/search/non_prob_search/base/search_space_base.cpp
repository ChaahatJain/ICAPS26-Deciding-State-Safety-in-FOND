//
// This file is part of the PlaJA code base.
// Copyright (c) (2019 - 2021) Marcel Vinzent.
// See README.md in the top-level directory for licensing information.
//

#include <fstream>
#include "search_space_base.h"
#include "../../../parser/ast/model.h"
#include "../../../utils/utils.h"
#include "../../fd_adaptions/state.h"
#include "../../information/model_information.h"
#include "search_node_info_base.h"

SearchSpaceBase::SearchSpaceBase(const ModelInformation& model_info):
    modelInformation(&model_info)
    , stateRegistry(new StateRegistry(model_info.get_ranges_int(), model_info.get_lower_bounds_int(), model_info.get_upper_bounds_int(), model_info.get_floating_state_size()))
    , initialState(stateRegistry->set_state(model_info.get_initial_values()))
    , initialStates()
    , currentStep(1) {
    initialStates.insert(initialState->get_id_value());
}

SearchSpaceBase::SearchSpaceBase(const class Model& model):
    modelInformation(&model.get_model_information())
    , stateRegistry(new StateRegistry(modelInformation->get_ranges_int(), modelInformation->get_lower_bounds_int(), modelInformation->get_upper_bounds_int(), modelInformation->get_floating_state_size()))
    , initialState(stateRegistry->set_state(modelInformation->get_initial_values()))
    , initialStates()
    , currentStep(1) {
    initialStates.insert(initialState->get_id_value());
}

SearchSpaceBase::~SearchSpaceBase() = default;

void SearchSpaceBase::reset() {
    goalPathFrontier.clear();
    ++currentStep;
}

/**********************************************************************************************************************/

StateID_type SearchSpaceBase::add_initial_state(const StateValues& initial_values) { return *initialStates.insert(stateRegistry->add_state(initial_values).get_id_value()).first; }

/**********************************************************************************************************************/

unsigned int SearchSpaceBase::compute_goal_path_states_recursively(StateID_type id, std::list<StateID_type>& stack) {

    unsigned int counter = 0;

    const auto& node = get_node_info(id);

    for (auto it = node.init_parents_iterator(); !it.end(); ++it) {
        auto& parent_node = get_node_info(it.parent_state());
        if (not parent_node.has_goal_path()) {
            parent_node.set_goal_path_successor(it.parent_update_op(), id, node.get_goal_distance() + 1);
            stack.push_back(it.parent_state());
            ++counter;
        } else { PLAJA_ASSERT_EXPECT(parent_node.get_goal_distance() <= node.get_goal_distance() + 1) }
    }

    return counter;

}

unsigned int SearchSpaceBase::compute_goal_path_states() {

    unsigned int counter = goalPathFrontier.size();

    std::list<StateID_type> goal_path_states;

    /* Goal states. */
    for (const StateID_type goal_state: goalPathFrontier) {
        counter += compute_goal_path_states_recursively(goal_state, goal_path_states);
    }

    /* Goal path states. */
    while (not goal_path_states.empty()) {
        const auto goal_path_state = goal_path_states.front();
        goal_path_states.pop_front();
        counter += compute_goal_path_states_recursively(goal_path_state, goal_path_states);
    }

    return counter;
}

/**********************************************************************************************************************/

State SearchSpaceBase::get_state(StateID_type id) const { return stateRegistry->lookup_state(id); }

std::unique_ptr<State> SearchSpaceBase::get_state_ptr(StateID_type id) const { return stateRegistry->get_state(id); }

std::unique_ptr<State> SearchSpaceBase::add_state_ptr(const StateValues& state) const { return stateRegistry->set_state(state); }

StateID_type SearchSpaceBase::add_state(const StateValues& state) const { return stateRegistry->set_state(state)->get_id_value(); }

/**********************************************************************************************************************/

UpdateOpID_type SearchSpaceBase::compute_update_op(ActionOpID_type op, UpdateIndex_type update) const {
    return modelInformation->compute_update_op_id(op, update);
}

UpdateOpStructure SearchSpaceBase::compute_update_op(UpdateOpID_type update_op) const {
    return modelInformation->compute_update_op_structure(update_op);
}

/** output ************************************************************************************************************/

StatePath SearchSpaceBase::trace_path(StateID_type goal_state) const {
    StatePath path;
    PLAJA_ASSERT(path.empty())
    auto current_state = get_state_ptr(goal_state);
    for (;;) {
        path.push_front(current_state->get_id_value());
        const auto& node_info = get_node_info(current_state->get_id_value());
        if (node_info.is_start()) { break; }
        current_state = get_state_ptr(node_info.get_parent());
    }
    return path;
}

StatePath SearchSpaceBase::dump_path(StateID_type goal_state, const Model* model) const {
    StatePath path = trace_path(goal_state);
    for (const StateID_type state_id: path) { get_state(state_id).dump(model); }
    return path;
}

/** stats *************************************************************************************************************/

void SearchSpaceBase::print_statistics() const {
    std::cout << "Number of registered states: " << stateRegistry->size() << std::endl;
    std::cout << "Bytes per state: " << stateRegistry->get_num_bytes_per_state() << std::endl;
}

void SearchSpaceBase::stats_to_csv(std::ofstream& file) const {
    file << PLAJA_UTILS::commaString << stateRegistry->size();
    file << PLAJA_UTILS::commaString << stateRegistry->get_num_bytes_per_state();
}

void SearchSpaceBase::stat_names_to_csv(std::ofstream& file) { file << ",registered_states,state_size"; }
