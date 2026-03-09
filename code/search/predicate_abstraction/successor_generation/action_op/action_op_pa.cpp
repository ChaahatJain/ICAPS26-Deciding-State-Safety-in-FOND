//
// This file is part of the PlaJA code base.
// Copyright (c) (2019 - 2021) Marcel Vinzent.
// See README.md in the top-level directory for licensing information.
//

#include "action_op_pa.h"
#include "../../../successor_generation/action_op.h"
#include "../../../smt/solver/smt_solver_z3.h"
#include "../../smt/model_z3_pa.h"
#include "op_inc_data_pa.h"

ActionOpPA::ActionOpPA(const ActionOp& parent_, const ModelZ3PA& model_z3, Z3_IN_PLAJA::SMTSolver& solver):
    ActionOpAbstract(parent_, model_z3, false, solver.share_statistics()) {
    solver.push();
    add_src_bounds(solver);

    compute_updates(solver); // updates (modified in derived class)

    // compute_op_in_z3(); // Cache operator encoding in z3.

    /* Finalize. */
    solver.pop();
}

ActionOpPA::~ActionOpPA() = default;

const ModelZ3PA& ActionOpPA::get_model_z3_pa() const { return static_cast<const ModelZ3PA&>(modelZ3); } // NOLINT(cppcoreguidelines-pro-type-static-cast-downcast)

void ActionOpPA::compute_updates(Z3_IN_PLAJA::SMTSolver& solver) {
    updates.reserve(actionOp.size());
    for (auto it = actionOp.updateIterator(); !it.end(); ++it) { updates.push_back(std::make_unique<UpdatePA>(it.update(), get_model_z3_pa(), solver)); }
}

void ActionOpPA::increment(const OpIncDataPa& inc_data) {
    auto& solver = inc_data.get_solver();

    solver.push();
    add_to_solver(solver);

    for (auto& update: updates) { PLAJA_UTILS::cast_ref<UpdatePA>(*update).increment(solver); }

    /* Check for incremental dynamic entailment computation. */
    if constexpr (not PLAJA_GLOBAL::lazyPA) {

        const auto pred_index_max = get_model_z3_pa().get_number_predicates() - 1;

        PLAJA_ASSERT(not inc_data.get_vars_in_newly_added_preds().empty())

        if (std::any_of(_guards_vars().cbegin(), _guards_vars().cend(), [&inc_data](VariableID_type var) { return inc_data.get_vars_in_newly_added_preds().count(var); })) {
            for (const auto& update: updates) { PLAJA_UTILS::cast_ref<UpdatePA>(*update).lastAffectingPred = pred_index_max; }
        } else {

            for (const auto& update: updates) {
                auto& update_pa = PLAJA_UTILS::cast_ref<UpdatePA>(*update);
                if (std::any_of(update_pa._updating_vars().cbegin(), update_pa._updating_vars().cend(), [&inc_data](VariableID_type var) { return inc_data.get_vars_in_newly_added_preds().count(var); })) {
                    update_pa.lastAffectingPred = pred_index_max;
                }
            }

        }

    }

    solver.pop();
}
