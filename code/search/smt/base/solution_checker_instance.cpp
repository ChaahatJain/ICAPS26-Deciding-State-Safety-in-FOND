//
// This file is part of the PlaJA code base.
// Copyright (c) (2019 - 2023) Marcel Vinzent.
// See README.md in the top-level directory for licensing information.
//

#include <algorithm>
#include "solution_checker_instance.h"
#include "../../../include/factory_config_const.h"
#include "../../states/state_values.h"
#include "../../successor_generation/action_op.h"
#include "../../successor_generation/update.h"
#include "solution_check_wrapper.h"
#ifdef BUILD_PA

#include "../../predicate_abstraction/solution_checker_pa.h"

#endif

SolutionCheckerInstance::SolutionCheckerInstance(const SolutionChecker& solution_checker, std::size_t solution_size):
    solutionChecker(&solution_checker)
    , solutions(solution_size) {
}

SolutionCheckerInstance::~SolutionCheckerInstance() = default;

void SolutionCheckerInstance::set_solution(std::unique_ptr<StateValues>&& solution, StepIndex_type step) {
#ifdef BUILD_PA
    PLAJA_ASSERT(solutionChecker->check_state(*solution))
#endif
    PLAJA_ASSERT(step < solutions.size());
    solutions[step] = std::move(solution);
}

void SolutionCheckerInstance::set_solution(std::vector<std::unique_ptr<StateValues>>&& solution) {
    PLAJA_ASSERT(solutions.empty() or solutions.size() == solution.size())
#ifdef BUILD_PA
    PLAJA_ASSERT(std::any_of(solution.cbegin(), solution.cend(), [this](const auto& state) { return solutionChecker->check_state(*state); }))
#endif
    solutions = std::move(solution);
}

const StateValues* SolutionCheckerInstance::get_solution(StepIndex_type step) const {
    PLAJA_ASSERT(step < solutions.size());
    return solutions[step].get();
}

void SolutionCheckerInstance::set_successor(const SolutionCheckWrapper& wrapper, StepIndex_type step, const Update& update) {
    const auto successor_step = step + 1;

    const auto* src = get_solution(step);
    auto target = src->to_ptr();

    /* Just assign locations, irrespectively whether they are part of SMT. */
    for (auto it = update.locationIterator(); !it.end(); ++it) { target->assign_int<true>(it.loc(), it.dest()); }

    /* Deprecated. */
    // auto state_indexes = update.collect_updated_state_indexes(0, false, src);
    // for (auto state_index: state_indexes) { wrapper.to_value(successor_step, *target, state_index); }

    /*
     * Set updated state vars.
     * Use concrete update evaluation if possible.
     * For non-deterministically assigned variables load solution.
     * However, clip value if oob.
     * The latter is possible due to rounding (when using relaxed solvers),
     * e.g., z in [x + y, ], where sol(z) = 3.4, sol(x) = 1.6, sol(y) = 1.6),
     * s.t., wrapper loads target(z) = 3, target(x) = 2, target(y) = 2.
     */
    for (auto it = update.assignmentIterator(0); !it.end(); ++it) {

        if (it.is_non_deterministic()) {

            wrapper.to_value(successor_step, *target, it.variable_index(src));
            it.clip(src, *target);

        } else {
            it.evaluate(src, *target);
            PLAJA_ASSERT(target->is_valid(it.variable_index(src))) // TODO This is presumably not guaranteed. Src might be unreachable model state, hence evaluation might result in OOB.
        }

    }

    set_solution(std::move(target), successor_step);
}

std::unique_ptr<StateValues> SolutionCheckerInstance::retrieve_solution(StepIndex_type step) {
    PLAJA_ASSERT(step < solutions.size());
    PLAJA_ASSERT(solutions[step])
    return std::move(solutions[step]);
}

std::vector<std::unique_ptr<StateValues>> SolutionCheckerInstance::retrieve_solution_vector() { return std::move(solutions); }

void SolutionCheckerInstance::invalidate() { solutions.clear(); }
