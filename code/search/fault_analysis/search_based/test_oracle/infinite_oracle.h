//
// Created by Chaahat Jain on 15/01/25.
//

#ifndef INFINITE_ORACLE_H
#define INFINITE_ORACLE_H

//
// This file is part of the PlaJA code base.
// Copyright (c) (2023 - 2024) Chaahat Jain.
//

// Intuition:
// Given transition s -a-> s' on unsafe policy path, try alternate actions. Check if they are safe.
// If any alternate action leads to safety, then we have detected a fault.

#include "../oracle.h"


// Given transition s -a-> s' on unsafe policy path, try alternate actions.
// s-a->s' is a fault if s-a'-> t and t is safe.


class InfiniteOracle: public Oracle {

public:
    explicit InfiniteOracle(const PLAJA::Configuration& config); // Constructor
    ~InfiniteOracle() override = default;

    enum class INFINITE_ORACLES {
        TARJAN,
        PI,
        PI2,
        PI3,
        PI3_INPUT_POLICY,
        PI3_RANDOM,
        LAO,
        LRTDP,
        BFS
    };
protected:

    /* Variable Hyper-parameters. */

    bool check_safe(const State& state) const override = 0;
    [[nodiscard]] bool check_if_fault(const State& state, const ActionLabel_type action) const override = 0;
    void clear(bool reset_all) const override = 0;
};

/* Factory. */
namespace FA_INFINITE_ORACLE {
    extern std::unique_ptr<InfiniteOracle> construct(const PLAJA::Configuration& config);
}

namespace PLAJA_OPTION {

    extern const std::string infinite_oracle;
    namespace FA_INFINITE_ORACLE {
        const std::unordered_map<std::string, InfiniteOracle::INFINITE_ORACLES> stringToInfiniteOracles { // NOLINT(cert-err58-cpp)
                            { "Tarjan", InfiniteOracle::INFINITE_ORACLES::TARJAN },
                            { "PI", InfiniteOracle::INFINITE_ORACLES::PI },
                            { "PI2", InfiniteOracle::INFINITE_ORACLES::PI2 },
                            { "PI3", InfiniteOracle::INFINITE_ORACLES::PI3 },
                            { "PI3-input-policy", InfiniteOracle::INFINITE_ORACLES::PI3_INPUT_POLICY },
                            { "PI3-random", InfiniteOracle::INFINITE_ORACLES::PI3_RANDOM },
                            { "LAO", InfiniteOracle::INFINITE_ORACLES::LAO },
                            { "LRTDP", InfiniteOracle::INFINITE_ORACLES::LRTDP },
                            { "BFS", InfiniteOracle::INFINITE_ORACLES::BFS }
                        };

        extern void add_options(PLAJA::OptionParser& option_parser);
        extern void print_options();
    }

}

#endif //INFINITE_ORACLE_H
