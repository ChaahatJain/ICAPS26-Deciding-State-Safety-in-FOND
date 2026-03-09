//
// This file is part of the PlaJA code base.
// Copyright (c) (2019 - 2020) Marcel Vinzent.
// See README.md in the top-level directory for licensing information.
//

#pragma GCC diagnostic push
#pragma ide diagnostic ignored "Simplify"

#include "search_pa_base.h"
#include "../../../stats/stats_base.h"
#include "../../factories/predicate_abstraction/search_engine_config_pa.h"
#include "../../information/property_information.h"
#include "../heuristic/heuristic_state_queue.h"
#include "../pa_states/abstract_path.h"
#include "../search_space/search_space_pa_base.h"
#include "../smt/model_z3_pa.h"
#include "../search_statistics_pa.h"
#include "expansion_pa_base.h"
#include "../../../option_parser/plaja_options.h"


// extern:
namespace MODEL_MARABOU_PA { extern void make_sharable(const PLAJA::Configuration& config); }

namespace MODEL_VERITAS_PA { extern void make_sharable(const PLAJA::Configuration& config); }


SearchPABase::SearchPABase(const SearchEngineConfigPA& config, bool goal_path_search, bool optimal_search, bool witness_interval):
    SearchEngine(config)
    , statsHeuristic(config.get_sharable_ptr<PLAJA::StatsBase>(PLAJA::SharableKey::STATS_HEURISTIC))
    , searchSpace(nullptr)
    , expansionPa(nullptr)
    , cachedStartStates(config.get_bool_option(PLAJA_OPTION::search_per_start) ? new std::list<StateID_type>() : nullptr)
    // solver(*modelZ3),
    // goalChecker(new StateConditionCheckerPA(*modelZ3)),
    // predicateRelations(nullptr),
    // jani2NNet(propertyInfo->get_nn_interface() ? propertyInfo->get_nn_interface() : nullptr)
    // stallingDetectionEnabled(optionParser.is_flag_set(PLAJA_OPTION::stalling_detection)),
    , pathLength(-1)
    , goalPathSearch(goal_path_search)
    , optimalSearch(optimal_search) {
    PLAJA_ASSERT(config.has_sharable_ptr(PLAJA::SharableKey::STATS) and config.get_sharable_ptr<PLAJA::StatsBase>(PLAJA::SharableKey::STATS) == searchStatistics.get());

    SEARCH_PA_BASE::add_stats(*searchStatistics);

    if (config.has_sharable(PLAJA::SharableKey::MODEL_Z3) and PLAJA_UTILS::is_derived_ptr_type<ModelZ3PA>(config.get_sharable<ModelZ3>(PLAJA::SharableKey::MODEL_Z3).get())) {
        auto model_z3 = config.get_sharable<ModelZ3>(PLAJA::SharableKey::MODEL_Z3);
        modelZ3 = std::static_pointer_cast<ModelZ3PA>(model_z3);
    } else {
        if (propertyInfo->get_nn_interface()) { MODEL_MARABOU_PA::make_sharable(config); } // TODO as long as ModelZ3 loads ModelMarabou for NN encoding
        #ifdef USE_VERITAS
        if (propertyInfo->get_ensemble_interface()) { MODEL_VERITAS_PA::make_sharable(config); }
        #endif
        modelZ3 = std::make_shared<ModelZ3PA>(config);
        config.set_sharable<ModelZ3>(PLAJA::SharableKey::MODEL_Z3, modelZ3);
    }

    // search space:
    searchSpace = std::make_unique<SearchSpacePABase>(config, *modelZ3); // construct search space (note: model Z3 needs predicates to be set and state info to be computed)

    // frontier:
    construct_heuristic(config);

    // set stats properly:
    searchStatistics->set_attr_unsigned(PLAJA::StatsUnsigned::PREDICATES, modelZ3->get_number_predicates());
    searchStatistics->set_attr_int(PLAJA::StatsInt::PATH_LENGTH, pathLength);

    expansionPa = std::make_unique<ExpansionPABase>(config, goalPathSearch, optimalSearch, *searchSpace, *modelZ3, witness_interval);

}

SearchPABase::~SearchPABase() = default;

/**********************************************************************************************************************/

void SearchPABase::construct_heuristic(const SearchEngineConfigPA& config) {
#ifdef ENABLE_HEURISTIC_SEARCH_PA
    config.set_sharable_ptr<PLAJA::StatsBase>(PLAJA::SharableKey::STATS_HEURISTIC, searchStatistics.get()); // set stats for heuristic
    frontier = HeuristicStateQueue::make_state_queue(config);
#else
    frontier = std::make_unique<FIFOStateQueue>();
#endif
}

/**********************************************************************************************************************/

void SearchPABase::increment() {
    searchStatistics->reset(); // reset stats
    modelZ3->increment(); // increment model *first*
    //
    // reset solver and successor generation:
    // solver.clear();
    // solver.add_unregistered_variables(&modelZ3->get_pa_expression());
    // successorGenerator->increment(solver);
    // solver.unregister_all_vars();
    //
    searchSpace->increment();
    //
    frontier->clear();
    frontier->increment();
    //
    // if (predicateRelations) { predicateRelations->increment(); }
    //
    // solution checker (if re-constructing succ gen)
    // solutionChecker = std::make_unique<SolutionChecker>(*model, modelZ3->get_pa_expression(), jani2NNet, &successorGenerator->get_concrete());
    //
    // if (SatChecker) { SatChecker->increment(); }
    //     SatChecker = sat_checker::construct_checker(*jani2NNet, generate_information());
    //     SatChecker->set_statistics(&searchStatistics->share_nn_sat_stats());
    // }
    expansionPa->increment();

    if (cachedStartStates) { cachedStartStates->clear(); }
    pathLength = -1;

    // reset stats properly:
    searchStatistics->set_attr_unsigned(PLAJA::StatsUnsigned::PREDICATES, modelZ3->get_number_predicates());
    searchStatistics->set_attr_int(PLAJA::StatsInt::PATH_LENGTH, pathLength);
}

