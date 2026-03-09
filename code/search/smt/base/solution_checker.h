//
// This file is part of the PlaJA code base.
// Copyright (c) (2019 - 2021) Marcel Vinzent.
// See README.md in the top-level directory for licensing information.
//

#ifndef PLAJA_SOLUTION_CHECKER_H
#define PLAJA_SOLUTION_CHECKER_H

#include <list>
#include <memory>
#include <vector>
#include "../../../include/ct_config_const.h"
#include "../../../parser/ast_forward_declarations.h"
#include "../../../utils/default_constructors.h"
#include "../../../assertions.h"
#include "../../factories/forward_factories.h"
#include "../../information/jani_2_interface.h"
#include "../../information/forward_information.h"
#include "../../non_prob_search/policy/forward_policy.h"
#include "../../states/forward_states.h"
#include "../../successor_generation/forward_successor_generation.h"

class SolutionChecker {

private:
    const ModelInformation* modelInfo;
    std::shared_ptr<const SuccessorGeneratorC> successorGenerator;
    std::unique_ptr<PolicyRestriction> policy;

protected:
    [[nodiscard]] PolicyRestriction* get_policy() const { return policy.get(); }

public:
    /**
     * @param nn_interface : Policy is not supported iff nn_interface is NULL.
     * @param config : Successor generator is constructed internally iff not provided.
     */
    SolutionChecker(const PLAJA::Configuration& config, const Model& model, const Jani2Interface* interface);
    virtual ~SolutionChecker();
    DELETE_CONSTRUCTOR(SolutionChecker)

    [[nodiscard]] const Model& get_model() const;

    [[nodiscard]] inline const ModelInformation& get_model_info() const { return *modelInfo; }

    [[nodiscard]] inline bool has_policy() const { return policy.get(); }

    /* getter */
    [[nodiscard]] const ActionOp& get_action_op(ActionOpID_type action_op_id) const;

    /* Solution interface. */

    [[nodiscard]] const StateValues& get_models_initial_values() const;

    [[nodiscard]] inline const StateValues& get_solution_constructor() const { return get_models_initial_values(); }

    [[nodiscard]] StateValues compute_successor_solution(const StateValues& solution, const ActionOp& action_op, UpdateIndex_type update_index) const;

    [[nodiscard]] bool agrees(const StateBase& solution, const ActionOp& action_op, UpdateIndex_type update_index, const StateBase& successor) const;

    /* Check interface. */

    [[nodiscard]] bool check_state(const StateValues& concrete_state) const;

#ifdef ENABLE_TERMINAL_STATE_SUPPORT
    [[nodiscard]] bool check_terminal(const StateBase& state) const;
    [[nodiscard]] const Expression* get_terminal() const;
#else

    [[nodiscard]] inline constexpr bool check_terminal(const StateBase&) const { return false; }

    [[nodiscard]] inline const Expression* get_terminal() const { PLAJA_ABORT }

#endif

    [[nodiscard]] bool check_applicability(const StateBase& source, const ActionOp& action_op) const;
    [[nodiscard]] bool check_transition(const StateValues& source, const ActionOp& action_op, UpdateIndex_type update_index, const StateValues& target) const;

    [[nodiscard]] bool check_policy(const StateBase& source, ActionLabel_type action_label) const;
    [[nodiscard]] bool check_policy_applicability(const StateBase& source, const ActionOp& action_op) const;
    [[nodiscard]] bool check_policy_transition(const StateValues& source, const ActionOp& action_op, UpdateIndex_type update_index, const StateValues& target) const;

    /** Delta between label of interest and label chosen with precision. */
    [[nodiscard]] PLAJA::floating policy_selection_delta() const;

    /** Did policy check pass due to tolerance? */
    [[nodiscard]] bool policy_chosen_with_tolerance() const;

};

#endif //PLAJA_SOLUTION_CHECKER_H
