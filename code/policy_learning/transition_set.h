//
// This file is part of the PlaJA code base.
// Copyright (c) (2019 - 2023) Marcel Vinzent.
// See README.md in the top-level directory for licensing information.
//

#ifndef PLAJA_TRANSITION_SET_H
#define PLAJA_TRANSITION_SET_H

#include <unordered_set>

namespace QL_AGENT {

    struct Transition {

        StateID_type src;
        ActionLabel_type label;
        StateID_type successor;

        Transition(StateID_type src, ActionLabel_type label, StateID_type successor):
            src(src)
            , label(label)
            , successor(successor) {
        }

    };

    struct TransitionHash {
        std::size_t operator()(const Transition& transition) const {
            constexpr unsigned prime = 31;
            std::size_t result = 1;
            result = prime * result + transition.src;
            result = prime * result + transition.label;
            result = prime * result + transition.successor;
            return result;
        }
    };

    struct TransitionEqual {
        bool operator()(const Transition& transition1, const Transition& transition2) const {
            return transition1.src == transition2.src and transition1.label == transition2.label and transition1.successor == transition2.successor;
        }
    };

    typedef std::unordered_set<Transition, TransitionHash, TransitionEqual> TransitionSet;

}

#endif //PLAJA_TRANSITION_SET_H
