//
// This file is part of the PlaJA code base.
// Copyright (c) (2019 - 2020) Marcel Vinzent.
// See README.md in the top-level directory for licensing information.
//

#include <memory>
#include "predicate_abstraction.h"
#include "../../option_parser/option_parser.h"
#include "../../option_parser/plaja_options.h"
#include "../../stats/stats_base.h"
#include "pa_states/abstract_path.h"
#include "search_space/search_space_pa_base.h"

PredicateAbstraction::PredicateAbstraction(const SearchEngineConfigPA& config):
    SearchPABase(config, false, false) {
}

PredicateAbstraction::~PredicateAbstraction() = default;

/**********************************************************************************************************************/

SearchEngine::SearchStatus PredicateAbstraction::finalize() {

    if constexpr (PLAJA_GLOBAL::paOnlyReachability) { return SearchStatus::FINISHED; } // when only computing reachability, not all goal paths are preserved

    PLAJA_LOG("Computing abstract states with a goal path ...")
    searchStatistics->inc_attr_unsigned(PLAJA::StatsUnsigned::DEAD_END_STATES, static_cast<int>(searchStatistics->get_attr_unsigned(PLAJA::StatsUnsigned::REACHABLE_STATES) - searchSpace->compute_goal_path_states()));
    PLAJA_LOG(PLAJA_UTILS::string_f("... found %i", searchSpace->number_of_goal_path_states()))
    PLAJA_LOG(PLAJA_UTILS::string_f("... including %i out of %i abstract initial states.", searchSpace->number_of_initial_goal_path_states(), searchSpace->get_number_initial_states()))

    const auto primary_initial_state_id = searchSpace->_initialState(); // id of the primary abstract initial state, i.e., the one corresponding to the model's initial state (specified in the model itself).
    if (searchSpace->has_goal_path(primary_initial_state_id) and PLAJA_GLOBAL::has_value_option(PLAJA_OPTION::savePath)) {
        PLAJA_LOG("Found path from model's concrete initial state.")
        searchSpace->extract_goal_path(primary_initial_state_id).dump();
    }

    return SearchStatus::FINISHED;
}

HeuristicValue_type PredicateAbstraction::get_goal_distance(const AbstractState& pa_state) {
    auto node = searchSpace->add_node_info(searchSpace->set_abstract_state(pa_state)->get_id_value());
    PLAJA_ASSERT(node.known_goal_distance())
    return node.get_goal_distance();
}

/**********************************************************************************************************************/

void PredicateAbstraction::print_statistics() const {
    std::cout << "Goal path states: " << searchSpace->number_of_goal_path_states() << std::endl;
    std::cout << "Initial states with goal path: " << (SearchStatus::FINISHED == get_status() ? static_cast<int>(searchSpace->number_of_initial_goal_path_states()) : -1) << std::endl; // Warning: under PLAJA_GLOBAL::paOnlyReachability this is a lower bound
    SearchPABase::print_statistics();
}

void PredicateAbstraction::stats_to_csv(std::ofstream& file) const {
    file << PLAJA_UTILS::commaString << searchSpace->number_of_goal_path_states();
    file << PLAJA_UTILS::commaString << (SearchStatus::FINISHED == get_status() ? static_cast<int>(searchSpace->number_of_initial_goal_path_states()) : -1);
    SearchPABase::stats_to_csv(file);
}

void PredicateAbstraction::stat_names_to_csv(std::ofstream& file) const {
    file << PLAJA_UTILS::commaString << "goal_path_states";
    file << PLAJA_UTILS::commaString << "initial_states_value";
    SearchPABase::stat_names_to_csv(file);
}
