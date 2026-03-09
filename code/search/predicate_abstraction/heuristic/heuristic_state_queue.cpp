//
// This file is part of the PlaJA code base.
// Copyright (c) (2019 - 2022) Marcel Vinzent.
// See README.md in the top-level directory for licensing information.
//

#include "heuristic_state_queue.h"
#include "../../factories/configuration.h"
#include "../search_space/search_node_pa.h"
#include "abstract_heuristic.h"

HeuristicStateQueueBase::HeuristicStateQueueBase(const PLAJA::Configuration& config)
#ifdef COMPRESS_GH
    :
    gMax(100)
    , randomizeTieBreaking(config.is_flag_set(PLAJA_OPTION::randomize_tie_breaking))
#endif
{
    if (config.is_flag_set(PLAJA_OPTION::randomize_tie_breaking)) {
        queue = std::make_unique<MapQueue<Priority_t, StateID_type,
#ifdef COMPRESS_GH
            PRIORITY_QUEUE::CompareNumeric<Priority_t, true>,
#else
            PRIORITY_QUEUE::CompareNumericPair<HeuristicValue_type, SearchDepth_type, true>,
#endif
            PRIORITY_QUEUE::BucketRandom<StateID_type>>>();
    } else { // FIFO
        queue = std::make_unique<MapQueue<Priority_t, StateID_type,
#ifdef COMPRESS_GH
            PRIORITY_QUEUE::CompareNumeric<Priority_t, true>,
#else
            PRIORITY_QUEUE::CompareNumericPair<HeuristicValue_type, SearchDepth_type, true>,
#endif
            PRIORITY_QUEUE::BucketFIFO<StateID_type>>>();
    }

}

HeuristicStateQueueBase::~HeuristicStateQueueBase() = default;

/* Interface. */

StateID_type HeuristicStateQueueBase::pop() { return queue->pop(); }

std::size_t HeuristicStateQueueBase::size() const { return queue->size(); }

bool HeuristicStateQueueBase::empty() const { return queue->empty(); }

void HeuristicStateQueueBase::clear() { queue->clear(); }

StateID_type HeuristicStateQueueBase::top() const { return queue->top(); }

std::list<StateID_type> HeuristicStateQueueBase::to_list() const { return queue->to_list(); }

/**********************************************************************************************************************/

HeuristicStateQueue::HeuristicStateQueue(const PLAJA::Configuration& config, std::unique_ptr<AbstractHeuristic>&& heuristic_):
    HeuristicStateQueueBase(config)
    , heuristic(std::move(heuristic_)) {
}

HeuristicStateQueue::~HeuristicStateQueue() = default;

std::unique_ptr<StateQueue> HeuristicStateQueue::make_state_queue(const PLAJA::Configuration& config) { return AbstractHeuristic::make_state_queue(config); }

void HeuristicStateQueue::increment() {
    clear();
    heuristic->increment();
}

void HeuristicStateQueue::push(const SEARCH_SPACE_PA::SearchNodeView& state_node) {
#ifdef COMPRESS_GH
    const auto g = state_node().get_start_distance();
    const auto h = heuristic->evaluate(state_node.get_state());
    if (g >= gMax) { // recompute priority
        const auto g_max_new = gMax * 2; // exponential growth
        std::unique_ptr<PriorityQueue<Priority_t, StateID_type>> queue_new(nullptr);

        if (randomizeTieBreaking) {
            queue_new = std::make_unique<MapQueue<Priority_t, StateID_type,
                PRIORITY_QUEUE::CompareNumeric<Priority_t, true>,
                PRIORITY_QUEUE::BucketRandom<StateID_type>>>();
        } else {
            queue_new = std::make_unique<MapQueue<Priority_t, StateID_type,
                PRIORITY_QUEUE::CompareNumeric<Priority_t, true>,
                PRIORITY_QUEUE::BucketFIFO<StateID_type>>>();
        }

        while (not queue->empty()) {
            auto entry = queue->pop_with_priority();
            const auto h_ = entry.first / gMax;
            const auto g_ = entry.first % gMax;
            queue_new->push(h_ * g_max_new + g_, StateID_type(entry.second));
        }

        queue = std::move(queue_new);
        gMax = g_max_new;
    }
    queue->push(h * gMax + g, state_node.get_id());
#else
    queue->push({ heuristic->evaluate(state_node.get_state()), state_node().get_start_distance() }, state_node.get_id());
#endif
}

/** stats *************************************************************************************************************/

void HeuristicStateQueue::print_statistics() const { heuristic->print_statistics(); }

void HeuristicStateQueue::stats_to_csv(std::ofstream& file) const { heuristic->stats_to_csv(file); }

void HeuristicStateQueue::stat_names_to_csv(std::ofstream& file) const { heuristic->stat_names_to_csv(file); }

/**********************************************************************************************************************/

ConstHeuristicStateQueue::ConstHeuristicStateQueue(const PLAJA::Configuration& config, const AbstractHeuristic* heuristic):
    HeuristicStateQueueBase(config)
    , heuristic(heuristic) {
}

ConstHeuristicStateQueue::~ConstHeuristicStateQueue() = default;

void ConstHeuristicStateQueue::increment() { clear(); }

void ConstHeuristicStateQueue::push(const SEARCH_SPACE_PA::SearchNodeView& state_node) {
#ifdef COMPRESS_GH
    const auto g = state_node().get_start_distance();
    const auto h = heuristic->evaluate(state_node.get_state());
    if (g >= gMax) { // recompute priority
        const auto g_max_new = gMax * 2; // exponential growth
        std::unique_ptr<PriorityQueue<Priority_t, StateID_type>> queue_new(nullptr);

        if (randomizeTieBreaking) {
            queue_new = std::make_unique<MapQueue<Priority_t, StateID_type,
                PRIORITY_QUEUE::CompareNumeric<Priority_t, true>,
                PRIORITY_QUEUE::BucketRandom<StateID_type>>>();
        } else {
            queue_new = std::make_unique<MapQueue<Priority_t, StateID_type,
                PRIORITY_QUEUE::CompareNumeric<Priority_t, true>,
                PRIORITY_QUEUE::BucketFIFO<StateID_type>>>();
        }

        while (not queue->empty()) {
            auto entry = queue->pop_with_priority();
            const auto h_ = entry.first / gMax;
            const auto g_ = entry.first % gMax;
            queue_new->push(h_ * g_max_new + g_, StateID_type(entry.second));
        }

        queue = std::move(queue_new);
        gMax = g_max_new;
    }
    queue->push(h * gMax + g, state_node.get_id());
#else
    queue->push({ heuristic->evaluate(state_node.get_state()), state_node().get_start_distance() }, state_node.get_id());
#endif
}
