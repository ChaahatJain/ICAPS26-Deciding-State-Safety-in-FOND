//
// This file is part of the PlaJA code base.
// Copyright (c) (2019 - 2020) Marcel Vinzent.
// See README.md in the top-level directory for licensing information.
//

#include "prob_search_engine.h"
#include "../../option_parser/plaja_options.h"
#include "../../parser/ast/model.h"
#include "../factories/prob_search/prob_search_options.h"
#include "../factories/configuration.h"
#include "../fd_adaptions/search_statistics.h"
#include "../information/property_information.h"

ProbSearchEngineBase::ProbSearchEngineBase(const PLAJA::Configuration& config):
    SearchEngine(config)
    , searchSpaceBase(nullptr)
    , goal(propertyInfo->get_reach())
    , stateSpace(nullptr)
    , cost(propertyInfo->get_cost())
    , outputPolicyFlag(config.is_flag_set(PLAJA_OPTION::output_policy))
    , outputStateSpaceFlag(config.has_value_option(PLAJA_OPTION::saveStateSpace))
    , propertyFulfilled(false)
    , successorGeneratorOwn(new SuccessorGenerator(config, *model))
    , successorGenerator(*successorGeneratorOwn) {

    SEARCH_STATISTICS::add_basic_stats(*searchStatistics);
    searchStatistics->add_unsigned_stats(PLAJA::StatsUnsigned::DONT_CARE_STATES, "DontCareStates", 0);
    searchStatistics->add_unsigned_stats(PLAJA::StatsUnsigned::FIXED_CHOICES, "FixedChoices", 0);
}

ProbSearchEngineBase::ProbSearchEngineBase(const PLAJA::Configuration& config, const ProbSearchEngineBase& parent):
    SearchEngine(config)
    , searchSpaceBase(nullptr)
    , goal(propertyInfo->get_reach())
    , stateSpace(nullptr)
    , cost(propertyInfo->get_cost())
    , outputPolicyFlag(config.is_flag_set(PLAJA_OPTION::output_policy))
    , outputStateSpaceFlag(config.has_value_option(PLAJA_OPTION::saveStateSpace))
    , propertyFulfilled(false)
    , successorGeneratorOwn(nullptr)
    , successorGenerator(parent.successorGenerator) {
    PLAJA_ASSERT(searchStatistics.get() == parent.searchStatistics.get())
}

ProbSearchEngineBase::~ProbSearchEngineBase() = default;

/* Construction auxiliaries. */

void ProbSearchEngineBase::set_search_space(ProbSearchSpaceBase& search_space_base) {
    searchSpaceBase = &search_space_base;
    stateSpace = &search_space_base.get_underlying_state_space();
}

/* */

const ModelInformation& ProbSearchEngineBase::extract_model_info(const Model& model) { return model.get_model_information(); }

/* */

void ProbSearchEngineBase::set_subEngine(std::unique_ptr<ProbSearchEngineBase>&& /*sub_engine*/) {}

bool ProbSearchEngineBase::check_goal(const StateBase& state) const {

    if (PLAJA_GLOBAL::enableTerminalStateSupport and not PLAJA_GLOBAL::reachMayBeTerminal) {
        if (successorGenerator.get_terminal_state_condition() and successorGenerator.is_terminal(state)) { return false; }
    }

    return goal and goal->evaluate_integer(state);
}

bool ProbSearchEngineBase::check_early_termination() { return false; }

// override stats:

void ProbSearchEngineBase::print_statistics() const {
    // Property fulfilled ?
    std::cout << (SearchStatus::FINISHED == this->get_status() ? (this->propertyFulfilled ? "Property fulfilled: " : "Property violated: ") : "UNSOLVED: ") << searchSpaceBase->get_initial_value() << std::endl;
    searchStatistics->print_statistics();
    searchSpaceBase->print_statistics();
}

void ProbSearchEngineBase::stats_to_csv(std::ofstream& file) const {
    file << PLAJA_UTILS::commaString << (SearchStatus::FINISHED == this->get_status() ? this->propertyFulfilled : -1);
    file << PLAJA_UTILS::commaString << searchSpaceBase->get_initial_value();
    searchStatistics->stats_to_csv(file);
    searchSpaceBase->stats_to_csv(file);
}

void ProbSearchEngineBase::stat_names_to_csv(std::ofstream& file) const {
    file << ",property_fulfilled,initial_state_value";
    searchStatistics->stat_names_to_csv(file);
    ProbSearchSpaceBase::stat_names_to_csv(file);
}

