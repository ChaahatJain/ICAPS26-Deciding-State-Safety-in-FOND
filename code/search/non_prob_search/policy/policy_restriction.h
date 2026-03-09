//
// This file is part of the PlaJA code base.
// Copyright (c) (2019 - 2021) Marcel Vinzent.
// See README.md in the top-level directory for licensing information.
//

#ifndef PLAJA_POLICY_RESTRICTION_H
#define PLAJA_POLICY_RESTRICTION_H

#include <memory>
#include "../../../assertions.h"
#include "../../factories/forward_factories.h"
#include "../../information/jani_2_interface.h"
#include "../../states/forward_states.h"
#include "../../states/state_base.h"
#include "../../using_search.h"
#include "forward_policy.h"
#include "policy.h"
#include <iostream>
class PolicyRestriction {

private:
    const Policy* policy;
    mutable StateID_type cachedState;

public:
    explicit PolicyRestriction(const PLAJA::Configuration& config, const Jani2Interface& policy_interface);
    ~PolicyRestriction();
    DELETE_CONSTRUCTOR(PolicyRestriction)

    [[nodiscard]] const Jani2Interface& get_interface() const;

    static std::unique_ptr<PolicyRestriction> construct_policy_restriction(const PLAJA::Configuration& config);


    [[nodiscard]] bool is_enabled(const State& state, ActionLabel_type action_label) const;

    [[nodiscard]] bool is_enabled(const StateBase& state, ActionLabel_type action_label) const;

    [[nodiscard]] inline bool is_pruned(const State& state, ActionLabel_type action_label) const { return !is_enabled(state, action_label); }

    [[nodiscard]] inline bool is_pruned(const StateBase& state, ActionLabel_type action_label) const { return !is_enabled(state, action_label); }

    /* Checking applicability -- with tolerance -- for several labels over a single state. */

    void evaluate(const StateBase& state);

    ActionLabel_type get_action(const StateBase& state) { return policy->evaluate(state); }

    std::vector<double> action_values(const StateBase& state) { 
        return policy->action_values(state);
    }

    [[nodiscard]] bool is_enabled(ActionLabel_type action_label) const;

    /* */

    [[nodiscard]] PLAJA::floating get_selection_delta() const;

    [[nodiscard]] bool is_chosen_with_tolerance() const;

    FCT_IF_DEBUG(void dump_cache() const;)

};

#endif //PLAJA_POLICY_RESTRICTION_H
