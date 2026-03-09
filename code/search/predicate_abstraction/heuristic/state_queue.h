//
// This file is part of the PlaJA code base.
// Copyright (c) (2019 - 2022) Marcel Vinzent.
// See README.md in the top-level directory for licensing information.
//

#ifndef PLAJA_STATE_QUEUE_H
#define PLAJA_STATE_QUEUE_H

#include <list>
#include "../../../utils/default_constructors.h"
#include "../../../stats/stats_interface.h"
#include "../../using_search.h"
#include "../search_space/forward_search_space.h"

/** Base class to order states based on heuristic values */
class StateQueue: public PLAJA::StatsInterface {

protected:

    StateQueue();
public:
    virtual ~StateQueue() = 0;
    DELETE_CONSTRUCTOR(StateQueue)

    virtual void increment() = 0;

    virtual void push(const SEARCH_SPACE_PA::SearchNodeView& state_node) = 0;

    /* Interface. */
    virtual StateID_type pop() = 0;
    [[nodiscard]] virtual std::size_t size() const = 0;
    [[nodiscard]] virtual bool empty() const = 0;
    virtual void clear() = 0;
    /* */
    [[nodiscard]] virtual StateID_type top() const = 0;
    [[nodiscard]] virtual std::list<StateID_type> to_list() const = 0;
};

class FIFOStateQueue final: public StateQueue {
private:
    std::list<StateID_type> queue;
public:
    FIFOStateQueue();
    ~FIFOStateQueue() final;
    DELETE_CONSTRUCTOR(FIFOStateQueue)

    /* Interface. */
    StateID_type pop() override;
    [[nodiscard]] std::size_t size() const override;
    [[nodiscard]] bool empty() const override;
    void clear() override;
    /* */
    [[nodiscard]] StateID_type top() const override;
    [[nodiscard]] std::list<StateID_type> to_list() const override;

    void increment() override { clear(); }

    void push(const SEARCH_SPACE_PA::SearchNodeView& state_node) final;
};

#endif //PLAJA_STATE_QUEUE_H
