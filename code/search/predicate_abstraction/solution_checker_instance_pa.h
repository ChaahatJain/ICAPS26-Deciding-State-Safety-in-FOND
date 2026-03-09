//
// This file is part of the PlaJA code base.
// Copyright (c) (2019 - 2023) Marcel Vinzent.
// See README.md in the top-level directory for licensing information.
//

#ifndef PLAJA_SOLUTION_CHECKER_INSTANCE_PA_H
#define PLAJA_SOLUTION_CHECKER_INSTANCE_PA_H

#include "../../parser/ast/expression/forward_expression.h"
#include "../smt/base/solution_checker_instance.h"
#include "forward_pa.h"
#include "pa_states/forward_pa_states.h"
#include <list>
#include <vector>
#include "../states/state_values.h"

#include <unordered_set>

class SolutionCheckerInstancePa final: public SolutionCheckerInstance {

protected:
    const AbstractState* paSrc;
    ActionLabel_type actionLabel;
    ActionOpID_type actionOp;
    UpdateIndex_type updateIndex;
    const AbstractState* paTarget;

public:
    SolutionCheckerInstancePa(const SolutionCheckerPa& solution_checker, const AbstractState* pa_src, ActionLabel_type action_label, ActionOpID_type action_op, UpdateIndex_type update_index, const AbstractState* pa_target);
    SolutionCheckerInstancePa(const SolutionCheckerPa& solution_checker, const PATransitionStructure& transition_structure);
    ~SolutionCheckerInstancePa() override;
    DELETE_CONSTRUCTOR(SolutionCheckerInstancePa)

    [[nodiscard]] const SolutionCheckerPa& get_checker_pa() const;

    [[nodiscard]] bool check() const override;

    [[nodiscard]] bool check(const SolutionCheckWrapper& wrapper) override;

    void set(const SolutionCheckWrapper& wrapper) override;

    [[nodiscard]] PLAJA::floating policy_selection_delta() const override;

    [[nodiscard]] bool policy_chosen_with_tolerance() const override;

    [[nodiscard]] StateValues get_solution_constructor() const override;

    void dump(const std::string& filename) const override;

    void dump_readable(const std::string& filename) const override;

};

/**********************************************************************************************************************/


class SolutionCheckerInstancePaPath final: public SolutionCheckerInstance {

private:
    friend struct SolutionCheckerInstancePaPathAux;

    const AbstractPath* path;
    StepIndex_type targetStep;
    StepIndex_type policyTargetStep;
    const Expression* start;
    const Expression* reach;
    const bool paStateAware; // NOLINT(*-avoid-const-or-ref-data-members)
    const bool includeInit; // NOLINT(*-avoid-const-or-ref-data-members)

    mutable PLAJA::floating lastDelta;

public:
    SolutionCheckerInstancePaPath(const SolutionCheckerPa& solution_checker, const AbstractPath& path, StepIndex_type target_step, StepIndex_type policy_target_step, bool pa_state_aware, const Expression* start, bool include_init, const Expression* reach);
    ~SolutionCheckerInstancePaPath() override;
    DELETE_CONSTRUCTOR(SolutionCheckerInstancePaPath)

    [[nodiscard]] const SolutionCheckerPa& get_checker_pa() const;

    std::unique_ptr<SolutionCheckerInstancePaPath> copy_problem() const;

    inline void set_policy_target(StepIndex_type step) {
        policyTargetStep = step;
        PLAJA_ASSERT(policyTargetStep <= targetStep)
    }

    inline void inc_policy_target() {
        ++policyTargetStep;
        PLAJA_ASSERT(policyTargetStep <= targetStep)
    }

    /* Getter. */

    [[nodiscard]] inline StepIndex_type get_target_step() const { return targetStep; }

    [[nodiscard]] inline StepIndex_type get_policy_target_step() const { return policyTargetStep; }

    [[nodiscard]] inline const AbstractPath& get_path() const { return *path; }

    [[nodiscard]] inline bool is_pa_state_aware() const { return paStateAware; }

    [[nodiscard]] inline const Expression* get_start() const { return start; }

    [[nodiscard]] inline bool includes_init() const { return includeInit; }

    [[nodiscard]] inline const Expression* get_reach() const { return reach; }

    /**/

    [[nodiscard]] bool check() const override;

    [[nodiscard]] bool check(const SolutionCheckWrapper& wrapper) override;

    void set(const SolutionCheckWrapper& wrapper) override;

    [[nodiscard]] PLAJA::floating policy_selection_delta() const override;

    [[nodiscard]] bool policy_chosen_with_tolerance() const override;

    [[nodiscard]] StateValues get_solution_constructor() const override;

    [[nodiscard]] const StateValues& get_target() const;

    [[nodiscard]] std::unique_ptr<StateValues> retrieve_target();

    [[nodiscard]] const StateValues& get_policy_target() const;

    [[nodiscard]] std::unique_ptr<StateValues> retrieve_policy_target();

    void dump(const std::string& filename) const override;

    void dump_readable(const std::string& filename) const override;

    std::unordered_set<std::unique_ptr<StateBase>>  get_states_along_path() const;
    std::vector<StateValues> get_concrete_unsafe_path() const;
};

#endif //PLAJA_SOLUTION_CHECKER_INSTANCE_PA_H
