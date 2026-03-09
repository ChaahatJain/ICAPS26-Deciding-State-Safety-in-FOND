//
// This file is part of the PlaJA code base.
// Copyright (c) (2019 - 2021) Marcel Vinzent.
// See README.md in the top-level directory for licensing information.
//

#include "to_nuxmv_factory.h"
#include "../option_parser/enum_option_values_set.h"
#include "../option_parser/option_parser_aux.h"
#include "../search/information/property_information.h"
#include "to_nuxmv_options.h"
#include "to_nuxmv_restrictions_checker.h"
#include "to_nuxmv.h"

namespace TO_NUXMV { extern std::unique_ptr<PLAJA_OPTION::EnumOptionValuesSet> construct_default_configs_enum(); }

/* */

ToNuxmvFactory::ToNuxmvFactory():
    SearchEngineFactory(SearchEngineFactory::ToNuxmvType) {
}

ToNuxmvFactory::~ToNuxmvFactory() = default;

/* */

void ToNuxmvFactory::add_options(PLAJA::OptionParser& option_parser) const {
    OPTION_PARSER::add_option(option_parser, PLAJA_OPTION::nuxmv_model, PLAJA_OPTION_DEFAULT::nuxmv_model);
    OPTION_PARSER::add_bool_option(option_parser, PLAJA_OPTION::nuxmv_use_ex_model, PLAJA_OPTION_DEFAULT::nuxmv_use_ex_model);
    OPTION_PARSER::add_value_option(option_parser, PLAJA_OPTION::nuxmv_config_file);
    OPTION_PARSER::add_value_option(option_parser, PLAJA_OPTION::nuxmv_config);
    OPTION_PARSER::add_enum_option(option_parser, PLAJA_OPTION::nuxmv_default_engine, TO_NUXMV::construct_default_configs_enum());
    OPTION_PARSER::add_value_option(option_parser, PLAJA_OPTION::nuxmv_engine_config);
    OPTION_PARSER::add_int_option(option_parser, PLAJA_OPTION::nuxmv_bmc_len, PLAJA_OPTION_DEFAULT::nuxmv_bmc_len);
    OPTION_PARSER::add_value_option(option_parser, PLAJA_OPTION::nuxmv_ex);

    OPTION_PARSER::add_bool_option(option_parser, PLAJA_OPTION::nuxmv_use_case, PLAJA_OPTION_DEFAULT::nuxmv_use_case);
    OPTION_PARSER::add_bool_option(option_parser, PLAJA_OPTION::nuxmv_invar_for_reals, PLAJA_OPTION_DEFAULT::nuxmv_invar_for_reals);
    OPTION_PARSER::add_bool_option(option_parser, PLAJA_OPTION::nuxmv_def_guards, PLAJA_OPTION_DEFAULT::nuxmv_def_guards);
    OPTION_PARSER::add_bool_option(option_parser, PLAJA_OPTION::nuxmv_def_updates, PLAJA_OPTION_DEFAULT::nuxmv_def_updates);
    OPTION_PARSER::add_bool_option(option_parser, PLAJA_OPTION::nuxmv_def_reach, PLAJA_OPTION_DEFAULT::nuxmv_def_reach);
    OPTION_PARSER::add_bool_option(option_parser, PLAJA_OPTION::nuxmv_use_guard_per_update, PLAJA_OPTION_DEFAULT::nuxmv_use_guard_per_update);
    OPTION_PARSER::add_bool_option(option_parser, PLAJA_OPTION::nuxmv_merge_updates, PLAJA_OPTION_DEFAULT::nuxmv_merge_updates);

    OPTION_PARSER::add_bool_option(option_parser, PLAJA_OPTION::nuxmv_backwards_def_order, PLAJA_OPTION_DEFAULT::nuxmv_backwards_def_order);
    OPTION_PARSER::add_bool_option(option_parser, PLAJA_OPTION::nuxmv_backwards_bool_vars, PLAJA_OPTION_DEFAULT::nuxmv_backwards_bool_vars);
    OPTION_PARSER::add_bool_option(option_parser, PLAJA_OPTION::nuxmv_backwards_copy_updates, PLAJA_OPTION_DEFAULT::nuxmv_backwards_copy_updates);
    OPTION_PARSER::add_bool_option(option_parser, PLAJA_OPTION::nuxmv_backwards_op_exclusion, PLAJA_OPTION_DEFAULT::nuxmv_backwards_op_exclusion);
    OPTION_PARSER::add_bool_option(option_parser, PLAJA_OPTION::nuxmv_backwards_def_goal, PLAJA_OPTION_DEFAULT::nuxmv_backwards_def_goal);

    OPTION_PARSER::add_bool_option(option_parser, PLAJA_OPTION::nuxmv_backwards_nn_precision, PLAJA_OPTION_DEFAULT::nuxmv_backwards_nn_precision);
    OPTION_PARSER::add_bool_option(option_parser, PLAJA_OPTION::nuxmv_backwards_policy_selection, PLAJA_OPTION_DEFAULT::nuxmv_backwards_policy_selection);
    OPTION_PARSER::add_bool_option(option_parser, PLAJA_OPTION::nuxmv_backwards_policy_module, PLAJA_OPTION_DEFAULT::nuxmv_backwards_policy_module);
    OPTION_PARSER::add_bool_option(option_parser, PLAJA_OPTION::nuxmv_backwards_policy_module_str, PLAJA_OPTION_DEFAULT::nuxmv_backwards_policy_module_str);

    OPTION_PARSER::add_bool_option(option_parser, PLAJA_OPTION::nuxmv_backwards_suppress_ast_special, PLAJA_OPTION_DEFAULT::nuxmv_backwards_suppress_ast_special);
    OPTION_PARSER::add_bool_option(option_parser, PLAJA_OPTION::nuxmv_backwards_special_trivial_bounds, PLAJA_OPTION_DEFAULT::nuxmv_backwards_special_trivial_bounds);
}

