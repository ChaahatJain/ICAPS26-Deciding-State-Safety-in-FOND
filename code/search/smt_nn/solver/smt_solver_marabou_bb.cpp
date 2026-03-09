//
// This file is part of the PlaJA code base.
// Copyright (c) (2019 - 2023) Marcel Vinzent.
// See README.md in the top-level directory for licensing information.
//

#include "smt_solver_marabou_bb.h"
#include "../../../stats/stats_base.h"
#include "../../../globals.h"
#include "../../fd_adaptions/timer.h"
#include "solution_check_wrapper_marabou.h"
#include "solution_marabou.h"

#ifndef NDEBUG

#include "../../../exception/plaja_exception.h"

#endif

namespace MARABOU_IN_PLAJA {

    constexpr unsigned int maxBranchingDepthNonReal = 2;

    enum SolutionType {
        RealSol,
        IntSol,
        ExactIntSol,
        ClippedSol,
        None,
    };

    enum BranchingType {
        Lower,
        Upper,
        Fix,
    };

    struct BranchingVariable {

        MarabouVarIndex_type branchingVar;
        SolutionType type; // NOLINT(*-use-default-member-init)
        MarabouFloating_type solution;
        std::vector<std::pair<MarabouInteger_type, BranchingType>> branches;

        BranchingVariable():
            branchingVar(MARABOU_IN_PLAJA::noneVar)
            , type(SolutionType::None)
            , solution(std::numeric_limits<MarabouFloating_type>::infinity()) {
            branches.reserve(2);
        }
    };

}

/**********************************************************************************************************************/

MARABOU_IN_PLAJA::SmtSolverBB::SmtSolverBB(const PLAJA::Configuration& config, std::shared_ptr<MARABOU_IN_PLAJA::Context>&& c):
    SMTSolver(config, std::move(c))
    , maxBranchingDepthNonReal(MARABOU_IN_PLAJA::maxBranchingDepthNonReal)
    , onlyAcceptExactSolution(true)
    , branchingDepthNonReal(0)
    , branchingDepth(0)
    , hasContinuousVars(false) {
}

MARABOU_IN_PLAJA::SmtSolverBB::SmtSolverBB(const PLAJA::Configuration& config, std::unique_ptr<MARABOU_IN_PLAJA::MarabouQuery>&& query):
    SMTSolver(config, std::move(query))
    , maxBranchingDepthNonReal(MARABOU_IN_PLAJA::maxBranchingDepthNonReal)
    , onlyAcceptExactSolution(true)
    , branchingDepthNonReal(0)
    , branchingDepth(0)
    , hasContinuousVars(false) {
}

MARABOU_IN_PLAJA::SmtSolverBB::~SmtSolverBB() = default;

bool MARABOU_IN_PLAJA::SmtSolverBB::is_exact() const { return true; }

/**********************************************************************************************************************/

#if 0

MarabouVarIndex_type MARABOU_IN_PLAJA::SmtSolverBB::select_branching_var(const std::list<std::pair<MarabouVarIndex_type, MarabouInteger_type>>& candidates) {

    MarabouVarIndex_type branching_var = -1;
    MarabouInteger_type min_domain_size = std::numeric_limits<MarabouInteger_type>::max();

    for (const auto& [var, domain_size]: candidates) {
        if (domain_size < min_domain_size) {
            PLAJA_ASSERT(domain_size < std::numeric_limits<MarabouInteger_type>::max())
            branching_var = var;
            min_domain_size = domain_size;
        }
    }

    PLAJA_ASSERT(min_domain_size > 1)

    return branching_var;
}

#endif

