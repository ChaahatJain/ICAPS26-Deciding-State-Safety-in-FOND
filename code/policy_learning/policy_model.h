//
// This file is part of the PlaJA code base.
// Copyright (c) (2019 - 2022) Marcel Vinzent.
// See README.md in the top-level directory for licensing information.
//

#ifndef PLAJA_POLICY_MODEL_H
#define PLAJA_POLICY_MODEL_H

#include <memory>
#include <vector>
#include "../search/successor_generation/forward_successor_generation.h"
#include "policy_teacher.h"

class Model;

class PolicyModel final: public PolicyTeacher {

private:
    std::unique_ptr<SimulationEnvironment> simulator;
    std::size_t standardAutomataOffset;
    // The policy model must subsume the standard model in locations and variables
    // The behavior of the policy model for a state of the standard model extended by policy model default values,
    // should correspond to the intended behavior of the policy.

public:
    explicit PolicyModel(const PLAJA::Configuration& config);
    ~PolicyModel() override;
    DELETE_CONSTRUCTOR(PolicyModel)

    [[nodiscard]] std::vector<ActionLabel_type> extract_applicable_actions(const StateBase& state, bool include_silent_action) const;

    [[nodiscard]] ActionLabel_type get_action(const StateBase& state) const override;

};

#endif //PLAJA_POLICY_MODEL_H
