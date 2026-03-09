//
// This file is part of the PlaJA code base.
// Copyright (c) (2019 - 2022) Marcel Vinzent.
// See README.md in the top-level directory for licensing information.
//

#include "state_queue.h"
#include "../search_space/search_node_pa.h"

StateQueue::StateQueue() = default;

StateQueue::~StateQueue() = default;

/**********************************************************************************************************************/

FIFOStateQueue::FIFOStateQueue() = default;

FIFOStateQueue::~FIFOStateQueue() = default;

void FIFOStateQueue::push(const SEARCH_SPACE_PA::SearchNodeView& state_node) { queue.push_back(state_node.get_id()); }

/* Interface. */

StateID_type FIFOStateQueue::pop() {
    auto id = queue.front();
    queue.pop_front();
    return id;
}

std::size_t FIFOStateQueue::size() const { return queue.size(); }

bool FIFOStateQueue::empty() const { return queue.empty(); }

void FIFOStateQueue::clear() { queue.clear(); }

StateID_type FIFOStateQueue::top() const { return queue.front(); }

std::list<StateID_type> FIFOStateQueue::to_list() const { return queue; }
