//
// This file is part of the PlaJA code base.
// Copyright (c) (2019 - 2024) Marcel Vinzent.
// See README.md in the top-level directory for licensing information.
//

#ifndef PLAJA_SOLUTION_CHECKER_BMC_H
#define PLAJA_SOLUTION_CHECKER_BMC_H

#include "../smt/base/solution_checker.h"

class SolutionCheckerBmc final: public SolutionChecker {

private:
    std::unique_ptr<SuccessorGeneratorC> successorGenerator;

public:
    SolutionCheckerBmc(const PLAJA::Configuration& config, const Model& model, const Jani2Interface* interface);
    ~SolutionCheckerBmc() override;
    DELETE_CONSTRUCTOR(SolutionCheckerBmc)

    [[nodiscard]] std::pair<const ActionOp*, UpdateIndex_type> find_transition(const StateValues& src, const StateValues& suc, bool policy_aware) const;

};

#endif //PLAJA_SOLUTION_CHECKER_BMC_H
