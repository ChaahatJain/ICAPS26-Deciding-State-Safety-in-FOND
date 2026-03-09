//
// This file is part of the PlaJA code base.
// Copyright (c) (2019 - 2020) Marcel Vinzent.
// See README.md in the top-level directory for licensing information.
//

#ifndef PLAJA_SIMULATION_ENVIRONMENT_H
#define PLAJA_SIMULATION_ENVIRONMENT_H

#include <memory>
#include <vector>
#include "../../parser/ast/forward_ast.h"
#include "../../parser/using_parser.h"
#include "../../assertions.h"
#include "../using_search.h"
#include "../factories/forward_factories.h"
#include "../information/jani_2_interface.h"
#include "../information/forward_information.h"
#include "../non_prob_search/policy/forward_policy.h"
#include "../states/forward_states.h"
#include "forward_successor_generation.h"

class SimulationEnvironment {
private:

    const Model* model;
    const ModelInformation* modelInfo;

    std::unique_ptr<State> initialState;
    std::unique_ptr<StateRegistry> stateRegistry; // Own structures for now, may think about sharing later.

    std::shared_ptr<SuccessorGeneratorC> successorGenerator;

    // policy oracle
    std::unique_ptr<PolicyRestriction> policyRestriction; // A policy oracle to decide whether an action label is applicable in a policy-restricted model; for checking paths.

    // simulation
    class RandomNumberGenerator* rng;

public:
    explicit SimulationEnvironment(const PLAJA::Configuration& config, const Model& model);
    ~SimulationEnvironment();
    DELETE_CONSTRUCTOR(SimulationEnvironment)

    /* Setter: */

    void set_policy_restriction(const Jani2Interface& interface);

    /* Getter: */

    [[nodiscard]] inline const Model& get_model() const {
        PLAJA_ASSERT(model)
        return *model;
    }

    [[nodiscard]] inline const ModelInformation& get_model_info() const {
        PLAJA_ASSERT(modelInfo)
        return *modelInfo;
    }

    [[nodiscard]] inline const State& get_initial_state() const {
        PLAJA_ASSERT(initialState)
        return *initialState;
    }

    State get_state(StateID_type id) const;

    State get_state(const StateValues& state_values) const;

    [[nodiscard]] const ActionOp& get_action_op(ActionOpID_type action_op_id) const;

    /* Simulation: */

    [[nodiscard]] bool is_selected(const StateBase& state, ActionLabel_type action_label) const;

    [[nodiscard]] bool is_applicable(const StateBase& state, const ActionOp& action_op) const;

    [[nodiscard]] bool is_applicable(const StateBase& state, ActionOpID_type action_op_id) const;

    [[nodiscard]] bool is_applicable(const StateBase& state, ActionLabel_type action_label) const;

    // (below are only for external usage)

    [[nodiscard]] static bool has_zero_probability(const StateBase& state, const ActionOp& action_op, UpdateIndex_type update_index);

    [[nodiscard]] bool has_zero_probability(const StateBase& state, ActionOpID_type action_op_id, UpdateIndex_type update_index) const;

    [[nodiscard]] std::vector<ActionLabel_type> extract_applicable_actions(const StateBase& state, bool including_silent_action) const;

    [[nodiscard]] std::vector<ActionOpID_type> extract_applicable_action_ops(const StateBase& state);

    /**
     * Action applicability with respect to last call to extract_applicable_actions or extract_applicable_action_ops.
     * @param action_label
     * @return
     */
    [[nodiscard]] bool is_cached_applicable(ActionLabel_type action_label) const;

    // Strictly speaking not const, as rng may be invoked, which is however secondary:

    [[nodiscard]] std::unique_ptr<State> compute_successor(const State& source, const ActionOp& action_op, UpdateIndex_type update_index = ACTION::noneUpdate) const;

    [[nodiscard]] std::unique_ptr<State> compute_successor(const State& source, ActionOpID_type action_op_id, UpdateIndex_type update_index = ACTION::noneUpdate) const;

    [[nodiscard]] std::unique_ptr<State> compute_successor(const State& source, UpdateOpID_type update_op_id) const;

    [[nodiscard]] std::unique_ptr<State> compute_successor(const State& source, ActionLabel_type action_label) const;

    // Compute all successors: 
    [[nodiscard]] std::vector<StateID_type> compute_successors(const State& source, const ActionOp& action_op) const;
    
    [[nodiscard]] std::vector<StateID_type> compute_successors(const State& source, ActionLabel_type action_label) const;

    //

    [[nodiscard]] std::unique_ptr<State> compute_successor_if_applicable(const State& source, const ActionOp& action_op, UpdateIndex_type update_index = ACTION::noneUpdate) const;

    [[nodiscard]] std::unique_ptr<State> compute_successor_if_applicable(const State& source, ActionOpID_type action_op_id, UpdateIndex_type update_index = ACTION::noneUpdate) const;

    [[nodiscard]] std::unique_ptr<State> compute_successor_if_applicable(const State& source, UpdateOpID_type update_op_id) const;

    [[nodiscard]] std::unique_ptr<State> compute_successor_if_applicable(const State& source, ActionLabel_type action_label) const;

    //

    [[nodiscard]] std::unique_ptr<SuccessorDistribution> compute_successor_distribution(const State& source, const ActionOp& action_op) const;

    [[nodiscard]] std::unique_ptr<SuccessorDistribution> compute_successor_distribution(const State& source, ActionOpID_type action_op_id) const;

    [[nodiscard]] std::unique_ptr<SuccessorDistribution> compute_successor_distribution(const State& source, ActionLabel_type action_label) const;

    [[nodiscard]] const State* sample(const SuccessorDistribution& successor_distribution) const;

};

#endif //PLAJA_SIMULATION_ENVIRONMENT_H