void MARABOU_IN_PLAJA::SmtSolverBB::select_branching_var(BranchingVariable& target, const MARABOU_IN_PLAJA::Solution& solution) const {

    MarabouVarIndex_type candidate = MARABOU_IN_PLAJA::noneVar;
    MarabouInteger_type min_domain_size = std::numeric_limits<MarabouInteger_type>::max(); // Branch over *most constrained* variable (we favor fixing variables). // TODO try least-constrained?
    SolutionType type = SolutionType::None;

    const auto& query = _query();

    for (const auto& [var, var_sol]: solution) { // TODO might want to allow wrapper to indicate some structural preference, e.g., step-0 vars over step-1, non-det-assigned vars over det-assigned.

        if (not query.is_marked_integer(var)) { continue; }

        auto domain_size = query.get_domain_size_int(var);
        PLAJA_ASSERT(domain_size > 0) // Contradicting bounds -> unsat query, however we only can have a solution for sat queries.
        PLAJA_ASSERT(domain_size < std::numeric_limits<MarabouInteger_type>::max())

        /* Special handling of clipped variables. */

        const auto is_clipped = solution.is_clipped(var);

        if (domain_size == 1) { // Variable is fixed -> solution should be int.

            PLAJA_ASSERT(query.get_lower_bound(var) == query.get_upper_bound(var)) // Sanity ... (using internally stored float value on purpose, should really be equal at 0-tolerance).
            PLAJA_LOG_DEBUG_IF(not PLAJA_FLOATS::is_integer(var_sol, MARABOU_IN_PLAJA::integerPrecision), PLAJA_UTILS::string_f("Lower: %.12f, Sol: %.12f, Upper: %.12f (NON-INT)", get_lower_bound(var), var_sol, get_upper_bound(var_sol)))

            PLAJA_ASSERT(is_clipped or PLAJA_FLOATS::is_integer(var_sol, GlobalConfiguration::PREPROCESSOR_ALMOST_FIXED_THRESHOLD)) // At least integer by largest tolerance applied throughout Marabou.
            PLAJA_ASSERT(is_clipped or PLAJA_FLOATS::lte(get_lower_bound(var), var_sol, GlobalConfiguration::PREPROCESSOR_ALMOST_FIXED_THRESHOLD))
            PLAJA_ASSERT(is_clipped or PLAJA_FLOATS::lte(var_sol, get_upper_bound(var), GlobalConfiguration::PREPROCESSOR_ALMOST_FIXED_THRESHOLD))

            continue;

        }

        if (PLAJA_FLOATS::is_integer(var_sol, MARABOU_IN_PLAJA::integerPrecision)) {

            if (type == SolutionType::RealSol) { continue; } // Always prefer real-valued solutions for splitting.

            if (is_clipped) {

                if (type != SolutionType::ClippedSol or domain_size < min_domain_size) {
                    candidate = var;
                    min_domain_size = domain_size;
                    type = SolutionType::ClippedSol;
                }

                continue;

            }

            /* We distinguish two cases: variables with an exact integer solution and with an integer solution up to precision. */

            if (std::floor(var_sol) == std::ceil(var_sol)) { // We only consider these if no other case can be applied.

                if (type == None or (type == ExactIntSol or domain_size < min_domain_size)) {
                    candidate = var;
                    min_domain_size = domain_size;
                    type = ExactIntSol;
                }

            } else { // Integer up to precision.

                if (type == None or type == ExactIntSol or (type == IntSol or domain_size < min_domain_size)) {
                    candidate = var;
                    min_domain_size = domain_size;
                    type = IntSol;
                }

            }

        } else {

            PLAJA_ASSERT(not is_clipped) // Clipped (integer) variables are clipped to bound and thus integer.
            PLAJA_ASSERT(domain_size > 1) // Integer variables with domain size 1, should have clipped or integer solution.

            if (type != RealSol or domain_size < min_domain_size) { // Always prefer real-value solution for branching.
                candidate = var;
                min_domain_size = domain_size;
                type = RealSol;
            }

        }

    }

    /* Set target. */
    target.branchingVar = candidate;
    target.type = type;

    switch (target.type) {

        case None: {
            PLAJA_ASSERT(candidate == MARABOU_IN_PLAJA::noneVar)
            return;
        }

        case ClippedSol: {
            target.solution = solution.get_clipped_value(candidate);
            return;
        }

        default: {
            target.solution = solution.get_value(candidate);
            return;
        }

    }

    PLAJA_ABORT

}

