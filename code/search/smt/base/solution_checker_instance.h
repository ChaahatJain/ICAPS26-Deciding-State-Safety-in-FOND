//
// This file is part of the PlaJA code base.
// Copyright (c) (2019 - 2023) Marcel Vinzent.
// See README.md in the top-level directory for licensing information.
//

#ifndef PLAJA_SOLUTION_CHECKER_INSTANCE_H
#define PLAJA_SOLUTION_CHECKER_INSTANCE_H

#include <memory>
#include <vector>
#include "../../../utils/default_constructors.h"
#include "../../../assertions.h"
#include "../../information/jani2nnet/using_jani2nnet.h"
#include "../../states/forward_states.h"
#include "../../successor_generation/forward_successor_generation.h"
#include "../../using_search.h"
#include "forward_solution_check.h"

/** Temporary object to interface the problem checked by the solution checker -- only implemented for PA. */
class SolutionCheckerInstance {
protected:
    const SolutionChecker* solutionChecker; // only implemented for PA

private:
    std::vector<std::unique_ptr<StateValues>> solutions; // vector fo solutions, possibly corresponding to a path

protected:
    void set_successor(const SolutionCheckWrapper& wrapper, StepIndex_type step, const Update& update);

public:
    explicit SolutionCheckerInstance(const SolutionChecker& solution_checker, std::size_t solution_size = 1);
    virtual ~SolutionCheckerInstance() = 0;
    DELETE_CONSTRUCTOR(SolutionCheckerInstance)

    [[nodiscard]] inline const SolutionChecker& get_checker() const { return *solutionChecker; }

    /** Check on set solution. */
    [[nodiscard]] virtual bool check() const = 0;

    [[nodiscard]] virtual bool check(const SolutionCheckWrapper& wrapper) = 0;

    virtual void set(const SolutionCheckWrapper& wrapper) = 0;

    [[nodiscard]] virtual PLAJA::floating policy_selection_delta() const = 0;

    [[nodiscard]] virtual bool policy_chosen_with_tolerance() const = 0;

    [[nodiscard]] virtual StateValues get_solution_constructor() const = 0;

    [[nodiscard]] inline std::size_t get_solution_size() const { return solutions.size(); }

    void set_solution(std::unique_ptr<StateValues>&& solution, StepIndex_type step = 0);

    void set_solution(std::vector<std::unique_ptr<StateValues>>&& solution);

    [[nodiscard]] const StateValues* get_solution(StepIndex_type step = 0) const;

    [[nodiscard]] std::unique_ptr<StateValues> retrieve_solution(StepIndex_type step = 0);

    [[nodiscard]] std::vector<std::unique_ptr<StateValues>> retrieve_solution_vector();

    /** To indicate that no (valid) solution is provided (e.g., to properly handle Marabou/Gurobi numeric issues). */
    void invalidate();

    [[nodiscard]] inline bool is_invalidated() const { return solutions.empty(); }

    [[nodiscard]] inline const StateValues* get_solution_if(StepIndex_type step) const { return is_invalidated() ? nullptr : get_solution(step); }

    /** Save this problem instance. */
    virtual void dump(const std::string& filename) const = 0;

    /** Save the problem instance + solution in a human-readable format. */
    virtual void dump_readable(const std::string& filename) const = 0;

};

#endif //PLAJA_SOLUTION_CHECKER_INSTANCE_H
