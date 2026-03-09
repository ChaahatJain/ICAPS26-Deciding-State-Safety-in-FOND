//
// This file is part of the PlaJA code base.
// Copyright (c) (2019 - 2020) Marcel Vinzent.
// See README.md in the top-level directory for licensing information.
//

#ifndef PLAJA_PLAJA_OPTIONS_H
#define PLAJA_PLAJA_OPTIONS_H

#include <string>
#include "forward_option_parser.h"

namespace PLAJA_OPTION {

    extern const std::string help;

    // Engines
    extern const std::string engine;
    extern const std::string max_time;
    extern const std::string print_time;

    // Model & Properties
    extern const std::string model_file;
    extern const std::string prop;
    extern const std::string additional_properties;
    extern const std::string allow_locals;
    extern const std::string nn_interface;
    extern const std::string nn_for_racetrack;
    extern const std::string nn;
    extern const std::string torch_model;
    extern const std::string applicability_filtering;
    extern const std::string ignore_nn;

    // Ensemble model
    extern const std::string ensemble;
    extern const std::string ensemble_interface;
    extern const std::string app_filter_ensemble;

    // Constants
    extern const std::string consts;
    extern const std::string const_vars_to_consts;

    // Statistics
    extern const std::string print_stats;
    extern const std::string stats_file;
    extern const std::string save_intermediate_stats;
    extern const std::string savePath;
    extern const std::string saveStateSpace;

    // Parsing
    extern const std::string ignore_unexpected_json;
    extern const std::string pa_objective;
    extern const std::string set_pa_goal_objective_terminal;

    // Search
    extern const std::string initial_state_enum;
    extern const std::string additional_initial_states;
    extern const std::string search_per_start;

    // Misc
    extern const std::string seed;
    extern const std::string local_backup;

}

namespace PLAJA_OPTION_DEFAULT {

    constexpr unsigned int max_time = 1800;

    constexpr int applicability_filtering = 0;

    constexpr bool const_vars_to_consts(false); // Only apply on-demand since usually variables would not be constant ...

    constexpr bool set_pa_goal_objective_terminal(false);

    constexpr bool search_per_start(false);

}

namespace PLAJA_OPTION {

    extern void add_initial_state_enum(PLAJA::OptionParser& option_parser);
    extern void print_initial_state_enum();

    extern void print_search_per_start();

}

#endif //PLAJA_PLAJA_OPTIONS_H
