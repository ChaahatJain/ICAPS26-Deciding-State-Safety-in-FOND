//
// This file is part of the PlaJA code base.
// Copyright (c) (2019 - 2023) Marcel Vinzent.
// See README.md in the top-level directory for licensing information.
//

#ifndef PLAJA_SOLUTION_CHECK_WRAPPER_H
#define PLAJA_SOLUTION_CHECK_WRAPPER_H

#include <memory>
#include "../../../utils/default_constructors.h"
#include "../../states/forward_states.h"
#include "../../using_search.h"
#include "solution_checker_instance.h"

/** Object to interface solution checker instance with SMT problem/solution. */
class SolutionCheckWrapper {

private:
    std::unique_ptr<::SolutionCheckerInstance> solutionCheckerInstance;

protected:

    explicit SolutionCheckWrapper(std::unique_ptr<::SolutionCheckerInstance>&& solution_checker_instance);
public:
    virtual ~SolutionCheckWrapper() = 0;
    DELETE_CONSTRUCTOR(SolutionCheckWrapper)

    [[nodiscard]] virtual std::unique_ptr<StateValues> to_state(StepIndex_type step, bool include_locs) const = 0;
    virtual void to_value(StepIndex_type step, StateValues& target, VariableIndex_type state_index) const = 0;

    [[nodiscard]] inline const ::SolutionCheckerInstance& get_instance() const { return *solutionCheckerInstance; }

    [[nodiscard]] inline ::SolutionCheckerInstance& get_instance() { return *solutionCheckerInstance; }

    [[nodiscard]] std::unique_ptr<::SolutionCheckerInstance> release_instance();

    /* short-cut interface to instance */

    [[nodiscard]] inline PLAJA::floating policy_selection_delta() const { return solutionCheckerInstance->policy_selection_delta(); }

    [[nodiscard]] inline bool policy_chosen_with_tolerance() const { return solutionCheckerInstance->policy_chosen_with_tolerance(); }

    [[nodiscard]] inline bool has_solution() const { return solutionCheckerInstance->get_solution(); }

    inline void invalidate() { solutionCheckerInstance->invalidate(); }

    [[nodiscard]] inline bool is_invalidated() const { return solutionCheckerInstance->is_invalidated(); }

    inline void dump(const std::string& filename) const { solutionCheckerInstance->dump(filename); }

};

#endif //PLAJA_SOLUTION_CHECK_WRAPPER_H
