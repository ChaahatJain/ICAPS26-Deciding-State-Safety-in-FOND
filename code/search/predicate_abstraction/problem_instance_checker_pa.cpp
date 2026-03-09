//
// This file is part of the PlaJA code base.
// Copyright (c) (2019 - 2023) Marcel Vinzent.
// See README.md in the top-level directory for licensing information.
//

#include "problem_instance_checker_pa.h"
#include "../../utils/utils.h"

ProblemInstanceCheckerPa::ProblemInstanceCheckerPa(const SearchEngineConfigPA& config):
    SearchPABase(config, false, false) {
}

ProblemInstanceCheckerPa::~ProblemInstanceCheckerPa() = default;

SearchEngine::SearchStatus ProblemInstanceCheckerPa::initialize() { return SearchStatus::IN_PROGRESS; }

SearchEngine::SearchStatus ProblemInstanceCheckerPa::step() {
    PLAJA_LOG(PLAJA_UTILS::to_red_string("Warning: This engine is not implemented."))
    return SearchStatus::SOLVED;
}

HeuristicValue_type ProblemInstanceCheckerPa::get_goal_distance(const AbstractState& /*pa_state*/) { PLAJA_ABORT }