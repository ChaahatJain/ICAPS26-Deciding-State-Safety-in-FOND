//
// This file is part of the PlaJA code base.
// Copyright (c) (2019 - 2022) Marcel Vinzent.
// See README.md in the top-level directory for licensing information.
//

#ifndef PLAJA_TO_NUXMV_OPTIONS_H
#define PLAJA_TO_NUXMV_OPTIONS_H

#include <string>

namespace PLAJA_OPTION {

    extern const std::string nuxmv_model;
    extern const std::string nuxmv_use_ex_model;

    extern const std::string nuxmv_config_file;

    extern const std::string nuxmv_config;

    extern const std::string nuxmv_default_engine;
    extern const std::string nuxmv_engine_config;
    extern const std::string nuxmv_bmc_len;

    extern const std::string nuxmv_ex;

    extern const std::string nuxmv_use_case; // Do this
    extern const std::string nuxmv_invar_for_reals;
    extern const std::string nuxmv_def_guards;
    extern const std::string nuxmv_def_updates;
    extern const std::string nuxmv_def_reach;
    extern const std::string nuxmv_use_guard_per_update;
    extern const std::string nuxmv_merge_updates;

    extern const std::string nuxmv_backwards_def_order;
    extern const std::string nuxmv_backwards_bool_vars;
    extern const std::string nuxmv_backwards_copy_updates;
    extern const std::string nuxmv_backwards_op_exclusion;
    extern const std::string nuxmv_backwards_def_goal;

    extern const std::string nuxmv_backwards_nn_precision;
    extern const std::string nuxmv_backwards_policy_selection;
    extern const std::string nuxmv_backwards_policy_module;
    extern const std::string nuxmv_backwards_policy_module_str;

    extern const std::string nuxmv_backwards_suppress_ast_special;
    extern const std::string nuxmv_backwards_special_trivial_bounds;

}

namespace PLAJA_OPTION_DEFAULT {

    extern const std::string nuxmv_model;
    constexpr bool nuxmv_use_ex_model(true);

    constexpr int nuxmv_bmc_len(1000);

    constexpr bool nuxmv_use_case(true);
    constexpr bool nuxmv_invar_for_reals(false); // If turtlebot, then try enabling/disabling
    constexpr bool nuxmv_def_guards(false); // syntactic sugar
    constexpr bool nuxmv_def_updates(false); // syntactic sugar
    constexpr bool nuxmv_def_reach(false);
    constexpr bool nuxmv_use_guard_per_update(false);
    constexpr bool nuxmv_merge_updates(false); 

    constexpr bool nuxmv_backwards_flag(false);

    constexpr bool nuxmv_backwards_def_order(false);
    constexpr bool nuxmv_backwards_bool_vars(false);
    constexpr bool nuxmv_backwards_copy_updates(false);
    constexpr bool nuxmv_backwards_op_exclusion(false);
    constexpr bool nuxmv_backwards_def_goal(false);

    constexpr bool nuxmv_backwards_nn_precision(false);
    constexpr bool nuxmv_backwards_policy_selection(false);
    constexpr bool nuxmv_backwards_policy_module(false);
    constexpr bool nuxmv_backwards_policy_module_str(false);

    constexpr bool nuxmv_backwards_suppress_ast_special(false);
    constexpr bool nuxmv_backwards_special_trivial_bounds(false);

}

#endif //PLAJA_TO_NUXMV_OPTIONS_H
