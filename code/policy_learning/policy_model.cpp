//
// This file is part of the PlaJA code base.
// Copyright (c) (2019 - 2022) Marcel Vinzent.
// See README.md in the top-level directory for licensing information.
//

#include "policy_model.h"
#include "../exception/constructor_exception.h"
#include "../parser/ast/model.h"
#include "../search/factories/configuration.h"
#include "../search/states/state_values.h"
#include "../search/successor_generation/simulation_environment.h"
#include "../utils/rng.h"

namespace POLICY_TEACHER {

    std::unique_ptr<PolicyTeacher> construct_policy_model(const PLAJA::Configuration& config) {
        return std::make_unique<PolicyModel>(config);
    }

}

/**********************************************************************************************************************/

PolicyModel::PolicyModel(const PLAJA::Configuration& config):
    PolicyTeacher(config)
    , simulator(nullptr)
    , standardAutomataOffset(parentModel->get_number_automataInstances()) {

#if 0
    if (teacherModel->get_number_automataInstances() < standardAutomataOffset or
        teacherModel->get_number_all_variables() < parentModel->get_number_all_variables()) {
        throw ConstructorException(PLAJA_UTILS::string_f("Policy model (%s) has incompatible state structure.", teacherModel->get_name().c_str()));
    }
#endif

    PLAJA::Configuration config_(config);
    config_.delete_sharable(PLAJA::SharableKey::SUCC_GEN); // Simulation environment may not use same succ gen.
    simulator = std::make_unique<SimulationEnvironment>(config_, *teacherModel);

}

PolicyModel::~PolicyModel() = default;

/**********************************************************************************************************************/

std::vector<ActionLabel_type> PolicyModel::extract_applicable_actions(const StateBase& state, bool include_silent_action) const {
    const auto policy_model_state = translate_state_parent(state);
    return simulator->extract_applicable_actions(policy_model_state, include_silent_action);
}

ActionLabel_type PolicyModel::get_action(const StateBase& state) const {

    const auto applicable_actions = extract_applicable_actions(state, false);

    if (applicable_actions.empty()) { return ACTION::noneLabel; }

    return applicable_actions[PLAJA_GLOBAL::rng->index(applicable_actions.size())];
}