/**********************************************************************************************************************/

void MARABOU_IN_PLAJA::SmtSolverBB::compute_branches(MARABOU_IN_PLAJA::BranchingVariable& branching_var) { // NOLINT(*-no-recursion)
    PLAJA_ASSERT(branching_var.branches.empty())

    const auto& query = _query();

    const auto var_sol = branching_var.solution;
    const auto cur_lower = query.get_lower_bound_int(branching_var.branchingVar);
    const auto cur_upper = query.get_upper_bound_int(branching_var.branchingVar);
    PLAJA_ASSERT(query.get_domain_size_int(branching_var.branchingVar) > 1) // Branching candidates are not fixed.

    switch (branching_var.type) {

        case SolutionType::RealSol: {

            /* This case assumes within bounds with infinite precision. */
            PLAJA_ASSERT(cur_lower <= var_sol)
            PLAJA_ASSERT(var_sol <= cur_upper)

            // [cur_lower, new_upper] and [new_lower, cur_upper]
            auto new_upper = PLAJA_UTILS::cast_numeric<MarabouInteger_type>(std::floor(var_sol));
            auto new_lower = PLAJA_UTILS::cast_numeric<MarabouInteger_type>(std::ceil(var_sol));
            PLAJA_ASSERT(cur_lower <= new_upper)
            PLAJA_ASSERT(new_lower <= cur_upper)
            PLAJA_ASSERT(new_upper < new_lower)

            // Check more-constrained branch first.
            if ((cur_upper - new_lower) <= (new_upper - cur_lower)) {
                branching_var.branches.emplace_back(new_lower, BranchingType::Lower);
                branching_var.branches.emplace_back(new_upper, BranchingType::Upper);
            } else {
                branching_var.branches.emplace_back(new_upper, BranchingType::Upper);
                branching_var.branches.emplace_back(new_lower, BranchingType::Lower);
            }

            return;

        }

        case SolutionType::IntSol: {

            /* This case assumes exact integer. */
            PLAJA_ASSERT(PLAJA_FLOATS::is_integer(branching_var.solution, MARABOU_IN_PLAJA::integerPrecision))

            /* Out-of-bounds due to floating point tolerance? */
            PLAJA_LOG_DEBUG_IF(var_sol < cur_lower or cur_upper < var_sol, PLAJA_UTILS::to_red_string(PLAJA_UTILS::string_f("Var %i, Current lower: %i, Sol: %.10f, Current upper: %i (OOB)", branching_var.branchingVar, cur_lower, var_sol, cur_upper)))

            if (var_sol < cur_lower) { // Due to floating point, we may have var_sol < cur_lower (while var_sol == cur_lower with Marabou's internal precision).

                PLAJA_ASSERT(FloatUtils::lte(cur_lower, var_sol)) // Should still be within bounds up to tolerance.

                if (sharedStatistics) { sharedStatistics->inc_attr_unsigned(PLAJA::StatsUnsigned::MARABOU_BB_BRANCH_OOB); }

                /* In one branch just fix variable at violated bound, check this branch first. */
                branching_var.branches.emplace_back(cur_lower, BranchingType::Upper);
                branching_var.branches.emplace_back(cur_lower + 1, BranchingType::Lower);

            } else if (var_sol > cur_upper) {

                PLAJA_ASSERT(FloatUtils::lte(var_sol, cur_upper)) // Should still be within bounds up to tolerance.

                if (sharedStatistics) { sharedStatistics->inc_attr_unsigned(PLAJA::StatsUnsigned::MARABOU_BB_BRANCH_OOB); }

                branching_var.branches.emplace_back(cur_upper, BranchingType::Lower);
                branching_var.branches.emplace_back(cur_upper - 1, BranchingType::Upper);

            } else {
                branching_var.type = SolutionType::RealSol;
                compute_branches(branching_var);
                branching_var.type = SolutionType::IntSol;
            }

            return;
        }

        case SolutionType::ExactIntSol: {

            if (sharedStatistics) { sharedStatistics->inc_attr_unsigned(PLAJA::StatsUnsigned::MARABOU_BB_BRANCH_EXACT_INT); }

            /* This case assumes exact integer. */
            PLAJA_ASSERT(std::floor(branching_var.solution) == std::ceil(branching_var.solution))
            PLAJA_ASSERT(PLAJA_FLOATS::is_integer(branching_var.solution, std::numeric_limits<MarabouFloating_type>::epsilon()))

            /* This case assumes exact integer within bounds. */
            PLAJA_ASSERT(cur_lower <= var_sol)
            PLAJA_ASSERT(var_sol <= cur_upper)

            auto new_upper = PLAJA_UTILS::cast_numeric<MarabouInteger_type>(std::round(var_sol));
            auto new_lower = new_upper;

            /* [cur_lower, var_sol - 1] or [var_sol + 1, cur_upper] or [var_sol, var_sol]
               Either Marabou finds an exact solution in one of the side branches (or maybe when fixing the value of the non-exact solution at query input). */
            bool solution_branch = true; // Only if neither of the two original branches is fixed already.

            if (var_sol < cur_upper) {
                new_lower += 1;
            } else { solution_branch = false; }

            if (cur_lower < new_upper) {
                new_upper -= 1;
            } else { solution_branch = false; }

            // Side branches.
            if ((cur_upper - new_lower) <= (new_upper - cur_lower)) {
                branching_var.branches.emplace_back(new_lower, BranchingType::Lower);
                branching_var.branches.emplace_back(new_upper, BranchingType::Upper);
            } else {
                branching_var.branches.emplace_back(new_upper, BranchingType::Upper);
                branching_var.branches.emplace_back(new_lower, BranchingType::Lower);
            }

            if (solution_branch) {
                // push();
                // solution.add_solution_exclusion(*this); // Fix variable, but exclude solution.
                branching_var.branches.emplace_back(var_sol, BranchingType::Fix);
                // pop();
            }

            return;
        }

        case SolutionType::ClippedSol: {

            /* This case assumes clipped variable. */
            PLAJA_LOG_DEBUG_IF(not(FloatUtils::lt(var_sol, cur_lower) or FloatUtils::lt(cur_upper, var_sol)), PLAJA_UTILS::to_red_string(PLAJA_UTILS::string_f("Var %i, Current lower: %i, Sol: %.10f, Current upper: %i (Clipped)", branching_var.branchingVar, cur_lower, var_sol, cur_upper)))
            PLAJA_ASSERT(FloatUtils::lt(var_sol, cur_lower) or FloatUtils::lt(cur_upper, var_sol))

            if (var_sol < cur_lower) {
                /* Split domain in two and first check split *not* next to the clipped value. */
                auto new_lower = PLAJA_UTILS::cast_numeric<MarabouInteger_type>(std::ceil(PLAJA_UTILS::cast_numeric<MarabouFloating_type>(cur_lower + cur_upper) / 2));
                PLAJA_ASSERT(new_lower > cur_lower)
                branching_var.branches.emplace_back(new_lower, BranchingType::Lower);
                branching_var.branches.emplace_back(new_lower - 1, BranchingType::Upper);
            } else {
                PLAJA_ASSERT(var_sol > cur_upper)
                auto new_upper = PLAJA_UTILS::cast_numeric<MarabouInteger_type>(std::floor(PLAJA_UTILS::cast_numeric<MarabouFloating_type>(cur_lower + cur_upper) / 2));
                PLAJA_ASSERT(new_upper < cur_upper)
                branching_var.branches.emplace_back(new_upper, BranchingType::Upper);
                branching_var.branches.emplace_back(new_upper + 1, BranchingType::Lower);
            }

            return;
        }

        case SolutionType::None: { PLAJA_ABORT }
    }

    PLAJA_ABORT;

}

