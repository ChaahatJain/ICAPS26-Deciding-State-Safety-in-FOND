//
// This file is part of the PlaJA code base.
// Copyright (c) (2019 - 2021) Marcel Vinzent.
// See README.md in the top-level directory for licensing information.
//

#include "solution_checker_pa.h"
#include "../information/model_information.h"
#include "../../globals.h"
#include "../successor_generation/action_op.h"
#include "pa_states/abstract_state.h"

namespace SOLUTION_CHECKER_PA {

    const std::string debugInfoSrc("Source not in abstract state.");
    const std::string debugInfoTarget("Target not in abstract state.");

    /** Auxiliary to reduce code duplication. */
   template<bool mayBeNull, bool srcCheck>
    inline bool check_abstraction(const AbstractState* pa_state, const StateBase& concrete_state) {

        PLAJA_ASSERT(pa_state or mayBeNull)

        if ((not mayBeNull or pa_state) and not pa_state->is_abstraction(concrete_state)) {

            STMT_IF_DEBUG(

                if (PLAJA_GLOBAL::do_additional_outputs()) {
                    PLAJA_LOG_DEBUG(srcCheck ? debugInfoSrc : debugInfoTarget)
                    concrete_state.dump(true);
                    pa_state->dump();
                }

            )

            return false;
        }

        return true;

    }

}

/* */

SolutionCheckerPa::SolutionCheckerPa(const PLAJA::Configuration& config, const Model& model_ref, const Jani2Interface* interface):
    SolutionChecker(config, model_ref, interface) {
}

SolutionCheckerPa::~SolutionCheckerPa() = default;

/* */


StateValues SolutionCheckerPa::get_solution_constructor(const AbstractState& pa_state) const {
    // set locations in constructor
    auto solution = get_model_info().get_initial_values();
    for (auto loc_it = pa_state.init_loc_state_iterator(); !loc_it.end(); ++loc_it) {
        solution.assign_int<false>(loc_it.state_index(), loc_it().location_value(loc_it.state_index()));
    }
    return solution;
}

/* */

bool SolutionCheckerPa::check_applicability(const StateBase& source, const ActionOp& action_op, const AbstractState* abstract_src) const {
    return SolutionChecker::check_applicability(source, action_op) and SOLUTION_CHECKER_PA::check_abstraction<true, true>(abstract_src, source);
}

bool SolutionCheckerPa::check_transition(const StateValues& source, const AbstractState& abstract_src, const ActionOp& action_op, UpdateIndex_type update_index, const AbstractState& abstract_target, const StateValues* target) const {

    if (target) {

        if (not SolutionChecker::check_transition(source, action_op, update_index, *target)) { return false; }

        if (not SOLUTION_CHECKER_PA::check_abstraction<false, true>(&abstract_src, source)) {  return false; }

        if (not SOLUTION_CHECKER_PA::check_abstraction<false, false>(&abstract_target, *target)) {
            return false;
        }

        return true;

    } else {

        if (not check_applicability(source, action_op, &abstract_src) or action_op.has_zero_probability(update_index, source)) { return false; }

        const auto successor = compute_successor_solution(source, action_op, update_index);

        return get_model_info().is_valid(successor) and SOLUTION_CHECKER_PA::check_abstraction<false, false>(&abstract_target, successor);

    }

}

bool SolutionCheckerPa::check_policy(const StateBase& source, ActionLabel_type action_label, const AbstractState* abstract_src) const {
    return SolutionChecker::check_policy(source, action_label) and SOLUTION_CHECKER_PA::check_abstraction<true, true>(abstract_src, source);
}

bool SolutionCheckerPa::check_policy_applicability(const StateBase& source, const ActionOp& action_op, const AbstractState* abstract_src) const {
    return check_applicability(source, action_op, nullptr) and check_policy(source, action_op._action_label(), abstract_src);
}

bool SolutionCheckerPa::check_policy_transition(const StateValues& source, const AbstractState& abstract_src, const ActionOp& action_op, UpdateIndex_type update_index, const AbstractState& abstract_target, const StateValues* target) const {
    if (not check_transition(source, abstract_src, action_op, update_index, abstract_target, target)) {
        PLAJA_LOG("Target failing")
    }
    return check_transition(source, abstract_src, action_op, update_index, abstract_target, target) and check_policy(source, action_op._action_label(), nullptr);
}

/* Interface. */

bool SolutionCheckerPa::check_abstraction(const StateBase& concrete_state, const AbstractState& abstract_state) const {
    PLAJA_ASSERT(concrete_state.is_valid())
    return abstract_state.is_abstraction(concrete_state);
}

FCT_IF_DEBUG(bool SolutionCheckerPa::check_state_and_abstraction(const StateValues& concrete_state, const AbstractState& abstract_state) const { return check_state(concrete_state) and check_abstraction(concrete_state, abstract_state);})

bool SolutionCheckerPa::check(const StateValues& solution, const AbstractState* pa_src, ActionLabel_type action_label, ActionOpID_type action_op_id, UpdateIndex_type update_index, const AbstractState* pa_target, const StateValues* target) const {

    PLAJA_ASSERT(solution.is_valid()) // Should hold ...

    PLAJA_ASSERT_EXPECT(not check_terminal(solution)) // Let's revisit if this occurs ...
    if constexpr (PLAJA_GLOBAL::enableTerminalStateSupport) { if (check_terminal(solution)) { return false; } }

    PLAJA_ASSERT(has_policy()) // So far non-NN case is not supported. // TODO reset chosenWithTolerance flag in policy if we do not call the policy

    if (pa_target) {
        PLAJA_ASSERT(action_op_id != ACTION::noneOp)
        PLAJA_ASSERT(action_label == ACTION::noneLabel or action_label == get_action_op(action_op_id)._action_label())
        PLAJA_ASSERT(update_index != ACTION::noneUpdate)
        return check_policy_transition(solution, *pa_src, get_action_op(action_op_id), update_index, *pa_target, target); // transition test
    } else {
        if (action_op_id != ACTION::noneOp) {
            PLAJA_ASSERT(action_label == ACTION::noneLabel or action_label == get_action_op(action_op_id)._action_label())
            return check_policy_applicability(solution, get_action_op(action_op_id), pa_src);  // applicability test
        } else {
            PLAJA_ASSERT(action_label != ACTION::noneLabel)
            return check_policy(solution, action_label, pa_src);  // selection test
        }
    }

}
