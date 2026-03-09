//
// This file is part of the PlaJA code base.
// Copyright (c) (2019 - 2023) Marcel Vinzent.
// See README.md in the top-level directory for licensing information.
//

#include "successor_generator_marabou.h"
#include "../../../exception/constructor_exception.h"
#include "../../../utils/utils.h"
#include "../../information/model_information.h"
#include "../../smt/successor_generation/op_applicability.h"
#include "../../successor_generation/successor_generator_c.h"
#include "../model/model_marabou.h"
#include "../constraints_in_marabou.h"
#include "../marabou_query.h"
#include "action_op_marabou.h"

MARABOU_IN_PLAJA::SuccessorGenerator::SuccessorGenerator(const SuccessorGeneratorC& successor_generator, const ModelMarabou& model, bool init):
    model(&model)
    , successorGenerator(&successor_generator) {

    if (init) { initialize_operators(); }

}

MARABOU_IN_PLAJA::SuccessorGenerator::~SuccessorGenerator() = default;

namespace MARABOU_IN_PLAJA {

    void print_op_warning() {
        static bool done = false;
        PLAJA_LOG_IF(not done, PLAJA_UTILS::to_red_string("Warning: Some non-labeled action op required SMT-fragment not supported by Marabou."))
        PLAJA_LOG_IF(not done, PLAJA_UTILS::to_red_string("Some features will not work properly."))
        PLAJA_LOG_IF(not done, PLAJA_UTILS::to_red_string("Continuing for legacy reasons. You might want to run in debug mode."))
        done = true;
    }

}

void MARABOU_IN_PLAJA::SuccessorGenerator::initialize_operators() {

    // iterate action operators
    nonSyncOps.reserve(successorGenerator->nonSyncOps.size());
    for (const auto& non_sync_op: successorGenerator->nonSyncOps) {
        PLAJA_ASSERT(non_sync_op)

#ifdef MARABOU_IGNORE_ARRAY_LEGACY
        try {
#endif
        nonSyncOps.push_back(initialize_op(*non_sync_op));
#ifdef MARABOU_IGNORE_ARRAY_LEGACY
        } catch (ConstructorException& e) {
            MARABOU_IN_PLAJA::print_op_warning();
            nonSyncOps.push_back(nullptr);
        }
#endif

    }

    const auto l = successorGenerator->syncOps.size();
    syncOps.resize(l);
    for (std::size_t i = 0; i < l; ++i) {
        auto& sync_ops_per_sync = syncOps[i];
        sync_ops_per_sync.reserve(successorGenerator->syncOps[i].size());
        for (const auto& sync_op: successorGenerator->syncOps[i]) {
            PLAJA_ASSERT(sync_op)
            PLAJA_ASSERT(i < successorGenerator->get_sync_information().synchronisationResults.size())

            if (PLAJA_GLOBAL::marabouIgnoreArrayLegacy and successorGenerator->get_sync_information().synchronisationResults[i] == ACTION::silentAction) {

                try { sync_ops_per_sync.push_back(initialize_op(*sync_op)); }
                catch (ConstructorException& e) {
                    MARABOU_IN_PLAJA::print_op_warning();
                    sync_ops_per_sync.push_back(nullptr);

                }
            } else {

                sync_ops_per_sync.push_back(initialize_op(*sync_op));

            }

        }
    }

    cache_per_label();

}

std::unique_ptr<ActionOpMarabou> MARABOU_IN_PLAJA::SuccessorGenerator::initialize_op(const ActionOp& action_op) const { return std::make_unique<ActionOpMarabou>(action_op, *model); }

void MARABOU_IN_PLAJA::SuccessorGenerator::cache_per_label() {

    { // SILENT

        // non sync
        for (auto& op: nonSyncOps) { silentOps.push_back(op.get()); }

        // silent synchs
        for (const SyncIndex_type sync_index: successorGenerator->get_sync_information().synchronisationsPerSilentAction) {
            for (auto& op: syncOps[sync_index]) { silentOps.push_back(op.get()); }
        }

        silentOps.shrink_to_fit();
    }

    // LABELED

    auto num_actions = successorGenerator->get_sync_information().num_actions;
    labeledOps.resize(num_actions);
    for (ActionLabel_type action_label = 0; action_label < num_actions; ++action_label) {
        auto& ops_labeled = labeledOps[action_label];
        // syncs
        for (const SyncIndex_type sync_index: successorGenerator->get_sync_information().synchronisationsPerAction[action_label]) {
            for (const auto& op: syncOps[sync_index]) { ops_labeled.push_back(op.get()); }
        }

        ops_labeled.shrink_to_fit();
    }

}

/**********************************************************************************************************************/

void MARABOU_IN_PLAJA::SuccessorGenerator::generate_steps() {

    for (auto& op: nonSyncOps) { if (op) { op->generate_steps(); } }

    for (auto& op_per_sync: syncOps) { for (auto& op: op_per_sync) { if (op) { op->generate_steps(); } } }

}

void MARABOU_IN_PLAJA::SuccessorGenerator::increment() {

    for (auto& op: nonSyncOps) { if (op) { op->increment(); } }

    for (auto& op_per_sync: syncOps) { for (auto& op: op_per_sync) { if (op) { op->increment(); } } }

}

/**********************************************************************************************************************/

const ActionOpMarabou& MARABOU_IN_PLAJA::SuccessorGenerator::get_action_op(ActionOpID_type op_id) const {
    const ActionOpIDStructure action_op_id_structure = model->get_model_info().compute_action_op_id_structure(op_id);
    if (action_op_id_structure.syncID == SYNCHRONISATION::nonSyncID) {

        PLAJA_ASSERT(nonSyncOps[action_op_id_structure.actionOpIndex])
        return *nonSyncOps[action_op_id_structure.actionOpIndex];

    } else {

        const std::size_t sync_index = ModelInformation::sync_id_to_index(action_op_id_structure.syncID);
        PLAJA_ASSERT(syncOps[sync_index][action_op_id_structure.actionOpIndex])
        return *syncOps[sync_index][action_op_id_structure.actionOpIndex];

    }
}