/**********************************************************************************************************************/

void MARABOU_IN_PLAJA::SmtSolverBB::push_branch(MARABOU_IN_PLAJA::BranchingVariable& branching_var, unsigned int branch_index) {

    if (sharedStatistics) {
        sharedStatistics->inc_attr_unsigned(PLAJA::StatsUnsigned::MARABOU_BB_RECURSIONS);
        sharedStatistics->set_attr_unsigned(PLAJA::StatsUnsigned::MARABOU_BB_MAX_DEPTH, std::max(sharedStatistics->get_attr_unsigned(PLAJA::StatsUnsigned::MARABOU_BB_MAX_DEPTH), ++branchingDepth));
    }

    push();

    const auto [new_bound, type] = branching_var.branches[branch_index];

    /* Tighten bound. */
    switch (type) {

        case BranchingType::Lower: {
            // cur_lower <= var_sol < new_lower <= cur_upper, where ">" as new_lower is integer while var_sol is not.
            PLAJA_ASSERT(FloatUtils::lt(_query().get_lower_bound(branching_var.branchingVar), new_bound))
            PLAJA_ASSERT(FloatUtils::lte(new_bound, _query().get_upper_bound(branching_var.branchingVar)))
            tighten_lower_bound(branching_var.branchingVar, new_bound);
            break;
        }

        case BranchingType::Upper: {
            // cur_lower <= new_upper < var_sol <= cur_upper, where "<" as new_lower is integer while var_sol is not.
            PLAJA_ASSERT(FloatUtils::lte(_query().get_lower_bound(branching_var.branchingVar), new_bound))
            PLAJA_ASSERT(FloatUtils::lt(new_bound, _query().get_upper_bound(branching_var.branchingVar)))
            tighten_upper_bound(branching_var.branchingVar, new_bound);
            break;
        }

        case BranchingType::Fix: {
            // cur_lower <(=) new_bound <(=) cur_upper; where but is strict.
            PLAJA_ASSERT(FloatUtils::lt(_query().get_lower_bound(branching_var.branchingVar), new_bound) or FloatUtils::lt(new_bound, _query().get_upper_bound(branching_var.branchingVar)))
            PLAJA_ASSERT_EXPECT(FloatUtils::lt(_query().get_lower_bound(branching_var.branchingVar), new_bound)) // Expect both for now.
            PLAJA_ASSERT_EXPECT(FloatUtils::lt(new_bound, _query().get_upper_bound(branching_var.branchingVar)))
            tighten_lower_bound(branching_var.branchingVar, new_bound);
            tighten_upper_bound(branching_var.branchingVar, new_bound);
            break;
        }

        default: { PLAJA_ABORT }

    }

}

