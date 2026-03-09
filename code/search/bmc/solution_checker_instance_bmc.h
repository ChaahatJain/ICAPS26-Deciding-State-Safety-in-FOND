//
// This file is part of the PlaJA code base.
// Copyright (c) (2019 - 2024) Marcel Vinzent.
// See README.md in the top-level directory for licensing information.
//

#ifndef PLAJA_SOLUTION_CHECKER_INSTANCE_BMC_H
#define PLAJA_SOLUTION_CHECKER_INSTANCE_BMC_H

#include "../../parser/ast/expression/forward_expression.h"
#include "../factories/forward_factories.h"
#include "../smt/base/solution_checker_instance.h"
#include "forward_bmc.h"

class SolutionCheckerInstanceBmc final: public SolutionCheckerInstance {

private:
    StepIndex_type targetStep;
    StepIndex_type policyTargetStep;
    const Expression* start;
    const Expression* reach;
    const bool includeInit; // NOLINT(*-avoid-const-or-ref-data-members)
    const bool loopFree; // NOLINT(*-avoid-const-or-ref-data-members)

    mutable PLAJA::floating lastDelta;

    mutable std::vector<std::pair<const ActionOp*, UpdateIndex_type>> ops;

public:
    SolutionCheckerInstanceBmc(const SolutionCheckerBmc& solution_checker, StepIndex_type target_step, StepIndex_type policy_target_step, const Expression* start, bool include_init, const Expression* reach, bool loop_free);
    ~SolutionCheckerInstanceBmc() override;
    DELETE_CONSTRUCTOR(SolutionCheckerInstanceBmc)

    [[nodiscard]] const SolutionCheckerBmc& get_checker_bmc() const;

    [[nodiscard]] bool check() const override;

    [[nodiscard]] bool check(const SolutionCheckWrapper& wrapper) override;

    void set(const SolutionCheckWrapper& wrapper) override;

    [[nodiscard]] PLAJA::floating policy_selection_delta() const override;

    [[nodiscard]] bool policy_chosen_with_tolerance() const override;

    [[nodiscard]] StateValues get_solution_constructor() const override;

    void dump(const std::string& filename) const override;

    void dump_readable(const std::string& filename) const override;

};

#endif //PLAJA_SOLUTION_CHECKER_INSTANCE_BMC_H
