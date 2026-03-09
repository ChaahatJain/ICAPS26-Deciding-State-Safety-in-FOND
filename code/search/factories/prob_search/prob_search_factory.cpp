//
// This file is part of the PlaJA code base.
// Copyright (c) (2019 - 2022) Marcel Vinzent.
// See README.md in the top-level directory for licensing information.
//

#include "prob_search_factory.h"
#include "prob_search_options.h"
#include "../../../option_parser/option_parser_aux.h"
#include "../../../option_parser/plaja_options.h"
#include "../../non_prob_search/restrictions_checker_explicit.h"

ProbSearchFactory::ProbSearchFactory(SearchEngineType engine_type):
    SearchEngineFactory(engine_type) {}

ProbSearchFactory::~ProbSearchFactory() = default;

void ProbSearchFactory::supports_model(const Model& model) const {
    RestrictionsCheckerExplicit::check_restrictions(&model, false);
}

void ProbSearchFactory::add_options(PLAJA::OptionParser& option_parser) const {
    OPTION_PARSER::add_value_option(option_parser, PLAJA_OPTION::saveStateSpace);
    //
    SearchEngineFactory::add_options(option_parser);
    //
    OPTION_PARSER::add_flag(option_parser, PLAJA_OPTION::upper_and_lower_bound);
    OPTION_PARSER::add_value_option(option_parser, PLAJA_OPTION::epsilon);
    OPTION_PARSER::add_value_option(option_parser, PLAJA_OPTION::delta);
    OPTION_PARSER::add_value_option(option_parser, PLAJA_OPTION::local_trap_elimination);
    OPTION_PARSER::add_value_option(option_parser, PLAJA_OPTION::target_value);
    OPTION_PARSER::add_flag(option_parser, PLAJA_OPTION::output_policy);
}

void ProbSearchFactory::print_options() const {
    OPTION_PARSER::print_options_headline("Options for probabilistic engines");

    OPTION_PARSER::print_flag(PLAJA_OPTION::upper_and_lower_bound, "Request engine to maintain an upper as well as a lower bound.");

    OPTION_PARSER::print_option(PLAJA_OPTION::epsilon, OPTION_PARSER::valueStr, "Consistency criterion for LRTDP and FRET (by default 0.000001)");

    OPTION_PARSER::print_option(PLAJA_OPTION::delta, OPTION_PARSER::valueStr, "Precision criterion for LRTDP and FRET (by default 0.000001)", true);
    OPTION_PARSER::print_additional_specification("For picking optimal greedy action operators, e.g., for FRET's \"all-greedy-policies\" graph (see below) or for tie-breaking (currently not supported)", true);
    OPTION_PARSER::print_additional_specification("For LRTDP, if upper and lower bound, the precision to apply early-termination (see below).");

    OPTION_PARSER::print_option(PLAJA_OPTION::local_trap_elimination, OPTION_PARSER::valueStr, "Whether FRET's trap elimination only considers the current policy graph or all policy graphs greedy w.r.t. to current state values.", true);
    OPTION_PARSER::print_additional_specification("By default: 1 (or true)");

    OPTION_PARSER::print_option(PLAJA_OPTION::target_value, OPTION_PARSER::valueStr, "For LRTDP the target value of the property.", true);
    OPTION_PARSER::print_additional_specification("Depending on the property treated as an upper or lower bound which allows early-termination when violated.");

    OPTION_PARSER::print_option(PLAJA_OPTION::output_policy, OPTION_PARSER::fileStr, "Output computed policy.");

    OPTION_PARSER::print_options_footline();
}