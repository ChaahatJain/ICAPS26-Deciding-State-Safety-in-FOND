//
// This file is part of the PlaJA code base.
// Copyright (c) (2019 - 2023) Marcel Vinzent.
// See README.md in the top-level directory for licensing information.
//

#ifndef PLAJA_EXPANSION_CACHE_PA_H
#define PLAJA_EXPANSION_CACHE_PA_H

#include <list>
#include <unordered_set>
#include "../../../utils/default_constructors.h"
#include "../../../assertions.h"
#include "../search_space/search_node_pa.h"
#include "../pa_states/pa_state_values.h"
#include "../pa_transition_structure.h"

#pragma clang diagnostic push
#pragma ide diagnostic ignored "misc-non-private-member-variables-in-classes"
#pragma ide diagnostic ignored "cppcoreguidelines-use-default-member-init"

struct ExpansionCachePa {

    PATransitionStructure transitionStruct;
    bool actionIsLearned;

    // Successor cache.
    std::list<StateID_type> newlyReached;
    std::unordered_set<StateID_type> currentSuccessors; // May be maintained per state, label, operator, ...

    // TODO rework structure, possibly combine with inc state space.
    std::unordered_map<ActionOpID_type, bool> applicableOps; // True if policy applicable.
    std::unordered_map<UpdateOpID_type, std::unique_ptr<PaStateValues>> computedTransitions; // For now we expect to pre-compute for each update at most one non-policy successor.
    std::unordered_map<UpdateOpID_type, std::unordered_set<StateID_type>> computedPolicyTransitions;

    bool foundTransition;
    bool hitReturn;
    SearchDepth_type currentSearchDepth;
    SearchDepth_type upperBoundGoalDistance;

    explicit ExpansionCachePa(const SEARCH_SPACE_PA::SearchNode& src):
        actionIsLearned(false)
        , currentSuccessors()
        , foundTransition(false)
        , hitReturn(false)
        , currentSearchDepth(src().get_start_distance())
        , upperBoundGoalDistance(std::numeric_limits<SearchDepth_type>::max()) {
    }

    ~ExpansionCachePa() = default;

    ExpansionCachePa() = delete;
    DELETE_CONSTRUCTOR(ExpansionCachePa)

    void set_op_applicable(ActionOpID_type op, bool under_policy) {
        PLAJA_ASSERT(not applicableOps.count(op))
        applicableOps[op] = under_policy;
    }

    void set_transition(UpdateOpID_type update_op, const AbstractState& target, bool under_policy) {
        PLAJA_ASSERT(update_op != ACTION::noneUpdateOp)

        if (under_policy) {
            PLAJA_ASSERT(not computedPolicyTransitions.count(update_op) or not computedPolicyTransitions.at(update_op).count(target.get_id_value())) // Should not have redundant calls.
            computedPolicyTransitions[update_op].insert(target.get_id_value());
        } else {
            PLAJA_ASSERT(not computedTransitions.count(update_op)) // Same update-op calls are not expected and thus not supported.
            computedTransitions[update_op] = target.to_pa_state_values(PLAJA_GLOBAL::debug);
        }

    }

    [[nodiscard]] bool op_is_applicable(ActionOpID_type op, bool under_policy) const {
        const auto it = applicableOps.find(op);
        return it != applicableOps.end() and (not under_policy or it->second);
    }

    [[nodiscard]] bool transition_computed_already(UpdateOpID_type update_op, const AbstractState& target, bool under_policy) const {
        PLAJA_ASSERT(update_op != ACTION::noneUpdateOp)

        {
            const auto it = computedPolicyTransitions.find(update_op);
            if (it != computedPolicyTransitions.end() and it->second.count(target.get_id_value())) { return true; }
        }

        if (not under_policy) {
            const auto it = computedTransitions.find(update_op);

            if (it != computedTransitions.end() and it->second->is_same_predicate_state(target)) {
                PLAJA_ASSERT(it->second->is_same_location_state(target))
                return true;
            }

        }

        return false;

    }

};

#pragma clang diagnostic pop

#endif //PLAJA_EXPANSION_CACHE_PA_H