inline void MARABOU_IN_PLAJA::SmtSolverBB::pop_branch() {
    pop();
    if (sharedStatistics) { --branchingDepth; }
}

bool MARABOU_IN_PLAJA::SmtSolverBB::check_recursively() { // NOLINT(misc-no-recursion)

    /* Has solution to branch over ?*/
    if (not has_solution()) {
        if (solutionCheckerInstanceWrapper) { solutionCheckerInstanceWrapper->invalidate(); }
        if (sharedStatistics) { sharedStatistics->inc_attr_unsigned(PLAJA::StatsUnsigned::MARABOU_BB_NO_SOLUTION); }
        return true;
    }

    BranchingVariable branching_variable;

    { // Scope.

        const auto solution = extract_solution();

        /* Check (rounded) integer solution. */
#ifndef NDEBUG
        try {
#endif
            if (solutionCheckerInstanceWrapper and solutionCheckerInstanceWrapper->check(solution)) {
                if (solutionCheckerInstanceWrapper->policy_chosen_with_tolerance()) { PLAJA::StatsBase::inc_attr_unsigned(sharedStatistics, PLAJA::StatsUnsigned::MARABOU_BB_EXACT_WITH_ARGMAX_TOLERANCE); }
                return true;
            }
#ifndef NDEBUG
        } catch (PlajaException& e) {
            PLAJA_LOG_DEBUG(e.what())
            _query().save_query("exception_query.txt");
//            get_last_solution()->saveQuery("exception_query_last_solution.txt");
//            get_last_solution()->saveAssignment("exception_query_assignment.txt");
            PLAJA_ABORT;
        }
#endif

        select_branching_var(branching_variable, solution);

    }

    if (branching_variable.type == RealSol or branching_variable.type == ClippedSol) { compute_branches(branching_variable); }
    else {

        /* If no (exact) solution check is provided, we just assume that the solution is valid. */
        if (not solutionCheckerInstanceWrapper) { return true; }

        if (branching_variable.type == SolutionType::None) { // No continuous-solution variables to branch over ...

            if (hasContinuousVars) {

                if (sharedStatistics) { sharedStatistics->inc_attr_unsigned(PLAJA::StatsUnsigned::MARABOU_BB_NON_EXACT); }

                if (onlyAcceptExactSolution) { solutionCheckerInstanceWrapper->invalidate(); }

                return true; // Must accept non-exact solution.

            } else { return false; } // All integer variables are fixed, though exact solution checker refutes solution -> this sub-query is indeed unsat.

        } else { // All variables integer, thus non-exact int solution ...

            if (sharedStatistics) {
                sharedStatistics->inc_attr_unsigned(PLAJA::StatsUnsigned::MARABOU_BB_NON_EXACT_INT);
                sharedStatistics->inc_attr_double(PLAJA::StatsDouble::MARABOU_BB_NON_EXACT_INT_DELTA, solutionCheckerInstanceWrapper->policy_selection_delta());
            }

            compute_branches(branching_variable);

        }

    }

    PLAJA_ASSERT(not branching_variable.branches.empty())

    if (branching_variable.type != SolutionType::RealSol and ++branchingDepthNonReal > maxBranchingDepthNonReal) {
        /* Accept sat without solution. */
        if (sharedStatistics) { sharedStatistics->inc_attr_unsigned(PLAJA::StatsUnsigned::MARABOU_BB_NON_EXACT); }
        if (onlyAcceptExactSolution and solutionCheckerInstanceWrapper) { solutionCheckerInstanceWrapper->invalidate(); }
        return true;
    }

    bool rlt = false;

    for (unsigned int index = 0; index < branching_variable.branches.size(); ++index) {

        push_branch(branching_variable, index);

        rlt = check_relaxed();
        if (rlt) { rlt = check_recursively(); }

        pop_branch();

        if (rlt) { break; }

    }

    if (branching_variable.type != SolutionType::RealSol) { --branchingDepthNonReal; }

    return rlt;

}

