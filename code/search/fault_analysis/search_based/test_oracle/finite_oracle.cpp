//
// Created by Chaahat Jain on 18/11/24.
//

//
// This file is part of the PlaJA code base.
// Copyright (c) (2023 - 2024) Chaahat Jain.
//

// Intuition:
// Given transition s -a-> s' on unsafe policy path, try alternate actions. Check if they are safe.
// If any alternate action leads to safety, then we have detected a fault.
#include "finite_oracle.h"
#include "finite_oracles/max_tarjan_finite_oracle.h"
#include "finite_oracles/vi_oracle.h"
#include "finite_oracles/max_lrtdp_finite_oracle.h"
#include "../../../../option_parser/option_parser.h"
#include "../../../../option_parser/option_parser_aux.h"
#include "../../../../option_parser/plaja_options.h"
#include "../../../factories/configuration.h"


namespace PLAJA_OPTION_DEFAULT {
    const std::string finite_oracle("Tarjan"); // NOLINT(cert-err58-cpp)
    constexpr int policy_deviations(100); // NOLINT(cert-err58-cpp)
}

namespace PLAJA_OPTION {
    const std::string finite_oracle("finite-oracle"); // NOLINT(cert-err58-cpp)
    const std::string policy_deviations("policy-deviations"); // NOLINT(cert-err58-cpp)

    namespace FA_FINITE_ORACLE {
        std::string print_supported_fin_oracles() { return PLAJA::OptionParser::print_supported_options(stringToFiniteOracles); }

        void add_options(PLAJA::OptionParser& option_parser) {
            OPTION_PARSER::add_option(option_parser, PLAJA_OPTION::finite_oracle, PLAJA_OPTION_DEFAULT::finite_oracle);
            OPTION_PARSER::add_int_option(option_parser, PLAJA_OPTION::policy_deviations, PLAJA_OPTION_DEFAULT::policy_deviations);
         //   DSMC_ORACLE::add_options(option_parser);
         //   ENUM_ORACLE::add_options(option_parser);
        }

        extern void print_options() {
            OPTION_PARSER::print_option(PLAJA_OPTION::finite_oracle, OPTION_PARSER::valueStr, PLAJA_OPTION_DEFAULT::finite_oracle, "Specify the oracle to be used.", true);
            OPTION_PARSER::print_additional_specification(print_supported_fin_oracles(), true);
            OPTION_PARSER::print_int_option(PLAJA_OPTION::policy_deviations, PLAJA_OPTION_DEFAULT::policy_deviations, "Specify the number of policy deviations to be used.");
        //    DSMC_ORACLE::print_options();
        //    ENUM_ORACLE::print_options();
        }
    }
}

FiniteOracle::FiniteOracle(const PLAJA::Configuration& config) : Oracle(config), policy_deviations(config.get_int_option(PLAJA_OPTION::policy_deviations)) {}


namespace FA_FINITE_ORACLE {

    std::unique_ptr<FiniteOracle> construct(const PLAJA::Configuration& config) {
        switch (config.get_option(PLAJA_OPTION::FA_FINITE_ORACLE::stringToFiniteOracles, PLAJA_OPTION::finite_oracle)) {
            case FiniteOracle::FINITE_ORACLES::TARJAN: { return std::make_unique<MaxTarjanFinOracle>(config); }
            case FiniteOracle::FINITE_ORACLES::VI: { return std::make_unique<VIOracle>(config); }
            case FiniteOracle::FINITE_ORACLES::LRTDP: { return std::make_unique<MaxLRTDPFinOracle>(config); }
            default: { PLAJA_ABORT }
        }
        PLAJA_ABORT
    }
}