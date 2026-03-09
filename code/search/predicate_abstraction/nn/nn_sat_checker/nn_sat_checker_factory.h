//
// This file is part of the PlaJA code base.
// Copyright (c) (2019 - 2022) Marcel Vinzent.
// See README.md in the top-level directory for licensing information.
//

#ifndef PLAJA_NN_SAT_CHECKER_FACTORY_H
#define PLAJA_NN_SAT_CHECKER_FACTORY_H

#include <memory>
#include "../../../factories/forward_factories.h"
#include "../../../information/jani2nnet/forward_jani_2_nnet.h"

class NNSatChecker;

namespace NN_SAT_CHECKER {

    enum class NNSatMode {
        NONE,
        MARABOU,
        Z3,
        Z3_MARABOU
    };

    extern std::unique_ptr<NNSatChecker> construct_checker(const Jani2NNet& jani_2_nnet, const PLAJA::Configuration& config);

}

/**********************************************************************************************************************/

namespace PLAJA_OPTION {

    extern const std::string nn_sat;
    extern const std::string per_src_tightening;
    extern const std::string per_src_label_tightening;
    extern const std::string relaxed_for_exact;
    extern const std::string stalling_detection;
    extern const std::string adversarial_attack;
    extern const std::string ignore_attack_constraints;
    extern const std::string attack_solution_guess;
    extern const std::string attack_starts;
    extern const std::string pgd_steps;
    extern const std::string random_attack;
    extern const std::string scale_attack_step;
    extern const std::string constraint_delta_dir;
    extern const std::string query_cache;

    namespace NN_SAT_CHECKER_PA {
        extern void add_options(PLAJA::OptionParser& option_parser);
        extern void print_options();

        extern std::string print_supported_nn_sat_modes();

        extern NN_SAT_CHECKER::NNSatMode get_nn_sat_mode(const PLAJA::Configuration& config);
        extern bool is_relaxed_nn_sat_mode(const PLAJA::Configuration& config);
        extern NN_SAT_CHECKER::NNSatMode get_nn_sat_mode(const std::string& mode);
        extern std::string get_nn_sat_mode(NN_SAT_CHECKER::NNSatMode mode);
        extern void set_nn_sat_mode(PLAJA::Configuration& config, NN_SAT_CHECKER::NNSatMode mode);
    }
}

namespace PLAJA_OPTION_DEFAULT {

    extern const std::string nn_sat;

}

#endif //PLAJA_NN_SAT_CHECKER_FACTORY_H
