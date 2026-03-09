//
// This file is part of the PlaJA code base.
// Copyright (c) (2019 - 2023) Marcel Vinzent.
// See README.md in the top-level directory for licensing information.
//

#include "smt_solver_marabou_enum.h"
#include "../../states/state_values.h"
#include "../vars_in_marabou.h"
#include "solution_check_wrapper_marabou.h"
#include "solution_marabou.h"

MARABOU_IN_PLAJA::SmtSolverEnum::SmtSolverEnum(const PLAJA::Configuration& config, std::shared_ptr<MARABOU_IN_PLAJA::Context>&& c):
    MARABOU_IN_PLAJA::SMTSolver(config, std::move(c)) {}

MARABOU_IN_PLAJA::SmtSolverEnum::SmtSolverEnum(const PLAJA::Configuration& config, std::unique_ptr<MARABOU_IN_PLAJA::MarabouQuery>&& query):
    MARABOU_IN_PLAJA::SMTSolver(config, std::move(query)) {}

MARABOU_IN_PLAJA::SmtSolverEnum::~SmtSolverEnum() = default;

/* */

bool MARABOU_IN_PLAJA::SmtSolverEnum::is_exact() const { return true; }

/* */

bool MARABOU_IN_PLAJA::SmtSolverEnum::check_recursively(std::list<MarabouVarIndex_type>& input_list, MARABOU_IN_PLAJA::Solution& solution) { // NOLINT(misc-no-recursion)

    if (input_list.empty()) {

        return solutionCheckerInstanceWrapper->check(solution); // deprecated alternative: solve trivial query in marabou

    } else {

        // cache
        const MarabouVarIndex_type input_var = input_list.back();
        PLAJA_ASSERT(_query().is_marked_integer(input_var))
        input_list.pop_back();

        const MarabouInteger_type lower_bound = std::floor(_query().get_lower_bound(input_var));
        const MarabouInteger_type upper_bound = std::ceil(_query().get_upper_bound(input_var));

        // iterate
        for (MarabouInteger_type bound = lower_bound; bound <= upper_bound; ++bound) {
            solution.assign(input_var, bound);
            if (check_recursively(input_list, solution)) { return true; }
        }

        // reset
        input_list.push_back(input_var);
        return false;

    }

}

bool MARABOU_IN_PLAJA::SmtSolverEnum::check() {
    PLAJA_ASSERT(solutionCheckerInstanceWrapper) // enumeration assumes a solution checker instance is provided

    // check
    MARABOU_IN_PLAJA::Solution solution(_query());

    auto input_list = _query().generate_input_list();
    STMT_IF_DEBUG(for (auto marabou_var: input_list) { PLAJA_ASSERT(_query().is_marked_integer(marabou_var)) }) // enumeration is supported for discrete state space only

    const bool rlt = check_recursively(input_list, solution);

    PLAJA_ASSERT(solutionCheckerInstanceWrapper->has_solution())

    return rlt;
}