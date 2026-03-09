//
// This file is part of the PlaJA code base.
// Copyright (c) (2019 - 2021) Marcel Vinzent.
// See README.md in the top-level directory for licensing information.
//

#include "bmc_factory.h"
#include "../../../include/factory_config_const.h"
#include "../../../option_parser/enum_option_values_set.h"
#include "../../../option_parser/option_parser_aux.h"
#include "../../../option_parser/plaja_options.h"
#include "../../information/property_information.h"
#include "../configuration.h"
#include "bmc_options.h"

#ifdef USE_Z3

#include "../../bmc/bmc_z3.h"

#endif

#ifdef USE_MARABOU

#include "../../bmc/bmc_marabou.h"

#endif

namespace PLAJA_OPTION {

    STMT_IF_DEBUG(extern std::unique_ptr<PLAJA_OPTION::EnumOptionValuesSet> construct_bmc_solver_enum();)

    std::unique_ptr<PLAJA_OPTION::EnumOptionValuesSet> construct_bmc_solver_enum() {
        return std::make_unique<PLAJA_OPTION::EnumOptionValuesSet>(
            std::list<PLAJA_OPTION::EnumOptionValue> {
#ifdef USE_Z3
                PLAJA_OPTION::EnumOptionValue::Z3,
#endif
#ifdef USE_MARABOU
                PLAJA_OPTION::EnumOptionValue::Marabou,
#endif
            },
#ifdef USE_Z3
            PLAJA_OPTION::EnumOptionValue::Z3
#else
#ifdef USE_MARABOU
            PLAJA_OPTION::EnumOptionValue::Marabou
#endif
#endif
        );
    }

}

BMCFactory::BMCFactory():
    SMTBaseFactory(SearchEngineFactory::BmcType) {}

BMCFactory::~BMCFactory() = default;

void BMCFactory::add_options(PLAJA::OptionParser& option_parser) const {
    OPTION_PARSER::add_value_option(option_parser, PLAJA_OPTION::savePath);
    //
    SMTBaseFactory::add_options(option_parser);
    OPTION_PARSER::add_enum_option(option_parser, PLAJA_OPTION::bmc_mode, PLAJA_OPTION::construct_bmc_mode_enum());
    OPTION_PARSER::add_int_option(option_parser, PLAJA_OPTION::max_steps, PLAJA_OPTION_DEFAULT::max_steps);
    OPTION_PARSER::add_bool_option(option_parser, PLAJA_OPTION::find_shortest_path_only, PLAJA_OPTION_DEFAULT::find_shortest_path_only);
    OPTION_PARSER::add_flag(option_parser, PLAJA_OPTION::constrain_no_start);
    OPTION_PARSER::add_flag(option_parser, PLAJA_OPTION::constrain_loop_free);
    OPTION_PARSER::add_flag(option_parser, PLAJA_OPTION::constrain_no_reach);
    OPTION_PARSER::add_enum_option(option_parser, PLAJA_OPTION::bmc_solver, PLAJA_OPTION::construct_bmc_solver_enum());
}

void BMCFactory::print_options() const {
    OPTION_PARSER::print_options_headline("BMC");
    OPTION_PARSER::print_enum_option(PLAJA_OPTION::bmc_mode, *PLAJA_OPTION::construct_bmc_mode_enum(), "BMC mode to be used.");
    OPTION_PARSER::print_int_option(PLAJA_OPTION::max_steps, PLAJA_OPTION_DEFAULT::max_steps, "Maximal number of steps to be unrolled.");
    OPTION_PARSER::print_bool_option(PLAJA_OPTION::find_shortest_path_only, PLAJA_OPTION_DEFAULT::find_shortest_path_only, "Terminate after finding the shortest unsafe path.");
    OPTION_PARSER::print_flag(PLAJA_OPTION::constrain_no_start, "Constrain non-start states to be non-start.");
    OPTION_PARSER::print_flag(PLAJA_OPTION::constrain_loop_free, "Constrain (unsafe) path to be loop free.");
    OPTION_PARSER::print_flag(PLAJA_OPTION::constrain_no_reach, "Constrain non-terminal states to be non-reach.");
    OPTION_PARSER::print_enum_option(PLAJA_OPTION::bmc_solver, *PLAJA_OPTION::construct_bmc_solver_enum(), "SMT-solver used for BMC.");
    SMTBaseFactory::print_options_base();
    OPTION_PARSER::print_options_footline();
}

std::unique_ptr<SearchEngine> BMCFactory::construct(const PLAJA::Configuration& config) const {

    switch (config.get_enum_option(PLAJA_OPTION::bmc_solver).get_value()) {

#ifdef USE_Z3
        case PLAJA_OPTION::EnumOptionValue::Z3: { return std::make_unique<BMC_Z3>(config); }
#endif

#ifdef USE_MARABOU
        case PLAJA_OPTION::EnumOptionValue::Marabou: { return std::make_unique<BMCMarabou>(config); }
#endif

        default: PLAJA_ABORT

    }

    PLAJA_ABORT
}

bool BMCFactory::supports_property_sub(const PropertyInformation& prop_info) const { return prop_info.get_reach(); }
