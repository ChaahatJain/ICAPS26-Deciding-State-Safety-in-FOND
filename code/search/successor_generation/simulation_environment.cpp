//
// This file is part of the PlaJA code base.
// Copyright (c) (2019 - 2020) Marcel Vinzent.
// See README.md in the top-level directory for licensing information.
//

#include "simulation_environment.h"
#include "../../parser/ast/iterators/model_iterator.h"
#include "../../parser/ast/action.h"
#include "../../utils/floating_utils.h"
#include "../../utils/rng.h"
#include "../factories/configuration.h"
#include "../fd_adaptions/state.h"
#include "../information/model_information.h"
#include "../non_prob_search/policy/policy_restriction.h"
#include "base/successor_distribution.h"
#include "action_op.h"
#include "successor_generator_c.h"
#include "../../globals.h"

SimulationEnvironment::SimulationEnvironment(const PLAJA::Configuration& config, const Model& model_ref):
    model(&model_ref)
    , modelInfo(&model->get_model_information())
    , policyRestriction(nullptr)
    , rng(PLAJA_GLOBAL::rng.get()) {

    if (config.has_sharable(PLAJA::SharableKey::SUCC_GEN)) {
        successorGenerator = config.get_sharable<SuccessorGeneratorC>(PLAJA::SharableKey::SUCC_GEN);
        PLAJA_ASSERT(&successorGenerator->get_model() == model)
    } else {
        successorGenerator = config.set_sharable<SuccessorGeneratorC>(PLAJA::SharableKey::SUCC_GEN, std::make_shared<SuccessorGeneratorC>(config, model_ref));
    }

    stateRegistry = std::make_unique<StateRegistry>(modelInfo->get_ranges_int(), modelInfo->get_lower_bounds_int(), modelInfo->get_upper_bounds_int(), modelInfo->get_floating_state_size());
    initialState = stateRegistry->set_state(modelInfo->get_initial_values());
}

SimulationEnvironment::~SimulationEnvironment() = default;

//

void SimulationEnvironment::set_policy_restriction(const Jani2Interface& interface) {
    PLAJA::Configuration configuration;
    configuration.set_sharable<SuccessorGeneratorC>(PLAJA::SharableKey::SUCC_GEN, successorGenerator); // for applicability filtering
    policyRestriction = std::make_unique<PolicyRestriction>(configuration, interface);
}

//

State SimulationEnvironment::get_state(const StateID_type id) const { return stateRegistry->lookup_state(id); }

State SimulationEnvironment::get_state(const StateValues& state_values) const { return stateRegistry->add_state(state_values); }

const ActionOp& SimulationEnvironment::get_action_op(ActionOpID_type action_op_id) const { return successorGenerator->get_action_op(action_op_id); }

// computations:

bool SimulationEnvironment::is_selected(const StateBase& state, ActionLabel_type action_label) const {
    PLAJA_ASSERT_EXPECT(not successorGenerator->is_terminal(state))
    return not(policyRestriction and policyRestriction->is_pruned(state, action_label));
}

bool SimulationEnvironment::is_applicable(const StateBase& state, const ActionOp& action_op) const {
    PLAJA_ASSERT_EXPECT(not successorGenerator->is_terminal(state))
    return action_op.is_enabled(state) and is_selected(state, action_op._action_label());
}

bool SimulationEnvironment::is_applicable(const StateBase& state, ActionOpID_type action_op_id) const {
    return is_applicable(state, successorGenerator->get_action_op(action_op_id));
}

bool SimulationEnvironment::is_applicable(const StateBase& state, ActionLabel_type action_label) const {
    return is_selected(state, action_label) and successorGenerator->is_applicable(state, action_label);
}

bool SimulationEnvironment::has_zero_probability(const StateBase& state, const ActionOp& action_op, UpdateIndex_type update_index) {
    return action_op.has_zero_probability(update_index, state);
}

bool SimulationEnvironment::has_zero_probability(const StateBase& state, ActionOpID_type action_op_id, UpdateIndex_type update_index) const {
    return SimulationEnvironment::has_zero_probability(state, successorGenerator->get_action_op(action_op_id), update_index);
}

//