const std::vector<const ActionOpMarabou*>& MARABOU_IN_PLAJA::SuccessorGenerator::get_action_structure(ActionLabel_type label) const {

    if (label == ACTION::silentAction) {

        PLAJA_ASSERT(std::all_of(silentOps.begin(), silentOps.end(), [](const ActionOpMarabou* op) { return op != nullptr; }))
        return silentOps;

    } else {

        PLAJA_ASSERT(0 <= label && label <= successorGenerator->get_sync_information().num_actions)
        PLAJA_ASSERT(std::all_of(labeledOps[label].begin(), labeledOps[label].end(), [](const ActionOpMarabou* op) { return op != nullptr; }))
        return labeledOps[label];

    }

}

ActionLabel_type MARABOU_IN_PLAJA::SuccessorGenerator::get_action_label(ActionOpID_type action_op_id) const { return successorGenerator->get_action_label(action_op_id); }

/**********************************************************************************************************************/

void MARABOU_IN_PLAJA::SuccessorGenerator::add_guard(MARABOU_IN_PLAJA::QueryConstructable& query, ActionOpID_type op_id, bool do_locs, StepIndex_type step) const { get_action_op(op_id).add_guard(query, do_locs, step); }

void MARABOU_IN_PLAJA::SuccessorGenerator::add_update(MARABOU_IN_PLAJA::QueryConstructable& query, ActionOpID_type op_id, UpdateIndex_type update, bool do_locs, bool do_copy, StepIndex_type step) const { get_action_op(op_id).add_update(query, update, do_locs, do_copy, step); }

void MARABOU_IN_PLAJA::SuccessorGenerator::add_action_op(MARABOU_IN_PLAJA::QueryConstructable& query, ActionOpID_type op_id, UpdateIndex_type update, bool do_locs, bool do_copy, StepIndex_type step) const {
    const auto& action_op = get_action_op(op_id);
    action_op.add_guard(query, do_locs, step);
    action_op.add_update(query, update, do_locs, do_copy, step);
}

void MARABOU_IN_PLAJA::SuccessorGenerator::add_guard_disjunction(MARABOU_IN_PLAJA::QueryConstructable& query, ActionLabel_type label, StepIndex_type step) const { guards_to_marabou(label, step)->move_to_query(query); }

std::unique_ptr<MarabouConstraint> MARABOU_IN_PLAJA::SuccessorGenerator::guards_to_marabou(ActionLabel_type label, StepIndex_type step) const {
    std::unique_ptr<DisjunctionInMarabou> disjunction(new DisjunctionInMarabou(model->get_context()));

    for (const auto* action_op: get_action_structure(label)) {

        if constexpr (PLAJA_GLOBAL::enableApplicabilityCache) {

            const auto* op_app = model->get_op_applicability();

            if (op_app) {

                const auto app = op_app->get_applicability(action_op->_op_id());

                if (app == OpApplicability::Infeasible) { continue; }

                if (app == OpApplicability::Entailed) { return model->get_context().trivially_true(); }

            }

        }

        auto guard_in_marabou = action_op->guard_to_marabou(not model->ignore_locs(), step);
        if (guard_in_marabou->empty()) { return model->get_context().trivially_true(); }
        guard_in_marabou->move_to_disjunction(*disjunction);

    }

    // Special case: trivially false.
    if (disjunction->empty()) { return model->get_context().trivially_false(); }

    return disjunction->optimize();
}

std::unique_ptr<MarabouConstraint> MARABOU_IN_PLAJA::SuccessorGenerator::action_to_marabou(ActionLabel_type label, StepIndex_type step) const {

    const auto& action_structure = get_action_structure(label);

    /* Special cases. */
    if (action_structure.empty()) { return model->get_context().trivially_false(); }
    else if (action_structure.size() == 1) { return action_structure.front()->to_marabou(model->get_state_indexes(step), model->get_state_indexes(step + 1), not model->ignore_locs(), true); }

    auto disjunction = std::make_unique<DisjunctionInMarabou>(model->get_context());

    for (const auto* op: action_structure) {
        op->to_marabou(model->get_state_indexes(step), model->get_state_indexes(step + 1), not model->ignore_locs(), true)->move_to_disjunction(*disjunction);
    }

    PLAJA_ASSERT_EXPECT(not disjunction->empty()) // Let's revisit once we encounter this.

    return disjunction;
}

std::unique_ptr<MarabouConstraint> MARABOU_IN_PLAJA::SuccessorGenerator::expansion_to_marabou(StepIndex_type step) const {
    auto disjunction = std::make_unique<DisjunctionInMarabou>(model->get_context());

    if (not silentOps.empty()) {
        auto silent_ops_in_marabou = action_to_marabou(ACTION::silentAction, step);
        if (silent_ops_in_marabou->is_trivially_true()) { return model->get_context().trivially_true(); }
        if (not silent_ops_in_marabou->is_trivially_false()) { silent_ops_in_marabou->move_to_disjunction(*disjunction); }
    }

    for (ActionLabel_type label = 0; label < successorGenerator->get_sync_information().num_actions; ++label) {
        auto action_in_marabou = action_to_marabou(label, step);
        if (action_in_marabou->is_trivially_true()) { return model->get_context().trivially_true(); }
        if (not action_in_marabou->is_trivially_false()) { action_in_marabou->move_to_disjunction(*disjunction); }
    }

    return disjunction;
}