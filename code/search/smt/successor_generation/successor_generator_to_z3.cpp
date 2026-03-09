//
// This file is part of the PlaJA code base.
// Copyright (c) (2019 - 2021) Marcel Vinzent.
// See README.md in the top-level directory for licensing information.
//

#include <z3++.h>
#include "successor_generator_to_z3.h"
#include "../../information/model_information.h"
#include "../../successor_generation/successor_generator_c.h"
#include "../model/model_z3.h"
#include "../successor_generation/op_applicability.h"
#include "../visitor/extern/to_z3_visitor.h"
#include "../context_z3.h"
#include "../constraint_z3.h"
#include "action_op_to_z3.h"

SuccessorGeneratorToZ3::SuccessorGeneratorToZ3(const ModelZ3& model_smt):
    modelSmt(&model_smt)
    , modelInformation(&model_smt.get_model_info())
    , successorGenerator(&model_smt.get_successor_generator()) {

    /* Iterate action operators. */
    nonSyncOps.reserve(successorGenerator->nonSyncOps.size());
    for (const auto& non_sync_op: successorGenerator->nonSyncOps) {
        PLAJA_ASSERT(non_sync_op)
        nonSyncOps.push_back(std::unique_ptr<ActionOpToZ3>(new ActionOpToZ3(*non_sync_op, get_model_smt())));
    }

    auto l = successorGenerator->syncOps.size();
    syncOps.resize(l);
    for (std::size_t i = 0; i < l; ++i) {
        auto& sync_ops_per_sync = syncOps[i];
        sync_ops_per_sync.reserve(successorGenerator->syncOps[i].size());
        for (const auto& sync_op: successorGenerator->syncOps[i]) {
            PLAJA_ASSERT(sync_op)
            sync_ops_per_sync.push_back(std::unique_ptr<ActionOpToZ3>(new ActionOpToZ3(*sync_op, get_model_smt())));
        }
    }

    // cache action structures
    compute_action_structure(ACTION::silentLabel);
    auto num_actions = successorGenerator->get_sync_information().num_actions;
    labeledOps.resize(num_actions);
    for (ActionLabel_type action_label = 0; action_label < num_actions; ++action_label) {
        compute_action_structure(action_label);
    }

}

SuccessorGeneratorToZ3::~SuccessorGeneratorToZ3() = default;

/* */

void SuccessorGeneratorToZ3::compute_action_structure(ActionLabel_type action_label) {

    if (action_label == ACTION::silentAction) { // SILENT
        auto& ops_to_z3 = silentOps;
        // non sync
        for (auto& op_to_z3: nonSyncOps) {
            ops_to_z3.push_back(op_to_z3.get());
        }
        // silent synchs
        for (const SyncIndex_type sync_index: successorGenerator->get_sync_information().synchronisationsPerSilentAction) {
            for (auto& op_to_z3: syncOps[sync_index]) {
                ops_to_z3.push_back(op_to_z3.get());
            }
        }
    } else { // NON-SILENT
        PLAJA_ASSERT(0 <= action_label && action_label <= successorGenerator->get_sync_information().num_actions)
        auto& ops_to_z3 = labeledOps[action_label];
        // syncs
        for (const SyncIndex_type sync_index: successorGenerator->get_sync_information().synchronisationsPerAction[action_label]) {
            for (const auto& op_to_z3: syncOps[sync_index]) {
                ops_to_z3.push_back(op_to_z3.get());
            }
        }
    }

}

const ActionOpToZ3& SuccessorGeneratorToZ3::get_action_op_structure(ActionOpID_type action_op_id) const {
    const ActionOpIDStructure action_op_id_structure = modelInformation->compute_action_op_id_structure(action_op_id);
    if (action_op_id_structure.syncID == SYNCHRONISATION::nonSyncID) {
        return *nonSyncOps[action_op_id_structure.actionOpIndex];
    } else {
        const std::size_t sync_index = ModelInformation::sync_id_to_index(action_op_id_structure.syncID);
        return *syncOps[sync_index][action_op_id_structure.actionOpIndex];
    }
}

const std::list<const ActionOpToZ3*>& SuccessorGeneratorToZ3::get_action_structure(ActionLabel_type action_label) const {
    if (action_label == ACTION::silentAction) { // SILENT
        return silentOps;
    } else { // NON-SILENT
        PLAJA_ASSERT(0 <= action_label && action_label <= successorGenerator->get_sync_information().num_actions)
        return labeledOps[action_label];
    }
}

/* */

