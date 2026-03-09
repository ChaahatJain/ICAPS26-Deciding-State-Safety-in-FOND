//
// This file is part of the PlaJA code base.
// Copyright (c) (2019 - 2023) Marcel Vinzent.
// See README.md in the top-level directory for licensing information.
//

#ifndef PLAJA_PROBLEM_INSTANCE_CHECKER_PA_H
#define PLAJA_PROBLEM_INSTANCE_CHECKER_PA_H

#include "base/search_pa_base.h"

/**
 * Run SMT on a specific sub-problem.
 * Mostly for debugging and solver analysis (e.g., behavior for different encodings).
 */
class ProblemInstanceCheckerPa: public SearchPABase {

private:
    SearchStatus initialize() override;
    SearchStatus step() override;

    [[nodiscard]] HeuristicValue_type get_goal_distance(const AbstractState& pa_state) override;

public:
    explicit ProblemInstanceCheckerPa(const SearchEngineConfigPA& config);
    ~ProblemInstanceCheckerPa() override;
    DELETE_CONSTRUCTOR(ProblemInstanceCheckerPa)

};

#endif //PLAJA_PROBLEM_INSTANCE_CHECKER_PA_H
