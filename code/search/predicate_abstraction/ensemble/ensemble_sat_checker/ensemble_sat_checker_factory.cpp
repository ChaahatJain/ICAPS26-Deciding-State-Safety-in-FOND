//
// This file is part of the PlaJA code base.
// Copyright (c) (2023 - 2024) Chaahat Jain.
//

#include <memory>
#include "ensemble_sat_checker_factory.h"
#include "../../../../exception/constructor_exception.h"
#include "../../../../include/ct_config_const.h"
#include "../../../../option_parser/option_parser_aux.h"
#include "../../../../option_parser/option_parser.h"
#include "../../../../parser/ast/expression/non_standard/predicates_expression.h"
#include "../../../../parser/visitor/extern/to_linear_expression.h"
#include "../../../../parser/visitor/linear_constraints_checker.h"
#include "../../../factories/configuration.h"
#include "../ensemble_sat_in_z3/ensemble_sat_checker_z3.h"
#include "../smt/model_veritas_pa.h"

// extern
namespace PLAJA_OPTION { extern const std::string ensemble_mode; }

namespace PLAJA_OPTION_DEFAULT {
    const std::string ensemble_sat("veritas"); // NOLINT(cert-err58-cpp)
}

namespace PLAJA_OPTION {

    const std::string ensemble_sat("ensemble-sat"); // NOLINT(cert-err58-cpp)

    namespace SMT_SOLVER_VERITAS { extern std::string print_supported_bound_tightening(); }

    namespace ENSEMBLE_SAT_CHECKER_PA {

        void add_options(PLAJA::OptionParser& option_parser) {
            OPTION_PARSER::add_option(option_parser, PLAJA_OPTION::ensemble_sat, PLAJA_OPTION_DEFAULT::ensemble_sat);
        }

        void print_options() {
            OPTION_PARSER::print_option(PLAJA_OPTION::ensemble_sat, OPTION_PARSER::valueStr, PLAJA_OPTION_DEFAULT::ensemble_sat, "Set mode for Ensemble-satisfiability check to compute (relaxed) PPA):", true);
            OPTION_PARSER::print_additional_specification(ENSEMBLE_SAT_CHECKER_PA::print_supported_ensemble_sat_modes());
        }

        const std::unordered_map<std::string, ENSEMBLE_SAT_CHECKER::EnsembleSatMode> stringToEnsembleSatMode { // NOLINT(cert-err58-cpp)
            { "veritas",    ENSEMBLE_SAT_CHECKER::EnsembleSatMode::VERITAS },
            { "z3",         ENSEMBLE_SAT_CHECKER::EnsembleSatMode::Z3 },
             };

        std::string print_supported_ensemble_sat_modes() { return PLAJA::OptionParser::print_supported_options(stringToEnsembleSatMode); }

        ENSEMBLE_SAT_CHECKER::EnsembleSatMode get_ensemble_sat_mode(const PLAJA::Configuration& config) {
            if (not config.has_option(PLAJA_OPTION::ensemble_sat)) { return ENSEMBLE_SAT_CHECKER::EnsembleSatMode::NONE; }
            return config.get_option(PLAJA_OPTION::ENSEMBLE_SAT_CHECKER_PA::stringToEnsembleSatMode, PLAJA_OPTION::ensemble_sat);
        }


        ENSEMBLE_SAT_CHECKER::EnsembleSatMode get_ensemble_sat_mode(const std::string& mode) {
            auto it = stringToEnsembleSatMode.find(mode);
            if (it == stringToEnsembleSatMode.end()) { return ENSEMBLE_SAT_CHECKER::EnsembleSatMode::NONE; }
            else { return it->second; }
        }

        std::string get_ensemble_sat_mode(ENSEMBLE_SAT_CHECKER::EnsembleSatMode mode) {
            switch (mode) {
                case ENSEMBLE_SAT_CHECKER::EnsembleSatMode::VERITAS: { return "veritas"; }
                case ENSEMBLE_SAT_CHECKER::EnsembleSatMode::Z3: { return "z3"; }
                default: { PLAJA_ABORT }
            }
            PLAJA_ABORT
        }

        void set_ensemble_sat_mode(PLAJA::Configuration& config, ENSEMBLE_SAT_CHECKER::EnsembleSatMode mode) {
            if (mode == ENSEMBLE_SAT_CHECKER::EnsembleSatMode::NONE) { config.disable_value_option(PLAJA_OPTION::ensemble_sat); }
            else { config.set_value_option(PLAJA_OPTION::ensemble_sat, get_ensemble_sat_mode(mode)); }
        }

    }

}

/**********************************************************************************************************************/

std::unique_ptr<EnsembleSatChecker> ENSEMBLE_SAT_CHECKER::construct_checker(const Jani2Ensemble& jani_2_ensemble, const PLAJA::Configuration& config) {
    PLAJA_LOG("Constructing Ensemble-SAT checker ...")

    // Check predicates
    PLAJA_ASSERT(config.has_sharable(PLAJA::SharableKey::MODEL_VERITAS))
    PLAJA_ASSERT(config.get_sharable<ModelVeritas>(PLAJA::SharableKey::MODEL_VERITAS)->_predicates())
    for (auto pred_it = config.get_sharable<ModelVeritas>(PLAJA::SharableKey::MODEL_VERITAS)->_predicates()->predicatesIterator(); !pred_it.end(); ++pred_it) {
        if (not TO_LINEAR_EXP::is_linear_constraint(*pred_it) and not LinearConstraintsChecker::is_linear(*pred_it, LINEAR_CONSTRAINTS_CHECKER::Specification::as_predicate(true))) { throw ConstructorException(pred_it->to_string(), PLAJA_EXCEPTION::nonLinear); }
    }

    switch (config.get_option(PLAJA_OPTION::ENSEMBLE_SAT_CHECKER_PA::stringToEnsembleSatMode, PLAJA_OPTION::ensemble_sat)) {
        case ENSEMBLE_SAT_CHECKER::EnsembleSatMode::VERITAS: { return std::make_unique<EnsembleSatChecker>(jani_2_ensemble, config); }
        case ENSEMBLE_SAT_CHECKER::EnsembleSatMode::Z3: { return std::unique_ptr<EnsembleSatChecker>(new EnsembleSatCheckerZ3(jani_2_ensemble, config)); }
        default: { PLAJA_ABORT }
    }
    PLAJA_ABORT
}