void ToNuxmvFactory::print_options() const {
    OPTION_PARSER::print_options_headline("TO-NUXMV");
    OPTION_PARSER::print_option(PLAJA_OPTION::nuxmv_model, OPTION_PARSER::fileStr, PLAJA_OPTION_DEFAULT::nuxmv_model, "Target path for nuXmv model file.");
    OPTION_PARSER::print_bool_option(PLAJA_OPTION::nuxmv_use_ex_model, PLAJA_OPTION_DEFAULT::nuxmv_use_ex_model, "Do not generate nuxmv if present under path.");
    OPTION_PARSER::print_option(PLAJA_OPTION::nuxmv_config_file, OPTION_PARSER::fileStr, "Target path for nuXmv configuration file.");
    OPTION_PARSER::print_option(PLAJA_OPTION::nuxmv_config, OPTION_PARSER::valueStr, "Nuxmv configuration for config file (\"--\" indicates linebreak).");
    OPTION_PARSER::print_enum_option(PLAJA_OPTION::nuxmv_engine_config, *TO_NUXMV::construct_default_configs_enum(), "Default nuxmv configuration for config file indicated by keyword.");
    OPTION_PARSER::print_option(PLAJA_OPTION::nuxmv_engine_config, OPTION_PARSER::valueStr, "Additional inline config for default engine.");
    OPTION_PARSER::print_int_option(PLAJA_OPTION::nuxmv_bmc_len, PLAJA_OPTION_DEFAULT::nuxmv_bmc_len, "Value of nuXmv's bmc_length variable.");
    OPTION_PARSER::print_option(PLAJA_OPTION::nuxmv_ex, OPTION_PARSER::fileStr, "Path to nuXmv executable.");
    OPTION_PARSER::print_options_footline();
}

/* */

void ToNuxmvFactory::supports_model(const Model& model) const { TO_NUXMV_RESTRICTIONS_CHECKER::check(model); }

bool ToNuxmvFactory::supports_property_sub(const PropertyInformation& property_info) const { return property_info.get_reach(); }

/* */

std::unique_ptr<SearchEngine> ToNuxmvFactory::construct(const PLAJA::Configuration& config) const {
    return std::make_unique<ToNuxmv>(config);
}

namespace SEARCH_ENGINE_FACTORY {

    std::unique_ptr<SearchEngineFactory> construct_to_nuxmv_factory() { return std::make_unique<ToNuxmvFactory>(); }

}
