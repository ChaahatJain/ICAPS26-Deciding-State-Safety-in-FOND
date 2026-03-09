//
// This file is part of the PlaJA code base.
// Copyright (c) (2019 - 2021) Marcel Vinzent.
// See README.md in the top-level directory for licensing information.
//

#include "successor_generator_pa.h"
#include "../../successor_generation/successor_generator_c.h"
#include "../../smt/solver/smt_solver_z3.h"
#include "../smt/model_z3_pa.h"
#include "../predicate_dependency_graph.h"
#include "action_op/action_op_pa.h"
#include "action_op/op_inc_data_pa.h"

SuccessorGeneratorPA::SuccessorGeneratorPA(const ModelZ3& model_Z3, const PLAJA::Configuration& config):
    SuccessorGeneratorAbstract(model_Z3, config)
    , predDepGraph(PLAJA_GLOBAL::lazyPA ? nullptr : new PredicateDependencyGraph(model_Z3))
    , lastNumPreds(model_Z3.get_number_predicates()) {
    UpdatePA::set_flags(); // set optimization flags within update structures
}

SuccessorGeneratorPA::~SuccessorGeneratorPA() = default;

std::unique_ptr<SuccessorGeneratorPA> SuccessorGeneratorPA::construct(const ModelZ3PA& model_z3_pa, const PLAJA::Configuration& config) {
    std::unique_ptr<SuccessorGeneratorPA> successor_generator(new SuccessorGeneratorPA(model_z3_pa, config));

    successor_generator->initialize_edges();

    successor_generator->initialize_action_ops_pa();

    return successor_generator;
}

/* */

const ModelZ3PA& SuccessorGeneratorPA::get_model_z3_pa() const { return PLAJA_UTILS::cast_ref<ModelZ3PA>(get_model_z3()); }

/* */

void SuccessorGeneratorPA::initialize_action_ops_pa() {

    const auto& model_z3 = get_model_z3_pa();
    Z3_IN_PLAJA::SMTSolver solver(model_z3.share_context(), share_stats());

    // non sync ops
    nonSyncOps.clear();
    nonSyncOps.reserve(parent->nonSyncOps.size());
    for (const auto& non_sync_op: parent->nonSyncOps) { nonSyncOps.push_back(std::make_unique<ActionOpPA>(*non_sync_op, model_z3, solver)); }

    // sync operators
    syncOps.clear();
    syncOps.reserve(parent->syncOps.size());
    for (const auto& sync_ops_per_label: parent->syncOps) {
        syncOps.emplace_back();
        auto& sync_ops_per_label_abstract = syncOps.back();
        sync_ops_per_label_abstract.reserve(sync_ops_per_label.size());
        for (const auto& sync_op: sync_ops_per_label) { sync_ops_per_label_abstract.push_back(std::make_unique<ActionOpPA>(*sync_op, model_z3, solver)); }
    }
}

/* */

void SuccessorGeneratorPA::increment() {

    const auto& model_z3 = get_model_z3_pa();
    PLAJA_ASSERT(lastNumPreds <= model_z3.get_number_predicates())

    Z3_IN_PLAJA::SMTSolver solver(model_z3.share_context(), share_stats());
    OpIncDataPa op_inc_data(solver);

    /* Some variable information used by entailment opts. */
    if constexpr (not PLAJA_GLOBAL::lazyPA) {

        predDepGraph->increment();

        const auto num_preds = model_z3.get_number_predicates();
        for (PredicateIndex_type pred_index = lastNumPreds; pred_index < num_preds; ++pred_index) {
            predDepGraph->compute_variable_closure(op_inc_data.access_vars_in_newly_added_preds(), pred_index);
        }
    }

    for (auto& non_sync_op: nonSyncOps) { PLAJA_UTILS::cast_ref<ActionOpPA>(*non_sync_op).increment(op_inc_data); }

    for (auto& sync_ops_per_label: syncOps) {
        for (auto& sync_op: sync_ops_per_label) {
            PLAJA_UTILS::cast_ref<ActionOpPA>(*sync_op).increment(op_inc_data);
        }
    }

    lastNumPreds = model_z3.get_number_predicates();
}

/* */

const ActionOpPA& SuccessorGeneratorPA::get_action_op_pa(ActionOpID_type action_op_id) const { return PLAJA_UTILS::cast_ref<ActionOpPA>(get_action_op(action_op_id)); }


/**********************************************************************************************************************/

namespace SUCCESSOR_GENERATOR_PA {

    [[nodiscard]] extern std::list<const ActionOpPA*> extract_ops_per_label(const SuccessorGeneratorPA& successor_generator, ActionLabel_type action_label) {
        std::list<const ActionOpPA*> ops_per_label;
        for (const auto* op: successor_generator.extract_ops_per_label(action_label)) { ops_per_label.push_back(PLAJA_UTILS::cast_ptr<ActionOpPA>(op)); }
        return ops_per_label;
    }

};