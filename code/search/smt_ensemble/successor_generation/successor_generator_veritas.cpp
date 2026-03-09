//
// This file is part of the PlaJA code base.
// Copyright (c) (2023 - 2024) Chaahat Jain.
//

#include "successor_generator_veritas.h"
#include "../../../exception/constructor_exception.h"
#include "../../../utils/utils.h"
#include "../../information/model_information.h"
#include "../../smt/successor_generation/op_applicability.h"
#include "../../successor_generation/successor_generator_c.h"
#include "../model/model_veritas.h"
#include "../constraints_in_veritas.h"
#include "../veritas_query.h"
#include "action_op_veritas.h"
#include "../../successor_generation/action_op.h"
#include "../../../parser/ast/expression/expression.h"


VERITAS_IN_PLAJA::SuccessorGenerator::SuccessorGenerator(const SuccessorGeneratorC& successor_generator, const ModelVeritas& model, bool init):
    model(&model)
    , successorGenerator(&successor_generator) {

    if (init) { initialize_operators(); }

}

VERITAS_IN_PLAJA::SuccessorGenerator::~SuccessorGenerator() = default;

namespace VERITAS_IN_PLAJA {

    void print_op_warning() {
        static bool done = false;
        PLAJA_LOG_IF(not done, PLAJA_UTILS::to_red_string("Warning: Some non-labeled action op required SMT-fragment not supported by Veritas."))
        PLAJA_LOG_IF(not done, PLAJA_UTILS::to_red_string("Some features will not work properly."))
        PLAJA_LOG_IF(not done, PLAJA_UTILS::to_red_string("Continuing for legacy reasons. You might want to run in debug mode."))
        done = true;
    }

}

void VERITAS_IN_PLAJA::SuccessorGenerator::initialize_operators() {

    // iterate action operators
    nonSyncOps.reserve(successorGenerator->nonSyncOps.size());
    for (const auto& non_sync_op: successorGenerator->nonSyncOps) {
        PLAJA_ASSERT(non_sync_op)

        try { nonSyncOps.push_back(initialize_op(*non_sync_op)); }

        catch (ConstructorException& e) {
            VERITAS_IN_PLAJA::print_op_warning();
            nonSyncOps.push_back(nullptr);
        }

    }

    auto l = successorGenerator->syncOps.size();
    syncOps.resize(l);
    for (std::size_t i = 0; i < l; ++i) {
        auto& sync_ops_per_sync = syncOps[i];
        sync_ops_per_sync.reserve(successorGenerator->syncOps[i].size());
        for (const auto& sync_op: successorGenerator->syncOps[i]) {
            PLAJA_ASSERT(sync_op)
            PLAJA_ASSERT(i < successorGenerator->get_sync_information().synchronisationResults.size())

            if (successorGenerator->get_sync_information().synchronisationResults[i] != ACTION::silentAction) {

                try {
                    sync_ops_per_sync.push_back(initialize_op(*sync_op));
                } catch (ConstructorException& e) {
                    VERITAS_IN_PLAJA::print_op_warning();
                    sync_ops_per_sync.push_back(nullptr);
                }

            } else { sync_ops_per_sync.push_back(initialize_op(*sync_op)); }

        }
    }

    cache_per_label();

}

std::unique_ptr<ActionOpVeritas> VERITAS_IN_PLAJA::SuccessorGenerator::initialize_op(const ActionOp& action_op) const { return std::make_unique<ActionOpVeritas>(action_op, *model); }

