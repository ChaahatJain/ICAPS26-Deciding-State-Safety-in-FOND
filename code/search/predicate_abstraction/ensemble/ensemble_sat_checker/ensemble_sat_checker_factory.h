//
// This file is part of the PlaJA code base.
// Copyright (c) (2023 - 2024) Chaahat Jain.
//

#ifndef PLAJA_ENSEMBLE_SAT_CHECKER_FACTORY_H
#define PLAJA_ENSEMBLE_SAT_CHECKER_FACTORY_H

#include <memory>
#include "../../../factories/forward_factories.h"
#include "../../../information/jani2ensemble/forward_jani_2_ensemble.h"
#include "ensemble_sat_checker.h"

namespace ENSEMBLE_SAT_CHECKER {

    enum class EnsembleSatMode {
        NONE,
        VERITAS,
        Z3
    };

    extern std::unique_ptr<EnsembleSatChecker> construct_checker(const Jani2Ensemble& jani_2_ensemble, const PLAJA::Configuration& config);

}

/**********************************************************************************************************************/

namespace PLAJA_OPTION {
    extern const std::string ensemble_sat;

    namespace ENSEMBLE_SAT_CHECKER_PA {
        extern void add_options(PLAJA::OptionParser& option_parser);
        extern void print_options();

        extern std::string print_supported_ensemble_sat_modes();

        extern ENSEMBLE_SAT_CHECKER::EnsembleSatMode get_ensemble_sat_mode(const PLAJA::Configuration& config);
        extern bool is_relaxed_ensemble_sat_mode(const PLAJA::Configuration& config);
        extern ENSEMBLE_SAT_CHECKER::EnsembleSatMode get_ensemble_sat_mode(const std::string& mode);
        extern std::string get_ensemble_sat_mode(ENSEMBLE_SAT_CHECKER::EnsembleSatMode mode);
        extern void set_ensemble_sat_mode(PLAJA::Configuration& config, ENSEMBLE_SAT_CHECKER::EnsembleSatMode mode);
    }
}

namespace PLAJA_OPTION_DEFAULT {

    extern const std::string ensemble_sat;

}

#endif //PLAJA_ENSEMBLE_SAT_CHECKER_FACTORY_H
