//
// This file is part of the PlaJA code base.
// Copyright (c) (2019 - 2022) Marcel Vinzent.
// See README.md in the top-level directory for licensing information.
//

#ifndef PLAJA_HEURISTIC_STATE_QUEUE_H
#define PLAJA_HEURISTIC_STATE_QUEUE_H

#include <memory>
#include "../../../utils/priority_queue.h"
#include "../../factories/forward_factories.h"
#include "state_queue.h"

class AbstractHeuristic;

#define COMPRESS_GH

class HeuristicStateQueueBase: public StateQueue {
protected:
#ifdef COMPRESS_GH
    using Priority_t = int;
    Priority_t gMax;
    const bool randomizeTieBreaking;
#else
    using Priority_t = std::pair<HeuristicValue_type, SearchDepth_type>;
#endif
    std::unique_ptr<PriorityQueue<Priority_t, StateID_type>> queue;

    explicit HeuristicStateQueueBase(const PLAJA::Configuration& config);
public:
    ~HeuristicStateQueueBase() override;
    DELETE_CONSTRUCTOR(HeuristicStateQueueBase)

    /* Interface. */
    StateID_type pop() override;
    [[nodiscard]] std::size_t size() const override;
    [[nodiscard]] bool empty() const override;
    void clear() override;
    /* */
    [[nodiscard]] StateID_type top() const override;
    [[nodiscard]] std::list<StateID_type> to_list() const override;
};

/**********************************************************************************************************************/

class HeuristicStateQueue final: public HeuristicStateQueueBase {
private:
    std::unique_ptr<AbstractHeuristic> heuristic;

public:
    explicit HeuristicStateQueue(const PLAJA::Configuration& config, std::unique_ptr<AbstractHeuristic>&& heuristic);
    ~HeuristicStateQueue() final;
    DELETE_CONSTRUCTOR(HeuristicStateQueue)

    static std::unique_ptr<StateQueue> make_state_queue(const PLAJA::Configuration& config);

    void increment() override;

    void push(const SEARCH_SPACE_PA::SearchNodeView& state_node) final;

    /* Stats. */
    void print_statistics() const override;
    void stats_to_csv(std::ofstream& file) const override;
    void stat_names_to_csv(std::ofstream& file) const override;
};

/**********************************************************************************************************************/

class ConstHeuristicStateQueue final: public HeuristicStateQueueBase {
private:
    const AbstractHeuristic* heuristic;
    DELETE_CONSTRUCTOR(ConstHeuristicStateQueue)

public:
    explicit ConstHeuristicStateQueue(const PLAJA::Configuration& config, const AbstractHeuristic* heuristic);
    ~ConstHeuristicStateQueue() final;

    void increment() override;

    void push(const SEARCH_SPACE_PA::SearchNodeView& state_node) final;
};

#endif //PLAJA_HEURISTIC_STATE_QUEUE_H