void VERITAS_IN_PLAJA::SuccessorGenerator::cache_per_label() {

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

void VERITAS_IN_PLAJA::SuccessorGenerator::generate_steps() {

    for (auto& op: nonSyncOps) { if (op) { op->generate_steps(); } }

    for (auto& op_per_sync: syncOps) { for (auto& op: op_per_sync) { if (op) { op->generate_steps(); } } }

}

void VERITAS_IN_PLAJA::SuccessorGenerator::increment() {

    for (auto& op: nonSyncOps) { if (op) { op->increment(); } }

    for (auto& op_per_sync: syncOps) { for (auto& op: op_per_sync) { if (op) { op->increment(); } } }

}

/**********************************************************************************************************************/

const ActionOpVeritas& VERITAS_IN_PLAJA::SuccessorGenerator::get_action_op(ActionOpID_type op_id) const {
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

const std::vector<const ActionOpVeritas*>& VERITAS_IN_PLAJA::SuccessorGenerator::get_action_structure(ActionLabel_type label) const {

    if (label == ACTION::silentAction) {

        PLAJA_ASSERT(std::all_of(silentOps.begin(), silentOps.end(), [](const ActionOpVeritas* op) { return op != nullptr; }))
        return silentOps;

    } else {

        PLAJA_ASSERT(0 <= label && label <= successorGenerator->get_sync_information().num_actions)
        PLAJA_ASSERT(std::all_of(labeledOps[label].begin(), labeledOps[label].end(), [](const ActionOpVeritas* op) { return op != nullptr; }))
        return labeledOps[label];

    }

}

ActionLabel_type VERITAS_IN_PLAJA::SuccessorGenerator::get_action_label(ActionOpID_type action_op_id) const { return successorGenerator->get_action_label(action_op_id); }

/**********************************************************************************************************************/

void VERITAS_IN_PLAJA::SuccessorGenerator::add_guard(VERITAS_IN_PLAJA::QueryConstructable& query, ActionOpID_type op_id, bool do_locs, StepIndex_type step) const { get_action_op(op_id).add_guard(query, do_locs, step); }

void VERITAS_IN_PLAJA::SuccessorGenerator::add_update(VERITAS_IN_PLAJA::QueryConstructable& query, ActionOpID_type op_id, UpdateIndex_type update, bool do_locs, bool do_copy, StepIndex_type step) const { get_action_op(op_id).add_update(query, update, do_locs, do_copy, step); }

void VERITAS_IN_PLAJA::SuccessorGenerator::add_action_op(VERITAS_IN_PLAJA::QueryConstructable& query, ActionOpID_type op_id, UpdateIndex_type update, bool do_locs, bool do_copy, StepIndex_type step) const {
    const auto& action_op = get_action_op(op_id);
    action_op.add_guard(query, do_locs, step);
    action_op.add_update(query, update, do_locs, do_copy, step);
}

veritas::Tree VERITAS_IN_PLAJA::SuccessorGenerator::get_label_filter_tree(ActionLabel_type action_label) {
    /**
     * Compute app filter tree for a label. The action is applicable if O1 + O2 + ... + On >= 1.
    */
    auto label_ops = get_action_structure(action_label);
    veritas::Tree tree(1);
    std::string prefix = "";
    for (auto op: label_ops) {
        auto opId = op->_op_id();
        auto var = model->get_operator_aux_var(opId);
        tree.split(tree[prefix.c_str()], {var, 1});
        tree.leaf_value(tree[(prefix + "r").c_str()], 0) = 0;
        prefix += "l";
    }
    tree.leaf_value(tree[prefix.c_str()], 0) = VERITAS_IN_PLAJA::negative_infinity;
    return tree;
}

std::vector<veritas::AddTree> VERITAS_IN_PLAJA::SuccessorGenerator::get_applicability_filter() {
    /**
     * Compute the optimized app filter.
    */
    int num_actions = labeledOps.size();
    std::vector<veritas::AddTree> treesPerLabel;
    treesPerLabel.reserve(num_actions);
    for (int i = 0; i < num_actions; ++i) {
        treesPerLabel.push_back(veritas::AddTree(0, veritas::AddTreeType::REGR));
    }
    for(int action_label = 0; action_label < num_actions; ++action_label) {
        veritas::AddTree trees(1, veritas::AddTreeType::REGR);
        trees.add_tree(get_label_filter_tree(action_label));
        auto appTrees = trees.make_multiclass(action_label, num_actions);
        treesPerLabel[action_label] = appTrees;
    }
    return treesPerLabel;
}

veritas::AddTree VERITAS_IN_PLAJA::SuccessorGenerator::get_indicator_trees() {
    veritas::AddTree trees(1, veritas::AddTreeType::REGR);
    int num_actions = labeledOps.size();
    for (int action_label = 0; action_label < num_actions; ++action_label) {
        auto label_ops = get_action_structure(action_label);
        for (auto op : label_ops) {
            auto ts = op->get_applicability_tree();
            trees.add_trees(ts);
        } 
    }
    return trees;
}