bool MARABOU_IN_PLAJA::SmtSolverBB::check_init() {
    // solve recursively
    if (check_relaxed()) {

        const bool rlt = check_recursively();

// TODO: temporary solution so that a solution state can be extracted after call to init.
// This probably breaks CEGAR PA and related modules.
#ifndef BUILD_SAFE_START_GENERATOR
        reset();
#endif
        return rlt;

    } else {
        reset();
        return false;
    }
}

bool MARABOU_IN_PLAJA::SmtSolverBB::check() {
    branchingDepthNonReal = 0;
    PLAJA_ASSERT(branchingDepth == 0)


    /* Has continuous inputs? */
    hasContinuousVars = _query().has_continuous_inputs();

    if (sharedStatistics) {

        sharedStatistics->inc_attr_unsigned(PLAJA::StatsUnsigned::MARABOU_BB);
        PLAJA_GLOBAL::timer->push_lap(sharedStatistics, PLAJA::StatsDouble::TIME_MARABOU_BB);

        auto rlt = check_init();

        Timer::Time_type bb_time; // NOLINT(*-init-variables)
        POP_LAP_AND_CACHE(sharedStatistics, PLAJA::StatsDouble::TIME_MARABOU_BB, bb_time)
        if (not rlt) {
            sharedStatistics->inc_attr_unsigned(PLAJA::StatsUnsigned::MARABOU_BB_UNSAT);
            sharedStatistics->inc_attr_double(PLAJA::StatsDouble::TIME_MARABOU_BB_UNSAT, bb_time);
        }

        return rlt;

    } else { return check_init(); }
}
