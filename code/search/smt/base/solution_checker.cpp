//
// This file is part of the PlaJA code base.
// Copyright (c) (2019 - 2021) Marcel Vinzent.
// See README.md in the top-level directory for licensing information.
//

#include "solution_checker.h"
#include "../../../parser/ast/model.h"
#include "../../../globals.h"
#include "../../factories/configuration.h"
#include "../../information/model_information.h"
#include "../../non_prob_search/policy/policy_restriction.h"
#include "../../states/state_values.h"
#include "../../successor_generation/action_op.h"
#include "../../successor_generation/successor_generator_c.h"

SolutionChecker::SolutionChecker(const PLAJA::Configuration& config, const Model& model_ref, const Jani2Interface* interface):
    modelInfo(&model_ref.get_model_information())
    , policy(interface ? new PolicyRestriction(config, *interface) : nullptr) {

    if (not config.has_sharable(PLAJA::SharableKey::SUCC_GEN)) {
        config.set_sharable<SuccessorGeneratorC>(PLAJA::SharableKey::SUCC_GEN, std::make_shared<SuccessorGeneratorC>(config, model_ref));
    }

    successorGenerator = config.get_sharable_as_const<SuccessorGeneratorC>(PLAJA::SharableKey::SUCC_GEN);

    PLAJA_ASSERT(&successorGenerator->get_model() == &model_ref)

}

SolutionChecker::~SolutionChecker() = default;

/* */

const Model& SolutionChecker::get_model() const { return successorGenerator->get_model(); }

/* */

const ActionOp& SolutionChecker::get_action_op(ActionOpID_type action_op_id) const { return successorGenerator->get_action_op(action_op_id); }

/* */

const StateValues& SolutionChecker::get_models_initial_values() const { return modelInfo->get_initial_values(); }

StateValues SolutionChecker::compute_successor_solution(const StateValues& solution, const ActionOp& action_op, UpdateIndex_type update_index) const {
    PLAJA_ASSERT(solution.is_valid())
    PLAJA_ASSERT(update_index != ACTION::noneUpdate)
    PLAJA_ASSERT(not action_op.has_zero_probability(update_index, solution))
    return successorGenerator->compute_successor(action_op.get_update(update_index), solution);
}

bool SolutionChecker::agrees(const StateBase& solution, const ActionOp& action_op, UpdateIndex_type update_index, const StateBase& successor) const {
    PLAJA_ASSERT(solution.is_valid())
    PLAJA_ASSERT(not successorGenerator->has_transient_variables())
    PLAJA_ASSERT(update_index != ACTION::noneUpdate)
    PLAJA_ASSERT(not action_op.has_zero_probability(update_index, solution))
    PLAJA_ASSERT(successor.is_valid())
    return action_op.get_update(update_index).agrees(solution, successor);
}

/* Interface. */

bool SolutionChecker::check_state(const StateValues& concrete_state) const { return modelInfo->is_valid(concrete_state); }

#ifdef ENABLE_TERMINAL_STATE_SUPPORT

bool SolutionChecker::check_terminal(const StateBase& state) const {
    PLAJA_ASSERT(state.is_valid())
    return successorGenerator->is_terminal(state);
}

const Expression* SolutionChecker::get_terminal() const { return successorGenerator->get_terminal_state_condition(); }

#endif

/* */

bool SolutionChecker::check_applicability(const StateBase& source, const ActionOp& action_op) const {
    PLAJA_ASSERT(source.is_valid())
    PLAJA_ASSERT_EXPECT(not check_terminal(source))

    STMT_IF_DEBUG(
        if (PLAJA_GLOBAL::do_additional_outputs() and not action_op.is_enabled(source)) {
            PLAJA_LOG_DEBUG("Guard no sat.")
            source.dump(true);
            action_op.dump(nullptr, true);
        }
    )

    return action_op.is_enabled(source);
}

bool SolutionChecker::check_transition(const StateValues& source, const ActionOp& action_op, UpdateIndex_type update_index, const StateValues& target) const {
    if (not check_applicability(source, action_op) or action_op.has_zero_probability(update_index, source)) { return false; }

    PLAJA_ASSERT(target.is_valid()) // TODO: For now keep as assertion. Though, presumably not guaranteed (set_successor() in solution_checker_instance.cpp)
    if (false and not modelInfo->is_valid(target)) { // Target may actually be invalid.

        STMT_IF_DEBUG(

                if (PLAJA_GLOBAL::do_additional_outputs()) {
                    PLAJA_LOG_DEBUG("Target not in bounds.")
                    target.dump(true);
                }

        )

        return false;
    }


    STMT_IF_DEBUG(PLAJA_GLOBAL::push_additional_outputs_frame(true))
    PLAJA_ASSERT_EXPECT(agrees(source, action_op, update_index, target)) // TODO For now keep assertion.
    STMT_IF_DEBUG(PLAJA_GLOBAL::pop_additional_outputs_frame())

    return true;

}

bool SolutionChecker::check_policy(const StateBase& source, ActionLabel_type action_label) const {
    PLAJA_ASSERT(policy)
    PLAJA_ASSERT(source.is_valid())
    PLAJA_ASSERT_EXPECT(not check_terminal(source))

    STMT_IF_DEBUG(if (PLAJA_GLOBAL::do_additional_outputs() and not policy->is_enabled(source, action_label)) { policy->dump_cache(); })

    return policy->is_enabled(source, action_label);
}

bool SolutionChecker::check_policy_applicability(const StateBase& source, const ActionOp& action_op) const {
    return check_applicability(source, action_op) and check_policy(source, action_op._action_label());
}

bool SolutionChecker::check_policy_transition(const StateValues& source, const ActionOp& action_op, UpdateIndex_type update_index, const StateValues& target) const {
    return check_transition(source, action_op, update_index, target) and check_policy(source, action_op._action_label());
}

/* */

PLAJA::floating SolutionChecker::policy_selection_delta() const { return policy ? policy->get_selection_delta() : 0; }

bool SolutionChecker::policy_chosen_with_tolerance() const { return policy and policy->is_chosen_with_tolerance(); }

