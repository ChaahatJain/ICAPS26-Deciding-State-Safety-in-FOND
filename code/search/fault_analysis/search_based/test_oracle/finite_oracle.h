//
// Created by Chaahat Jain on 18/11/24.
//

#ifndef FINITE_ORACLE_H
#define FINITE_ORACLE_H

//
// This file is part of the PlaJA code base.
// Copyright (c) (2023 - 2024) Chaahat Jain.
//

// Intuition:
// Given transition s -a-> s' on unsafe policy path, try alternate actions. Check if they are safe.
// If any alternate action leads to safety, then we have detected a fault.

#include <vector>
#include <unordered_set>
#include "../oracle.h"


// Given transition s -a-> s' on unsafe policy path, try alternate actions.
// s-a->s' is a fault if s-a'-> t and t is safe.


class FiniteOracle: public Oracle {

public:
    explicit FiniteOracle(const PLAJA::Configuration& config); // Constructor
    ~FiniteOracle() override = default;

    enum class FINITE_ORACLES {
        TARJAN,
        VI,
        LRTDP,
    };
protected:

    /* Variable Hyper-parameters. */
    int policy_deviations = 0;

    virtual bool check_safe(const State& state) const = 0;
    [[nodiscard]] bool check_if_fault(const State& state, const ActionLabel_type action) const override = 0;
    void clear(bool reset_all) const override = 0;
};

/* Factory. */
namespace FA_FINITE_ORACLE {
    extern std::unique_ptr<FiniteOracle> construct(const PLAJA::Configuration& config);
}

namespace PLAJA_OPTION {

    extern const std::string finite_oracle;
    extern const std::string policy_deviations;
    namespace FA_FINITE_ORACLE {
        const std::unordered_map<std::string, FiniteOracle::FINITE_ORACLES> stringToFiniteOracles { // NOLINT(cert-err58-cpp)
                        { "Tarjan", FiniteOracle::FINITE_ORACLES::TARJAN },
                        { "VI", FiniteOracle::FINITE_ORACLES::VI },
                        {"LRTDP", FiniteOracle::FINITE_ORACLES::LRTDP },
                    };

        extern void add_options(PLAJA::OptionParser& option_parser);
        extern void print_options();
    }

}

#endif //FINITE_ORACLE_H
