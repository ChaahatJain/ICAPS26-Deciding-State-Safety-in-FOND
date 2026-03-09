//
// Created by Chaahat Jain on 15/01/25.
//

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
#include "infinite_oracle.h"
#include "infinite_oracles/tarjan_oracle.h"
#include "infinite_oracles/pi_oracle.h"
#include "infinite_oracles/pi2_oracle.h"
#include "infinite_oracles/pi3_oracle.h"
#include "infinite_oracles/lrtdp_oracle.h"
#include "infinite_oracles/lao_oracle.h"
#include "../../../../option_parser/option_parser.h"
#include "../../../../option_parser/option_parser_aux.h"
#include "../../../factories/configuration.h"


namespace PLAJA_OPTION_DEFAULT {
    const std::string infinite_oracle("Tarjan"); // NOLINT(cert-err58-cpp)
}

namespace PLAJA_OPTION {
    const std::string infinite_oracle("infinite-oracle"); // NOLINT(cert-err58-cpp)

    namespace FA_INFINITE_ORACLE {
        std::string print_supported_oracles() { return PLAJA::OptionParser::print_supported_options(stringToInfiniteOracles); }

        void add_options(PLAJA::OptionParser& option_parser) {
            OPTION_PARSER::add_option(option_parser, PLAJA_OPTION::infinite_oracle, PLAJA_OPTION_DEFAULT::infinite_oracle);
        }

        extern void print_options() {
            OPTION_PARSER::print_option(PLAJA_OPTION::infinite_oracle, OPTION_PARSER::valueStr, PLAJA_OPTION_DEFAULT::infinite_oracle, "Specify the oracle to be used.", true);
            OPTION_PARSER::print_additional_specification(print_supported_oracles(), true);
        }
    }
}

 InfiniteOracle::InfiniteOracle(const PLAJA::Configuration& config) : Oracle(config) {}


namespace FA_INFINITE_ORACLE {

    std::unique_ptr<InfiniteOracle> construct(const PLAJA::Configuration& config) {
        switch (config.get_option(PLAJA_OPTION::FA_INFINITE_ORACLE::stringToInfiniteOracles, PLAJA_OPTION::infinite_oracle)) {
            case InfiniteOracle::INFINITE_ORACLES::TARJAN: { return std::make_unique<TarjanOracle>(config); }
            case InfiniteOracle::INFINITE_ORACLES::PI: { return std::make_unique<PIOracle>(config); }
            case InfiniteOracle::INFINITE_ORACLES::PI2: { return std::make_unique<PI2Oracle>(config); }
            case InfiniteOracle::INFINITE_ORACLES::PI3: { return std::make_unique<PI3OracleZero>(config); }
            case InfiniteOracle::INFINITE_ORACLES::PI3_INPUT_POLICY: { return std::make_unique<PI3OracleInputPolicy>(config); }
            case InfiniteOracle::INFINITE_ORACLES::PI3_RANDOM: { return std::make_unique<PI3OracleRandom>(config); }
            case InfiniteOracle::INFINITE_ORACLES::LAO: { return std::make_unique<LAOOracle>(config); }
            case InfiniteOracle::INFINITE_ORACLES::LRTDP: { return std::make_unique<LRTDPOracle>(config); }
            case InfiniteOracle::INFINITE_ORACLES::BFS: { PLAJA_ABORT }
            default: { PLAJA_ABORT }
        }
        PLAJA_ABORT
    }
}