//
// This file is part of the PlaJA code base.
// Copyright (c) (2019 - 2022) Marcel Vinzent.
// See README.md in the top-level directory for licensing information.
//

#ifndef PLAJA_CONSTANT_HEURISTIC_H
#define PLAJA_CONSTANT_HEURISTIC_H

#include "abstract_heuristic.h"

class ConstantHeuristic final: public AbstractHeuristic {
public:
    explicit ConstantHeuristic();
    ~ConstantHeuristic() final;

    void increment() override {}

    [[nodiscard]] HeuristicValue_type _evaluate(const AbstractState& /*state*/) final { return 0; }
};

#endif //PLAJA_CONSTANT_HEURISTIC_H
