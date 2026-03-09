//
// This file is part of the PlaJA code base (2019 - 2022).
//

#include "options_ensemble.h"
#include "../../../include/marabou_include/options.h"
#include "../../../include/marabou_config_const.h"
#include "../../../option_parser/option_parser.h"
#include "../using_veritas.h"

namespace PLAJA_OPTION {

    extern const std::string marabou_options;
    extern const std::string marabou_solver;

    extern const std::string marabouPreOptDis;

    extern const std::string marabouSupportEqDis;
    extern const std::string marabouOptAuxForDis;
    extern const std::string marabouOnlyFeasibleDis;
    extern const std::string marabouAdvEntailOptsDis;
    extern const std::string marabouTighteningAuxInDis;
    extern const std::string marabouSoiForDis;
    extern const std::string marabouEncodeExactDis;
}

namespace OPTIONS_MARABOU {

    const std::unordered_map<std::string, SolverType> stringToSolverType { // NOLINT(cert-err58-cpp)
        { "reluplex", OPTIONS_MARABOU::SolverType::RELUPLEX },
        { "soi",      OPTIONS_MARABOU::SolverType::SOI },
        { "milp",     OPTIONS_MARABOU::SolverType::MILP },
    };

    bool dncMode; // NOLINT(*-avoid-non-const-global-variables)
    int marabouTimeout; // NOLINT(*-avoid-non-const-global-variables)

    int get_dnc_mode() { return dncMode; }

    int get_timeout() { return marabouTimeout; }

}

void OPTIONS_MARABOU::parse_marabou_options() {
    static bool done = false;
    if (done) { return; } // parsing already happened once ...
    else { PLAJA_LOG("Parsing options for Marabou ..."); }

    std::string options_str = "marabou "; // dummy program call

    if (PLAJA_GLOBAL::optionParser->has_value_option(PLAJA_OPTION::marabou_options)) {
        options_str.append(PLAJA_GLOBAL::optionParser->get_value_option_string(PLAJA_OPTION::marabou_options));
    }

    if (not PLAJA_UTILS::contains(options_str, "verbosity")) { options_str.append(" --verbosity 0"); }

    const auto marabou_solver = PLAJA_GLOBAL::optionParser->get_option(stringToSolverType, PLAJA_OPTION::marabou_solver);

    if constexpr (PLAJA_GLOBAL::supportGurobi) {
        if (marabou_solver != MILP) {
            if (not PLAJA_UTILS::contains(options_str, "lp-solver")) { options_str.append(" --lp-solver gurobi"); }
        }
    }

    switch (marabou_solver) {
        case RELUPLEX: {
            GlobalConfiguration::USE_DEEPSOI_LOCAL_SEARCH = false;
            break;
        }
        case SOI: {
            GlobalConfiguration::USE_DEEPSOI_LOCAL_SEARCH = true;
            break;
        }
        case MILP: {
            options_str.append(" --milp");
            break;
        }
        default: PLAJA_ABORT
    }

    if constexpr (PLAJA_GLOBAL::marabouDisBaselineSupport) {
        GlobalConfiguration::PRE_OPTIMIZE_DISJUNCTIONS = PLAJA_GLOBAL::get_bool_option(PLAJA_OPTION::marabouPreOptDis);
        GlobalConfiguration::SUPPORT_DISJUNCTIONS_WITH_EQUATIONS = PLAJA_GLOBAL::get_bool_option(PLAJA_OPTION::marabouSupportEqDis);
        GlobalConfiguration::OPTIMIZE_AUX_VARS_FOR_DIS = PLAJA_GLOBAL::get_bool_option(PLAJA_OPTION::marabouOptAuxForDis);
        GlobalConfiguration::OPTIMIZE_AUX_VARS_FOR_EQUALITY_EQUATIONS_IN_DIS = GlobalConfiguration::OPTIMIZE_AUX_VARS_FOR_DIS;
        GlobalConfiguration::ENCODE_ONLY_FEASIBLE_DISJUNCTS = PLAJA_GLOBAL::get_bool_option(PLAJA_OPTION::marabouOnlyFeasibleDis);
        GlobalConfiguration::ADVANCED_ENTAILMENT_OPTS_FOR_DIS = PLAJA_GLOBAL::get_bool_option(PLAJA_OPTION::marabouAdvEntailOptsDis);
        GlobalConfiguration::SUPPORT_BOUND_TIGHTENING_FOR_AUX_VARS_IN_DIS = PLAJA_GLOBAL::get_bool_option(PLAJA_OPTION::marabouTighteningAuxInDis);
        GlobalConfiguration::SUPPORT_SOI_FOR_DISJUNCTIONS = PLAJA_GLOBAL::get_bool_option(PLAJA_OPTION::marabouSoiForDis);
        GlobalConfiguration::EXACT_DISJUNCTION_IN_GUROBI = PLAJA_GLOBAL::get_bool_option(PLAJA_OPTION::marabouEncodeExactDis);
    }

#if 0
    /*
     * MV: This does not work if preprocessorBoundTolerance is smallter than precision of std::string ...
     * Better to just use Marabou's internal default value.
     */
    if (not PLAJA_UTILS::contains(options_str, "preprocessor-bound-tolerance")) {
        options_str.append(" --preprocessor-bound-tolerance "); // for incremental solving
        options_str.append(std::to_string(MARABOU_IN_PLAJA::preprocessorBoundTolerance));
    }
#endif

    // replace "~" by "-" (not required)
    // PLAJA_UTILS::replace_all(options_str, "~", PLAJA_UTILS::dashString);

    // Split at whitespace
    const std::vector<std::string> arguments_strs = PLAJA_UTILS::split(options_str, PLAJA_UTILS::spaceString);

    // arguments to char*
    char** args = PLAJA_UTILS::strings_to_char_array(arguments_strs);

    Options* options = Options::get();
    PLAJA_ASSERT(arguments_strs.size() <= std::numeric_limits<int>::max())
    options->parseOptions(arguments_strs.size(), args); // NOLINT(cppcoreguidelines-narrowing-conversions)
    OPTIONS_MARABOU::dncMode = options->getBool(Options::DNC_MODE);
    OPTIONS_MARABOU::marabouTimeout = options->getInt(Options::TIMEOUT);

    // delete
    PLAJA_UTILS::delete_char_array(arguments_strs.size(), args); // NOLINT(cppcoreguidelines-narrowing-conversions)

    done = true; // do not repeat parsing !!!
}