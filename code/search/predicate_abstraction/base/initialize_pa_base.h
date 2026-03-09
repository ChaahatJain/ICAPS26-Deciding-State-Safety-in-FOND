//
// This file is part of the PlaJA code base (2019 - 2021).
//

#ifndef PLAJA_INITIALIZE_PA_BASE_H
#define PLAJA_INITIALIZE_PA_BASE_H

#include "search_pa_macros.h"

#pragma GCC diagnostic push
#pragma ide diagnostic ignored "Simplify"

struct InitializePABase {

    template<typename SearchPA_t>
    inline static SearchEngine::SearchStatus initialize(SearchPA_t* const derived_engine) { // i.e., the "this" pointer

        SearchPABase* engine = derived_engine;

        if constexpr (PA_COMPUTATION_AUX::is_path_search<SearchPA_t>()) { // PATH_SEARCH:
            std::cout << "Initializing PA path search ..." << std::endl;
        } else { // PA:
            std::cout << "Initializing predicate abstraction ..." << std::endl;
        }

        engine->searchSpace->compute_initial_states();

        // initialize start states
        for (const StateID_type start_state_id: engine->searchSpace->_initialStates()) {

            // set start state & node
            auto start_node = derived_engine->searchSpace->add_node(engine->searchSpace->get_abstract_state(start_state_id));
            PLAJA_ASSERT(not start_node.is_reached())
            engine->searchStatistics->inc_attr_unsigned(PLAJA::StatsUnsigned::START_STATES);
            engine->searchStatistics->inc_attr_unsigned(PLAJA::StatsUnsigned::GENERATED_STATES);

            // PA-BFS-PER-STATE: cache start states
            PLAJA_ASSERT(PA_COMPUTATION_AUX::is_pa<SearchPA_t>() or not engine->cachedStartStates)
            if constexpr (PA_COMPUTATION_AUX::is_pa<SearchPA_t>()) { // cache
                if (engine->cachedStartStates) {
                    engine->cachedStartStates->push_back(start_state_id);
                    continue;
                }
            }

            // set reached and add to frontier
            start_node.set_reached_init();
            engine->searchStatistics->inc_attr_unsigned(PLAJA::StatsUnsigned::REACHABLE_STATES);
            engine->frontier->push(start_node);

            // if constexpr(PA_COMPUTATION_AUX::is_pa<SearchPA_t>()) { derived_engine->stateSpace->add_state_information(start_state_id); } // PA: maintain state space

            // goal check
            if (engine->expansionPa->check_reach(start_node._state())) {
                std::cout << "Start state is unsafe!" << std::endl;
                start_node.set_goal();
                engine->searchStatistics->inc_attr_unsigned(PLAJA::StatsUnsigned::GOAL_STATES);
                engine->searchSpace->add_goal_path_frontier(start_state_id);

                if constexpr (PA_COMPUTATION_AUX::is_path_search<SearchPA_t>()) { return SearchEngine::SOLVED; } // PATH_SEARCH
            }
        }

        // PA-BFS-PER-STATE: add first start state
        if constexpr (PA_COMPUTATION_AUX::is_pa<SearchPA_t>()) { engine->add_start_state(); }

        return SearchEngine::IN_PROGRESS;
    }

};

#pragma GCC diagnostic pop

#endif //PLAJA_INITIALIZE_PA_BASE_H