bool SearchPABase::add_start_state() {
    if (not cachedStartStates) { return false; }

    while (not cachedStartStates->empty()) {

        /* Pop start state. */
        const StateID_type start_id = cachedStartStates->front();
        cachedStartStates->pop_front();

        /* Get search node of start state. */
        auto start_node = searchSpace->get_node(start_id);

        if (not start_node.is_reached()) { // Not already reached (and expanded).

            /* Set reached. */
            start_node.set_reached_init();
            searchStatistics->inc_attr_unsigned(PLAJA::StatsUnsigned::REACHABLE_STATES);

            /* Add to frontier. */
            frontier->push(start_node);

            // stateSpace->add_state_information(start_id); // Maintain state space.

            if (expansionPa->check_reach(*searchSpace->get_abstract_state(start_id))) {
                PLAJA_LOG("Start state is a goal!")
                start_node().set_goal();
                searchStatistics->inc_attr_unsigned(PLAJA::StatsUnsigned::GOAL_STATES);
                searchSpace->add_goal_path_frontier(start_id);
            }

            return true;

        } else { // Still print goal state info.
            if (searchSpace->has_goal_path(start_id)) { PLAJA_LOG("Start state is a goal!") }
        }
    }

    return false;
}

SearchEngine::SearchStatus SearchPABase::initialize() {
    PLAJA_LOG("Initializing PA engine ...")

    searchSpace->compute_initial_states();

    /* Initialize start states. */
    for (const StateID_type start_state_id: searchSpace->_initialStates()) {

        /* Set start state & node. */
        auto start_node = searchSpace->add_node(searchSpace->get_abstract_state(start_state_id));
        PLAJA_ASSERT(not start_node.is_reached())
        searchStatistics->inc_attr_unsigned(PLAJA::StatsUnsigned::START_STATES);
        searchStatistics->inc_attr_unsigned(PLAJA::StatsUnsigned::GENERATED_STATES);

        /* BFS-PER-STATE: cache start states. */
        PLAJA_ASSERT(not cachedStartStates or (not goalPathSearch and not optimalSearch))
        if (cachedStartStates) {
            cachedStartStates->push_back(start_state_id);
            continue;
        }

        /* Set reached and add to frontier. */
        start_node.set_reached_init();
        searchStatistics->inc_attr_unsigned(PLAJA::StatsUnsigned::REACHABLE_STATES);
        frontier->push(start_node);

        // if constexpr(PA_COMPUTATION_AUX::is_pa<SearchPA_t>()) { derived_engine->stateSpace->add_state_information(start_state_id); } // Maintain state space.

        /* Goal check. */
        if (expansionPa->check_reach(start_node.get_state())) {
            PLAJA_LOG("Start state is a goal!!")
            start_node().set_goal();
            searchStatistics->inc_attr_unsigned(PLAJA::StatsUnsigned::GOAL_STATES);
            searchSpace->add_goal_path_frontier(start_state_id);

            if (goalPathSearch) { return SearchEngine::SOLVED; }
        }
    }

    /* BFS-PER-STATE: add first start state. */
    add_start_state();

    return SearchEngine::IN_PROGRESS;
}

SearchEngine::SearchStatus SearchPABase::step() {

    if (frontier->empty() and not add_start_state()) {
        PLAJA_LOG_IF(searchSpace->goal_path_frontier_empty(), "Goal unreachable!")
        return SearchEngine::SOLVED;
    }

    const auto newly_reached = expansionPa->step(frontier->pop());

    if (goalPathSearch and not optimalSearch and not searchSpace->goal_path_frontier_empty()) { return SearchStatus::SOLVED; }

    /* Update frontier. */
    for (const StateID_type state_id: newly_reached) { frontier->push(searchSpace->get_node(state_id)); }

    return SearchStatus::IN_PROGRESS;

}

/**********************************************************************************************************************/

bool SearchPABase::has_goal_path() const {
    PLAJA_ASSERT(this->get_status() == SearchStatus::FINISHED)
    return not searchSpace->goal_path_frontier_empty() or pathLength != -1; // Check if path exists.
}

AbstractPath SearchPABase::extract_goal_path() const {
    PLAJA_ASSERT(not searchSpace->goal_path_frontier_empty())
    auto path = searchSpace->extract_goal_path(searchSpace->goal_frontier_state());
    pathLength = PLAJA_UTILS::cast_numeric<int>(path.get_size());
    searchStatistics->set_attr_int(PLAJA::StatsInt::PATH_LENGTH, pathLength);
    return path;
}

STMT_IF_DEBUG(SatChecker* SearchPABase::get_sat_checker() { return expansionPa->get_sat_checker(); })

/** statistics ********************************************************************************************************/

void SearchPABase::update_statistics() const { searchSpace->update_statistics(); }

void SearchPABase::print_statistics() const {
    searchStatistics->print_statistics();
    searchSpace->print_statistics();
    // additional stats:
    expansionPa->print_stats();
    frontier->print_statistics();
}

void SearchPABase::stats_to_csv(std::ofstream& file) const {
    searchStatistics->stats_to_csv(file);
    searchSpace->stats_to_csv(file);
    frontier->stats_to_csv(file);
}

void SearchPABase::stat_names_to_csv(std::ofstream& file) const {
    searchStatistics->stat_names_to_csv(file);
    SearchSpacePABase::stat_names_to_csv(file);
    frontier->stat_names_to_csv(file);
}

#pragma GCC diagnostic pop