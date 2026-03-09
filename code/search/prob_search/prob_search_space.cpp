//
// This file is part of the PlaJA code base.
// Copyright (c) (2019 - 2020) Marcel Vinzent.
// See README.md in the top-level directory for licensing information.
//

#include <fstream>
#include <sstream>
#include "prob_search_space.h"
#include "../information/model_information.h"
#include "state_space.h"

// extern:
namespace PLAJA_UTILS { extern const std::string commaString; }

namespace PROB_SEARCH_SPACE {

    template<typename PSearchNodeInfo_t>
    void dump_policy_graph(StateID_type root_state, const StateSpaceProb& state_space, const segmented_vector::SegmentedVector<PSearchNodeInfo_t>& data, std::ostream& out) {
        std::ostringstream graph;
        std::list<StateID_type> states;
        states.push_back(root_state);

        for (StateID_type state_id: states) {
            const PSearchNodeInfo_t& node_info = data[state_id];
            if (ProbSearchNodeInfo::GOAL == node_info.status || ProbSearchNodeInfo::DEAD_END == node_info.status) {
                continue;
            }
            LocalOpIndex_type local_op = node_info.policy_choice;
            if (local_op < 0) {
                continue;
            }
            for (auto per_op_it = state_space.perOpTransitionIterator(state_id, local_op); !per_op_it.end(); ++per_op_it) {
                graph << "s" << state_id << " -> " << "s" << per_op_it.id() << std::endl;
                if (std::find(states.begin(), states.end(), per_op_it.id()) == states.end()) { // could use mark & unmark, but then possible interaction with other code, moreover printing need not be efficient
                    states.push_back(per_op_it.id());
                }
            }
        }

        out << "digraph {" << std::endl;
        for (StateID_type state_id: states) {
            const PSearchNodeInfo_t& node_info = data[state_id];
            out << "s" << state_id << " [label=\"" << state_id << ": "
                << node_info.v_current << "\"";
            if (ProbSearchNodeInfo::GOAL == node_info.status) {
                out << ", shape=rectangle";
            }
            if (ProbSearchNodeInfo::DEAD_END == node_info.status || ProbSearchNodeInfo::GOAL == node_info.status || ProbSearchNodeInfo::DONT_CARE == node_info.status) {
                out << ", peripheries=2";
            }
            out << "]" << std::endl;
        }
        out << graph.str() << "}" << std::endl;

    }
}

//

ProbSearchSpaceBase::ProbSearchSpaceBase(const ModelInformation& modelInformation):
    stateRegistry(new StateRegistry(modelInformation.get_ranges_int(), modelInformation.get_lower_bounds_int(), modelInformation.get_upper_bounds_int(), modelInformation.get_floating_state_size())),
    initialState(stateRegistry->set_state(StateValues(modelInformation.get_initial_values()))),
    stateSpace(),
    current_step(1) {

}

ProbSearchSpaceBase::~ProbSearchSpaceBase() = default;

// output & stats

void ProbSearchSpaceBase::dump_policy_graph_aux(StateID_type root_state, const segmented_vector::SegmentedVector<ProbSearchNodeInfo>& data, std::ostream& out) const {
    PROB_SEARCH_SPACE::dump_policy_graph(root_state, stateSpace, data, out);
}

void ProbSearchSpaceBase::dump_policy_graph_aux(StateID_type root_state, const segmented_vector::SegmentedVector<UpperAndLowerPNodeInfo>& data, std::ostream& out) const {
    PROB_SEARCH_SPACE::dump_policy_graph(root_state, stateSpace, data, out);
}

void ProbSearchSpaceBase::print_statistics() const {
    std::cout << "Number of registered states: " << stateRegistry->size() << std::endl;
    std::cout << "Bytes per state: " << stateRegistry->get_num_bytes_per_state() << std::endl;
}

void ProbSearchSpaceBase::stats_to_csv(std::ofstream& file) const {
    file << PLAJA_UTILS::commaString << stateRegistry->size();
    file << PLAJA_UTILS::commaString << stateRegistry->get_num_bytes_per_state();
}

void ProbSearchSpaceBase::stat_names_to_csv(std::ofstream& file) {
    file << ",registered_states,state_size";
}