std::vector<ActionLabel_type> SimulationEnvironment::extract_applicable_actions(const StateBase& state, bool including_silent_action) const {
    successorGenerator->explore(state);
    std::vector<ActionLabel_type> applicable_actions;

    // SILENT
    if (including_silent_action) {
        if (successorGenerator->is_applicable(ACTION::silentAction) and is_selected(state, ACTION::silentAction)) {
            applicable_actions.push_back(ACTION::silentAction);
        }
    }

    // NON-SILENT
    for (auto it = ModelIterator::actionIterator(*model); !it.end(); ++it) {
        const ActionID_type action_id = it->get_id();
        if (successorGenerator->is_applicable(action_id) and is_selected(state, action_id)) {
            applicable_actions.push_back(action_id);
        }
    }

    return applicable_actions;
}

std::vector<ActionOpID_type> SimulationEnvironment::extract_applicable_action_ops(const StateBase& state) {
    successorGenerator->explore(state);
    std::vector<ActionOpID_type> applicable_action_ops;
    applicable_action_ops.reserve(successorGenerator->_number_of_enabled_ops());

    if (policyRestriction) {
        for (auto action_it = successorGenerator->init_action_it_per_explore(); !action_it.end(); ++action_it) {
            if (policyRestriction->is_pruned(state, action_it.action_label())) { continue; }
            for (auto op_it = action_it.iterator(); !op_it.end(); ++op_it) {
                applicable_action_ops.push_back(op_it->_op_id());
            }
        }
    } else { // Simply add all applicable operators ...
        for (auto op_it = successorGenerator->init_action_op_it_per_explore(); !op_it.end(); ++op_it) { applicable_action_ops.push_back(op_it->_op_id()); }
    }

    return applicable_action_ops;
}

bool SimulationEnvironment::is_cached_applicable(ActionLabel_type action_label) const {
    return successorGenerator->is_applicable(action_label);
}

//

std::unique_ptr<State> SimulationEnvironment::compute_successor(const State& source, const ActionOp& action_op, UpdateIndex_type update_index) const {
    if (update_index == ACTION::noneUpdate) {
        // random update
        PLAJA::Prob_type prob = rng->prob();
        for (auto it = action_op.updateIterator(); !it.end(); ++it) {
            const PLAJA::Prob_type prob_update = it.prob(source);
            if (prob_update > prob) { 
                auto successor = successorGenerator->compute_successor(it.update(), source);
                return successor;
            }
            else { prob -= prob_update; }
        }
        PLAJA_ABORT
    } else {
        auto successor = successorGenerator->compute_successor(action_op.get_update(update_index), source);
        return successor;
    }
}

std::unique_ptr<State> SimulationEnvironment::compute_successor(const State& source, ActionOpID_type action_op_id, UpdateIndex_type update_index) const {
    return compute_successor(source, successorGenerator->get_action_op(action_op_id), update_index);
}

std::unique_ptr<State> SimulationEnvironment::compute_successor(const State& source, UpdateOpID_type update_op_id) const {
    auto update_op_structure = modelInfo->compute_update_op_structure(update_op_id);
    return compute_successor(source, update_op_structure.actionOpID, update_op_structure.updateIndex);
}

std::unique_ptr<State> SimulationEnvironment::compute_successor(const State& source, ActionLabel_type action_label) const {
    PLAJA_ASSERT(is_applicable(source, action_label))
    const auto applicable_ops = successorGenerator->get_action(source, action_label);
    PLAJA_ASSERT(not applicable_ops.empty())
    const auto chosen_index = rng->index(applicable_ops.size()); // NOLINT(cppcoreguidelines-narrowing-conversions)
    auto current_index = 0;
    for (const auto* action_op: applicable_ops) {
        if (current_index == chosen_index) { return compute_successor(source, *action_op, ACTION::noneUpdate); }
        else { ++current_index; }
    }
    PLAJA_ABORT
}

/**********************************************************************************************************************/
std::vector<StateID_type> SimulationEnvironment::compute_successors(const State& source, const ActionOp& action_op) const {
    // action_op.dump(nullptr, true);
    std::vector<StateID_type> successors;
    for (auto it = action_op.updateIterator(); !it.end(); ++it) {
        auto successor = successorGenerator->compute_successor(it.update(), source);
        // successor->dump(true);
        successors.push_back(successor->get_id_value());
    }
    return successors;
}

