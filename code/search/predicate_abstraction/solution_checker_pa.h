//
// This file is part of the PlaJA code base.
// Copyright (c) (2019 - 2021) Marcel Vinzent.
// See README.md in the top-level directory for licensing information.
//

#ifndef PLAJA_SOLUTION_CHECKER_PA_H
#define PLAJA_SOLUTION_CHECKER_PA_H

#include "../smt/base/solution_checker.h"
#include "pa_states/forward_pa_states.h"

class SolutionCheckerPa final: public SolutionChecker {

public:
    SolutionCheckerPa(const PLAJA::Configuration& config, const Model& model, const Jani2Interface* interface);
    ~SolutionCheckerPa() override;
    DELETE_CONSTRUCTOR(SolutionCheckerPa)

    /* Solution interface. */

    using SolutionChecker::get_solution_constructor;
    [[nodiscard]] StateValues get_solution_constructor(const AbstractState& pa_state) const;
    // TODO: might try to reuse last solution instead
    //  intention: keep solution values (of preceding queries) for variables non-relevant to the current query;
    //  however: unlikely to be beneficial as source variables are always present in solver structures due to the predicates (in PA context).

    /* Check interface. */

    [[nodiscard]] bool check_abstraction(const StateBase& concrete_state, const AbstractState& abstract_state) const;  // Could generalize to PaStateBase since -- currently -- internal call drops subclass info anyways ...
    FCT_IF_DEBUG([[nodiscard]] bool check_state_and_abstraction(const StateValues& concrete_state, const AbstractState& abstract_state) const;)

#if 0
    using SolutionChecker::check_applicability;
    using SolutionChecker::check_transition;
    using SolutionChecker::check_policy;
    using SolutionChecker::check_policy_applicability;
    using SolutionChecker::check_policy_transition;
#endif

    [[nodiscard]] bool check_applicability(const StateBase& source, const ActionOp& action_op, const AbstractState* abstract_src) const;
    [[nodiscard]] bool check_transition(const StateValues& source, const AbstractState& abstract_src, const ActionOp& action_op, UpdateIndex_type update_index, const AbstractState& abstract_target, const StateValues* target = nullptr) const;
    [[nodiscard]] bool check_policy(const StateBase& source, ActionLabel_type action_label, const AbstractState* abstract_src) const;
    [[nodiscard]] bool check_policy_applicability(const StateBase& source, const ActionOp& action_op, const AbstractState* abstract_src) const;
    [[nodiscard]] bool check_policy_transition(const StateValues& source, const AbstractState& abstract_src, const ActionOp& action_op, UpdateIndex_type update_index, const AbstractState& abstract_target, const StateValues* target = nullptr) const;

    [[nodiscard]] bool check(const StateValues& solution, const AbstractState* pa_src, ActionLabel_type action_label, ActionOpID_type action_op_id, UpdateIndex_type update_index, const AbstractState* pa_target, const StateValues* target = nullptr) const;

};

#endif //PLAJA_SOLUTION_CHECKER_PA_H
