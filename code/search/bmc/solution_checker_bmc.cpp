//
// This file is part of the PlaJA code base.
// Copyright (c) (2019 - 2024) Marcel Vinzent.
// See README.md in the top-level directory for licensing information.
//

#include "solution_checker_bmc.h"
#include "../non_prob_search/policy/policy_restriction.h"
#include "../states/state_values.h"
#include "../successor_generation/action_op.h"
#include "../successor_generation/successor_generator_c.h"

SolutionCheckerBmc::SolutionCheckerBmc(const PLAJA::Configuration& config, const Model& model, const Jani2Interface* interface):
    SolutionChecker(config, model, interface)
    , successorGenerator(new SuccessorGeneratorC(config, model)) {
}

SolutionCheckerBmc::~SolutionCheckerBmc() = default;

/* */

std::pair<const ActionOp*, UpdateIndex_type> SolutionCheckerBmc::find_transition(const StateValues& src, const StateValues& suc, bool policy_aware) const {
    PLAJA_ASSERT(check_state(src))
    PLAJA_ASSERT(check_state(suc))

    successorGenerator->explore(src);

    if (policy_aware) {
        PLAJA_ASSERT(has_policy())
        get_policy()->evaluate(src);
    }

    for (auto it_label = successorGenerator->init_action_it_per_explore(); !it_label.end(); ++it_label) {

        if (policy_aware and not get_policy()->is_enabled(it_label.action_label())) { continue; }

        for (auto it_op = it_label.iterator(); !it_op.end(); ++it_op) {
            PLAJA_ASSERT(it_op->is_enabled(src))

            for (auto it_update = it_op->transitionIterator(); !it_update.end(); ++it_update) {
                PLAJA_ASSERT(it_update.prob() > 0)
                PLAJA_ASSERT(not successorGenerator->has_transient_variables())
                if (it_update.update().agrees(src, suc)) { return { it_op(), it_update.update_index() }; }

            }

        }

    }

    return { nullptr, ACTION::noneUpdate };

}