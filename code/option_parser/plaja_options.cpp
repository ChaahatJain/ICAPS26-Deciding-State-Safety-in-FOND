//
// This file is part of the PlaJA code base.
// Copyright (c) (2019 - 2020) Marcel Vinzent.
// See README.md in the top-level directory for licensing information.
//

#include "plaja_options.h"
#include "enum_option_values_set.h"
#include "option_parser_aux.h"

namespace PLAJA_OPTION {
    const std::string help("help"); // NOLINT(cert-err58-cpp)
    // Engines
    const std::string engine("engine"); // NOLINT(cert-err58-cpp)
    const std::string max_time("max-time"); // NOLINT(cert-err58-cpp)
    const std::string print_time("print-time"); // NOLINT(cert-err58-cpp)
    // Properties
    const std::string model_file("model-file"); // NOLINT(cert-err58-cpp)
    const std::string prop("prop"); // NOLINT(cert-err58-cpp)
    const std::string additional_properties("additional-properties"); // NOLINT(cert-err58-cpp)
    const std::string allow_locals("allow-locals"); // NOLINT(cert-err58-cpp)
    const std::string nn_interface("nn-interface"); // NOLINT(cert-err58-cpp)
    const std::string nn_for_racetrack("nn-for-racetrack"); // NOLINT(cert-err58-cpp)
    const std::string nn("nn"); // NOLINT(cert-err58-cpp)
    const std::string torch_model("torch-model"); // NOLINT(cert-err58-cpp)
    const std::string applicability_filtering("applicability-filtering"); // NOLINT(cert-err58-cpp)
    const std::string ignore_nn("ignore-nn"); // NOLINT(cert-err58-cpp)

    // Ensemble
    const std::string ensemble("ensemble"); // NOLINT(cert-err58-cpp)
    const std::string ensemble_interface("ensemble-interface"); // NOLINT(cert-err58-cpp)
    const std::string app_filter_ensemble("app-filter-ensemble"); // NOLINT(cert-err58-cpp)

    // Constants
    extern const std::string consts("consts"); // NOLINT(cert-err58-cpp)
    extern const std::string const_vars_to_consts("const-vars-to-consts"); // NOLINT(cert-err58-cpp)
    // Statistics
    const std::string print_stats("print-stats"); // NOLINT(cert-err58-cpp)
    const std::string stats_file("stats-file"); // NOLINT(cert-err58-cpp)
    const std::string save_intermediate_stats("save-intermediate-stats"); // NOLINT(cert-err58-cpp)
    const std::string savePath("save-path"); // NOLINT(cert-err58-cpp)
    const std::string saveStateSpace("output-state-space"); // NOLINT(cert-err58-cpp)
    // Parsing
    const std::string ignore_unexpected_json("ignore-unexpected-json"); // NOLINT(cert-err58-cpp)
    const std::string pa_objective("pa-objective"); // NOLINT(cert-err58-cpp)
    const std::string set_pa_goal_objective_terminal("set-pa-goal-objective-terminal"); // NOLINT(cert-err58-cpp)
    // Search
    const std::string initial_state_enum("initial-state-enum"); // NOLINT(cert-err58-cpp)
    const std::string additional_initial_states("additional-initial-states"); // NOLINT(cert-err58-cpp)
    const std::string search_per_start("search-per-start"); // NOLINT(cert-err58-cpp)
    // Misc
    const std::string seed("seed"); // NOLINT(cert-err58-cpp)
    const std::string local_backup("local-backup"); // NOLINT(cert-err58-cpp)
}

/**********************************************************************************************************************/

namespace PLAJA_OPTION {

    namespace SOLUTION_ENUMERATION { extern std::unique_ptr<PLAJA_OPTION::EnumOptionValuesSet> construct_default_configs_enum(); }

    void add_initial_state_enum(PLAJA::OptionParser& option_parser) {
        OPTION_PARSER::add_enum_option(option_parser, PLAJA_OPTION::initial_state_enum, SOLUTION_ENUMERATION::construct_default_configs_enum());
        OPTION_PARSER::add_value_option(option_parser, PLAJA_OPTION::additional_initial_states);
    }

    void print_initial_state_enum() {
        OPTION_PARSER::print_enum_option(PLAJA_OPTION::initial_state_enum, *PLAJA_OPTION::SOLUTION_ENUMERATION::construct_default_configs_enum(), "Type for initial state enumeration.");
        OPTION_PARSER::print_option(PLAJA_OPTION::additional_initial_states, "<index>|<start_index>:<end_index>, ..., <index>|<start_index>:<end_index>", "Restrict additional initial states to a subset.");
    }

    void print_search_per_start() {
        OPTION_PARSER::print_bool_option(PLAJA_OPTION::search_per_start, PLAJA_OPTION_DEFAULT::search_per_start, "Run search per start state -- e.g., used to reduce fluctuation in PA runtime when scaling start state sets.");
    }

}