std::unique_ptr<Z3_IN_PLAJA::Constraint> SuccessorGeneratorToZ3::generate_action_op(ActionOpID_type action_op_id, const Z3_IN_PLAJA::StateVarsInZ3& src_vars, const Z3_IN_PLAJA::StateVarsInZ3& target_vars) const {
    return get_action_op_structure(action_op_id).to_z3(src_vars, target_vars, not get_model_smt().ignore_locs(), true);
}

std::unique_ptr<Z3_IN_PLAJA::Constraint> SuccessorGeneratorToZ3::generate_guards(ActionLabel_type label, StepIndex_type step) const {
    std::vector<z3::expr> guards_in_smt;

    const auto& src_vars = modelSmt->get_state_vars(step);

    const auto action_ops = get_action_structure(label);
    guards_in_smt.reserve(action_ops.size());
    for (const auto* action_op: action_ops) {

        if constexpr (PLAJA_GLOBAL::enableApplicabilityCache) {

            const auto* op_app = get_model_smt().get_op_applicability();

            if (op_app) {

                const auto app = op_app->get_applicability(action_op->_op_id());

                if (app == OpApplicability::Infeasible) { continue; }

                if (app == OpApplicability::Entailed) { return get_model_smt().get_context().trivially_true(); }

            }

        }

        auto guard_in_smt = action_op->guard_to_z3(src_vars, not get_model_smt().ignore_locs());
        if (guard_in_smt->is_trivially_true()) { continue; } // True.
        if (guard_in_smt->is_trivially_false()) { return guard_in_smt; } // False.

        guards_in_smt.push_back(guard_in_smt->move_to_expr());

    }

    /* Special case: trivially false. */
    if (guards_in_smt.empty()) { return get_model_smt().get_context().trivially_false(); }

    return std::make_unique<Z3_IN_PLAJA::ConstraintExpr>(Z3_IN_PLAJA::to_disjunction(guards_in_smt));
}

std::unique_ptr<Z3_IN_PLAJA::Constraint> SuccessorGeneratorToZ3::generate_action(const std::list<const ActionOpToZ3*>& action_structure, const Z3_IN_PLAJA::StateVarsInZ3& src_vars, const Z3_IN_PLAJA::StateVarsInZ3& target_vars) const {
    PLAJA_ASSERT(not action_structure.empty())
    std::vector<z3::expr> action_in_smt;
    action_in_smt.reserve(action_structure.size());

    for (const auto* op_to_z3: action_structure) {
        auto action_op_in_smt = op_to_z3->to_z3(src_vars, target_vars, not modelSmt->ignore_locs(), true);
        if (action_op_in_smt->is_trivially_true()) { continue; } // True.
        if (action_op_in_smt->is_trivially_false()) { return action_op_in_smt; } // False.
        action_in_smt.push_back(action_op_in_smt->move_to_expr());
    }

    PLAJA_ASSERT(not action_in_smt.empty())

    return std::make_unique<Z3_IN_PLAJA::ConstraintExpr>(Z3_IN_PLAJA::to_disjunction(action_in_smt));
}

std::unique_ptr<Z3_IN_PLAJA::Constraint> SuccessorGeneratorToZ3::generate_action(ActionLabel_type action_label, const Z3_IN_PLAJA::StateVarsInZ3& src_vars, const Z3_IN_PLAJA::StateVarsInZ3& target_vars) const {
    return generate_action(get_action_structure(action_label), src_vars, target_vars);
}

std::unique_ptr<Z3_IN_PLAJA::Constraint> SuccessorGeneratorToZ3::generate_expansion(const Z3_IN_PLAJA::StateVarsInZ3& src_vars, const Z3_IN_PLAJA::StateVarsInZ3& target_vars) const {

    std::vector<z3::expr> expansion_in_smt;

    if (not silentOps.empty()) {
        auto action_in_smt = generate_action(silentOps, src_vars, target_vars);
        if (action_in_smt->is_trivially_false()) { return action_in_smt; } // False.
        if (not action_in_smt->is_trivially_true()) { expansion_in_smt.push_back(action_in_smt->move_to_expr()); }  // If not true.
    }
    for (const auto& labeled_ops: labeledOps) {
        if (not labeled_ops.empty()) {
            auto action_in_smt = generate_action(labeled_ops, src_vars, target_vars);
            if (action_in_smt->is_trivially_false()) { return action_in_smt; } // False.
            if (not action_in_smt->is_trivially_true()) { expansion_in_smt.push_back(action_in_smt->move_to_expr()); } // If not true.
        }
    }

    PLAJA_ASSERT(not expansion_in_smt.empty())

    return std::make_unique<Z3_IN_PLAJA::ConstraintExpr>(Z3_IN_PLAJA::to_disjunction(expansion_in_smt));
}
