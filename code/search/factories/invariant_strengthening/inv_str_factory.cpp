//
// This file is part of the PlaJA code base.
// Copyright (c) (2019 - 2021) Marcel Vinzent.
// See README.md in the top-level directory for licensing information.
//

#include "inv_str_factory.h"
#include "../../../option_parser/option_parser_aux.h"
#include "../../invariant_strengthening/invariant_strengthening.h"
#include "../../information/property_information.h"
#include "../configuration.h"
#include "inv_str_options.h"

InvStrFactory::InvStrFactory():
    SMTBaseFactory(SearchEngineFactory::InvariantStrengtheningType) {}

InvStrFactory::~InvStrFactory() = default;

void InvStrFactory::add_options(PLAJA::OptionParser& option_parser) const {
    SMTBaseFactory::add_options(option_parser);
    //
    OPTION_PARSER::add_flag(option_parser, PLAJA_OPTION::per_action_query);
}

void InvStrFactory::print_options() const {
    OPTION_PARSER::print_options_headline("Invariant Strengthening");
    OPTION_PARSER::print_flag(PLAJA_OPTION::per_action_query, "Check invariance test via SMT queries per action label.");
    SMTBaseFactory::print_options_base();
    OPTION_PARSER::print_options_footline();
}

std::unique_ptr<SearchEngine> InvStrFactory::construct(const PLAJA::Configuration& config) const { return std::make_unique<InvariantStrengthening>(config); }

bool InvStrFactory::supports_property_sub(const PropertyInformation& prop_info) const { return prop_info.get_reach(); }
