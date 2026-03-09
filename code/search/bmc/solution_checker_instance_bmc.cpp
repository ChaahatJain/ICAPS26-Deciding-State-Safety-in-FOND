//
// This file is part of the PlaJA code base.
// Copyright (c) (2019 - 2024) Marcel Vinzent.
// See README.md in the top-level directory for licensing information.
//

#include <sstream>
#include "solution_checker_instance_bmc.h"
#include "../../parser/ast/expression/non_standard/problem_instance_expression.h"
#include "../../parser/ast/expression/expression.h"
#include "../../utils/utils.h"
#include "../information/model_information.h"
#include "../smt/base/solution_check_wrapper.h"
#include "../states/state_values.h"
#include "../states/state_set.h"
#include "../successor_generation/action_op.h"
#include "../successor_generation/successor_generator_c.h"
#include "solution_checker_bmc.h"

SolutionCheckerInstanceBmc::SolutionCheckerInstanceBmc(const SolutionCheckerBmc& solution_checker, StepIndex_type target_step, StepIndex_type policy_target_step, const Expression* start, bool include_init, const Expression* reach, bool loop_free):
    SolutionCheckerInstance(solution_checker, target_step + 1)
    , targetStep(target_step)
    , policyTargetStep(policy_target_step)
    , start(start)
    , reach(reach)
    , includeInit(include_init)
    , loopFree(loop_free)
    , lastDelta(0) {
    PLAJA_ASSERT(policy_target_step <= target_step)
}

SolutionCheckerInstanceBmc::~SolutionCheckerInstanceBmc() = default;

/* */

const SolutionCheckerBmc& SolutionCheckerInstanceBmc::get_checker_bmc() const { return PLAJA_UTILS::cast_ref<SolutionCheckerBmc>(get_checker()); }

/* */

bool SolutionCheckerInstanceBmc::check() const {

    /* CLear. */
    lastDelta = 0;
    ops.clear();

    auto& checker = get_checker_bmc();

    PLAJA::floating max_delta = 0;

    const auto* current = get_solution(0);
    PLAJA_ASSERT(checker.check_state(*current))

    /* Possible start constraints. */
    if (includeInit and start) {
        if (checker.get_model_info().get_initial_values() != (*current) and not start->evaluate_integer(*current)) {
            return false;
        }
    } else if (includeInit and checker.get_model_info().get_initial_values() != (*current)) { return false; }
    else if (start and not start->evaluate_integer(*current)) { return false; }

    for (StepIndex_type step = 0; step < targetStep; ++step) {
        PLAJA_ASSERT(current == get_solution(step))

        /* Check terminal. */
        PLAJA_ASSERT_EXPECT(not checker.check_terminal(*current))
        if (PLAJA_GLOBAL::enableTerminalStateSupport and checker.check_terminal(*current)) { return false; }

        const auto* successor = get_solution(step + 1);
        PLAJA_ASSERT(checker.check_state(*successor))

        const bool check_policy = checker.has_policy() and step < policyTargetStep;
        ops.push_back(checker.find_transition(*current, *successor, check_policy));
        if (check_policy) { max_delta = std::max(max_delta, checker.policy_selection_delta()); }

        /* No ops found?: */
        if (not ops.back().first) { return false; }

        current = successor;

    }

    /* Check terminal. */
    if (PLAJA_GLOBAL::enableTerminalStateSupport and not PLAJA_GLOBAL::reachMayBeTerminal and checker.check_terminal(*current)) { return false; }
    if constexpr (PLAJA_GLOBAL::reachMayBeTerminal) { PLAJA_ASSERT_EXPECT(not checker.check_terminal(*current)) }

    /* Reach condition. */
    if (reach and not reach->evaluate_integer(*current)) { return false; }

    /* Loop free. */
    if (loopFree) {
        StateValuesSet state_set;
        for (StepIndex_type step = 0; step < targetStep; ++step) { if (not state_set.insert(get_solution(step)).second) { return false; } }
    }

    lastDelta = max_delta;

    return true;

}

bool SolutionCheckerInstanceBmc::check(const SolutionCheckWrapper& wrapper) {
    set(wrapper);
    return check();
}

void SolutionCheckerInstanceBmc::set(const SolutionCheckWrapper& wrapper) {
    for (StepIndex_type step = 0; step <= targetStep; ++step) {
        set_solution(wrapper.to_state(step, true), step);
    }
}


PLAJA::floating SolutionCheckerInstanceBmc::policy_selection_delta() const { return lastDelta; }

bool SolutionCheckerInstanceBmc::policy_chosen_with_tolerance() const { return lastDelta > 0; }

StateValues SolutionCheckerInstanceBmc::get_solution_constructor() const { return get_checker().get_solution_constructor(); }

/**********************************************************************************************************************/

void SolutionCheckerInstanceBmc::dump(const std::string& filename) const {
    std::unique_ptr<ProblemInstanceExpression> problem_instance(new ProblemInstanceExpression());

    problem_instance->set_target_step(targetStep);
    problem_instance->set_policy_target_step(policyTargetStep);
    problem_instance->set_start(start);
    problem_instance->set_reach(reach);
    problem_instance->set_includes_init(includeInit);

    /* Op path. */
    if (not ops.empty()) {
        problem_instance->reserve_op_path();
        for (const auto& [op, update]: ops) {
            problem_instance->add_op_path_step(op->_op_id(), update);
        }
    }

    PropertyExpression::write_to_properties(std::move(problem_instance), filename);
}

void SolutionCheckerInstanceBmc::dump_readable(const std::string& filename) const {
    const auto* model = &get_checker().get_model();

    std::stringstream ss;
    ss << "PA-Path:" << std::endl;
    ss << "Target step: " << targetStep << std::endl;
    ss << "Policy target step: " << targetStep << std::endl;
    if (start) { ss << "Has start." << std::endl; }
    if (includeInit) { ss << "Includes init." << std::endl; }
    if (reach) { ss << "Has reach." << std::endl; }
    if (loopFree) { ss << "Has Loop-free." << std::endl; } // TODO: Not supported in PropertyExpression.

    for (StepIndex_type step = 0; step <= targetStep; ++step) {
        if (step < get_solution_size() and get_solution(step)) { ss << get_solution(step)->to_str(model) << std::endl; }
        if (step < ops.size()) {
            const auto& op_upd = ops[step];
            ss << op_upd.first->to_str(model, true) << std::endl;
            ss << ActionOp::update_to_str(op_upd.second) << std::endl;
        }
    }

    PLAJA_UTILS::write_to_file(filename, ss.str(), true);

}

