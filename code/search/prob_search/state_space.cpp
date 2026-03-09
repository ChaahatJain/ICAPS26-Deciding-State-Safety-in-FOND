//
// This file is part of the PlaJA code base.
// Copyright (c) (2019 - 2020) Marcel Vinzent.
// See README.md in the top-level directory for licensing information.
//

#include "state_space.h"
#include "../../utils/rng.h"

StateSpaceProb::StateSpaceProb():
    StateSpaceBase<STATE_SPACE::StateInformationProb, PLAJA::Prob_type>()
    , reverseAbstraction()
    , rng(PLAJA_GLOBAL::rng.get()) {

}

StateSpaceProb::~StateSpaceProb() = default;

//

void StateSpaceProb::replace_state(StateID_type state1, StateID_type state2) {
    STATE_SPACE::StateInformationProb& info1 = add_state_information(state1);
    PLAJA_ASSERT(info1.abstractedTo == STATE::none)
    info1.abstractedTo = state2;

    // state1 is abstracted to state2
    std::vector<StateID_type>& state_mapping = reverseAbstraction[state2];
    state_mapping.push_back(state1);

    // update states abstracted to state1 to now abstract to state2
    auto it = reverseAbstraction.find(state1);
    if (it != reverseAbstraction.end()) {
        for (const StateID_type i: it->second) {
            STATE_SPACE::StateInformationProb& info = add_state_information(i);
            PLAJA_ASSERT(info.abstractedTo == state1)
            info.abstractedTo = state2;
        }
        state_mapping.insert(state_mapping.end(), it->second.begin(), it->second.end());
        reverseAbstraction.erase(it);
    }
}

StateID_type StateSpaceProb::sample(StateID_type state_id, LocalOpIndex_type local_op) const {
    PLAJA_ASSERT(not get_state_information(state_id)._transitions(local_op).empty()) // local op within bound is checked by iterator
    PLAJA::Prob_type r = rng->prob();
    for (auto transition_it = perOpTransitionIterator(state_id, local_op); !transition_it.end(); ++transition_it) {
        if (transition_it.prob() > r) { return transition_it.id(); }
        else { r -= transition_it.prob(); }
    }
    PLAJA_ABORT
}

//

void StateSpaceProb::dump_state(StateID_type id) const {
    PLAJA_ASSERT(id < cachedStateInformation.size())
    std::cout << "State information of " << id;
    const STATE_SPACE::StateInformationProb& state_info = cachedStateInformation[id];
    if (state_info.is_abstracted()) {
        std::cout << " (abstracted to " << state_info.abstractedTo << ")" << std::endl;
    } else {
        std::cout << std::endl;
    }

    std::cout << "Successor information" << std::endl;
    for (auto app_op_it = applicableOpIterator(id); !app_op_it.end(); ++app_op_it) {
        std::cout << "\tfor action operator ID: " << app_op_it.action_op_id() << std::endl;
        std::cout << "\t";
        for (auto per_op_it = app_op_it.transitions_of_op(); !per_op_it.end(); ++per_op_it) {
            std::cout << "(" << per_op_it.id() << "," << per_op_it.info() << ")" << ",";
        }
        std::cout << std::endl;
    }

    std::cout << "Parent information" << std::endl << "\t";
    for (const StateID_type parent: state_info._parents()) { std::cout << parent << ","; }

    std::cout << std::endl;
}

void StateSpaceProb::dump_state_space() const {
    const std::size_t l = cachedStateInformation.size();
    std::cout << "State space{" << std::endl;
    for (std::size_t i = 0; i < l; ++i) { dump_state(i); }
    std::cout << "}" << std::endl;
}

/**********************************************************************************************************************/

#if 0
StateSpacePA::StateSpacePA() = default;

StateSpacePA::~StateSpacePA() = default;
#endif