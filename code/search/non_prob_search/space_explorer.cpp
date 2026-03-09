//
// This file is part of the PlaJA code base.
// Copyright (c) (2019 - 2020) Marcel Vinzent.
// See README.md in the top-level directory for licensing information.
//

#include "space_explorer.h"
#include "../../option_parser/plaja_options.h"
#include "../../stats/stats_base.h"
#include "../factories/non_prob_search/non_prob_search_options.h"
#include "../factories/configuration.h"
#include "../successor_generation/successor_generator.h"
#include "../prob_search/state_space.h"
#include "base/search_space_non_prob.h"

SpaceExplorer::SpaceExplorer(const PLAJA::Configuration& config):
    SearchEngineNonProb(config, false, true, true)
    , searchSpace(new SearchSpaceNonProb(*model))
    , computeGoalPaths(config.get_bool_option(PLAJA_OPTION::computeGoalPaths)) {

    SearchEngineNonProb::set_search_space(searchSpace.get());
}

SpaceExplorer::~SpaceExplorer() = default;

/* */

SearchEngine::SearchStatus SpaceExplorer::finalize() {
    if (computeGoalPaths) { compute_goal_path_states(); }
    return FINISHED;
}

/* Override stats. */

void SpaceExplorer::print_statistics() const {
    SearchEngineNonProb::print_statistics();
    if (PLAJA_GLOBAL::has_value_option(PLAJA_OPTION::saveStateSpace)) { get_state_space()->dump_state_space(); }
}