std::vector<StateID_type> SimulationEnvironment::compute_successors(const State& source, ActionLabel_type action_label) const {
    std::vector<StateID_type> succ_states;
    const auto applicable_ops = successorGenerator->get_action(source, action_label);
    for (const auto* action_op: applicable_ops) {
        auto successors = compute_successors(source, *action_op);
        for (auto succ: successors) {
            succ_states.push_back(succ);
        }
    }
    return succ_states;
}

/**********************************************************************************************************************/

std::unique_ptr<State> SimulationEnvironment::compute_successor_if_applicable(const State& source, const ActionOp& action_op, UpdateIndex_type update_index) const {
    if (is_applicable(source, action_op) and (update_index == ACTION::noneUpdate or not action_op.has_zero_probability(update_index, source))) {
        return compute_successor(source, action_op, update_index);
    } else { return nullptr; }
}

std::unique_ptr<State> SimulationEnvironment::compute_successor_if_applicable(const State& source, ActionOpID_type action_op_id, UpdateIndex_type update_index) const {
    return compute_successor_if_applicable(source, successorGenerator->get_action_op(action_op_id), update_index);
}

std::unique_ptr<State> SimulationEnvironment::compute_successor_if_applicable(const State& source, UpdateOpID_type update_op_id) const {
    auto update_op_structure = modelInfo->compute_update_op_structure(update_op_id);
    return compute_successor_if_applicable(source, update_op_structure.actionOpID, update_op_structure.updateIndex);
}

std::unique_ptr<State> SimulationEnvironment::compute_successor_if_applicable(const State& source, ActionLabel_type action_label) const {
    PLAJA_ASSERT_EXPECT(not successorGenerator->is_terminal(source))
    if (not is_selected(source, action_label)) { return nullptr; }
    // else:
    const auto applicable_ops = successorGenerator->get_action(source, action_label);
    if (applicable_ops.empty()) { return nullptr; }
    else {
        const auto chosen_index = rng->index(applicable_ops.size()); // NOLINT(cppcoreguidelines-narrowing-conversions)
        auto current_index = 0;
        for (const auto* action_op: applicable_ops) {
            if (current_index == chosen_index) { return compute_successor(source, *action_op, ACTION::noneUpdate); }
            else { ++current_index; }
        }
        PLAJA_ABORT
    }
}

/**********************************************************************************************************************/


std::unique_ptr<SuccessorDistribution> SimulationEnvironment::compute_successor_distribution(const State& source, const ActionOp& action_op) const {
    auto successor_distro = std::make_unique<SuccessorDistribution>();
    if (not is_applicable(source, action_op)) { return successor_distro; } // empty distro
    for (auto it = action_op.updateIterator(); !it.end(); ++it) {
        const PLAJA::Prob_type prob = it.prob(source);
        if (PLAJA_FLOATS::is_positive(prob, PLAJA::probabilityPrecision)) { successor_distro->add_successor(successorGenerator->compute_successor(it.update(), source), prob); }
    }
    return successor_distro;
}

std::unique_ptr<SuccessorDistribution> SimulationEnvironment::compute_successor_distribution(const State& source, ActionOpID_type action_op_id) const {
    return compute_successor_distribution(source, successorGenerator->get_action_op(action_op_id));
}

std::unique_ptr<SuccessorDistribution> SimulationEnvironment::compute_successor_distribution(const State& source, ActionLabel_type action_label) const {
    PLAJA_ASSERT_EXPECT(not successorGenerator->is_terminal(source))
    auto successor_distro = std::make_unique<SuccessorDistribution>();
    if (not is_selected(source, action_label)) { return successor_distro; }
    // else:
    auto applicable_ops = successorGenerator->get_action(source, action_label);
    const PLAJA::Prob_type scale_factor = 1.0 / static_cast<PLAJA::Prob_type>(applicable_ops.size());
    for (const auto* action_op: applicable_ops) {
        successor_distro->merge(compute_successor_distribution(source, *action_op), scale_factor);
    }
    return successor_distro;
}

const State* SimulationEnvironment::sample(const SuccessorDistribution& successor_distribution) const {
    PLAJA::Prob_type prob_sum = rng->prob();
    for (const auto& [state, prob]: successor_distribution._probDistro()) {
        if (prob > prob_sum) { return state.get(); }
        else { prob_sum -= prob; }
    }
    return nullptr;
}





