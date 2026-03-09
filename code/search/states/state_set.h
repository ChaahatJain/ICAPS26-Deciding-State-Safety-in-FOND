//
// This file is part of the PlaJA code base.
// Copyright (c) (2019 - 2024) Marcel Vinzent.
// See README.md in the top-level directory for licensing information.
//

#ifndef PLAJA_STATE_SET_H
#define PLAJA_STATE_SET_H

#include <unordered_set>

namespace STATE_SET {

    template<typename State_t>
    struct StateHash {
        std::size_t operator()(const State_t* state) const {
            PLAJA_ASSERT(state)
            return state->hash();
        }
    };

    template<typename State_t>
    struct StateEqual {
        bool operator()(const State_t* state1, const State_t* state2) const {
            PLAJA_ASSERT(state1)
            PLAJA_ASSERT(state2)
            return state1->operator==(*state2);
        }
    };

}

typedef std::unordered_set<const StateBase*,
    STATE_SET::StateHash<StateBase>,
    STATE_SET::StateEqual<StateBase>> StateBaseSet;

typedef std::unordered_set<const State*,
    STATE_SET::StateHash<State>,
    STATE_SET::StateEqual<State>> StateSet;

typedef std::unordered_set<const StateValues*,
    STATE_SET::StateHash<StateValues>,
    STATE_SET::StateEqual<StateValues>> StateValuesSet;

#endif //PLAJA_STATE_SET_H
