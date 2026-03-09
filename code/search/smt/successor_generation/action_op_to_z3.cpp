//
// This file is part of the PlaJA code base.
// Copyright (c) (2019 - 2021) Marcel Vinzent.
// See README.md in the top-level directory for licensing information.
//

#include <z3++.h>
#include "action_op_to_z3.h"
#include "../../../parser/ast/expression/expression.h"
#include "../../successor_generation/action_op.h"
#include "../utils/to_z3_expr_splits.h"
#include "../visitor/extern/to_z3_visitor.h"
#include "../model/model_z3.h"
#include "../context_z3.h"
#include "../constraint_z3.h"
#include "../vars_in_z3.h"
#include "update_to_z3.h"

ActionOpToZ3::ActionOpToZ3(const ActionOp& action_op, const ModelZ3& model_z3, bool compute_updates):
    actionOp(action_op)
    , modelZ3(model_z3) {
    updates.reserve(action_op.size());
    if (compute_updates) { for (auto it = actionOp.updateIterator(); !it.end(); ++it) { updates.push_back(std::unique_ptr<UpdateToZ3>(new UpdateToZ3(it.update(), modelZ3))); } }
}

ActionOpToZ3::~ActionOpToZ3() = default;

/**********************************************************************************************************************/

ActionLabel_type ActionOpToZ3::_action_label() const { return actionOp._action_label(); }

ActionOpID_type ActionOpToZ3::_op_id() const { return actionOp._op_id(); }

FCT_IF_DEBUG(void ActionOpToZ3::dump() const { actionOp.dump(&modelZ3.get_model(), true); })

/**********************************************************************************************************************/

std::unordered_set<VariableID_type> ActionOpToZ3::collect_guard_variables() const { return actionOp.collect_guard_vars(); }

std::unordered_set<VariableID_type> ActionOpToZ3::collect_guard_state_indexes() const { return actionOp.collect_guard_state_indexes(); }

/* */

std::unique_ptr<Z3_IN_PLAJA::Constraint> ActionOpToZ3::guard_to_z3_loc(const Z3_IN_PLAJA::StateVarsInZ3& vars) const {
    auto& c = modelZ3.get_context();
    auto guard_in_z3 = std::make_unique<Z3_IN_PLAJA::ConstraintVec>(modelZ3.get_context_z3());
    for (auto it = actionOp.locationIterator(); !it.end(); ++it) { guard_in_z3->append(vars.loc_to_z3_expr(it.loc()) == c.int_val(it.src())); }
    return guard_in_z3;
}

std::unique_ptr<Z3_IN_PLAJA::Constraint> ActionOpToZ3::guard_to_z3(const Z3_IN_PLAJA::StateVarsInZ3& vars, bool do_locs) const {
    auto& c = modelZ3.get_context();

    auto guard_in_z3 = std::make_unique<Z3_IN_PLAJA::ConstraintVec>(modelZ3.get_context_z3());

    for (auto it = actionOp.guardIterator(); !it.end(); ++it) {
        /* Special case. */
        if (it->is_constant()) {
            if (it->evaluate_integer_const()) { continue; } // TRUE
            else { return c.trivially_false(); } // FALSE
        }
        guard_in_z3->append(Z3_IN_PLAJA::to_z3_condition(*it, vars));
    }

    if (do_locs) { guard_in_z3->append(PLAJA_UTILS::cast_unique<Z3_IN_PLAJA::ConstraintVec>(guard_to_z3_loc(vars))); }
    if (guard_in_z3->operator()().empty()) { return c.trivially_true(); }

    return guard_in_z3;
}

std::unique_ptr<ToZ3ExprSplits> ActionOpToZ3::guard_to_z3_splits(const Z3_IN_PLAJA::StateVarsInZ3& vars) const {
    std::unique_ptr<ToZ3ExprSplits> to_z3_expr_splits(new ToZ3ExprSplits(actionOp.guard_to_splits(), modelZ3.get_context()));
    /* Set splits in z3. */
    for (const auto& split: to_z3_expr_splits->splits) { to_z3_expr_splits->splitsInZ3.push_back(Z3_IN_PLAJA::to_z3_condition(*split, vars)); }
    /* */
    return to_z3_expr_splits;
}

std::unique_ptr<Z3_IN_PLAJA::Constraint> ActionOpToZ3::update_to_z3(UpdateIndex_type update_index, const Z3_IN_PLAJA::StateVarsInZ3& src_vars, const Z3_IN_PLAJA::StateVarsInZ3& target_vars, bool do_locs, bool do_copy) const {
    PLAJA_ASSERT(update_index < actionOp.size())
    return std::make_unique<Z3_IN_PLAJA::ConstraintVec>(updates[update_index]->to_z3(src_vars, target_vars, do_locs, do_copy));
}

std::unique_ptr<Z3_IN_PLAJA::Constraint> ActionOpToZ3::to_z3(UpdateIndex_type update_index, const Z3_IN_PLAJA::StateVarsInZ3& src_vars, const Z3_IN_PLAJA::StateVarsInZ3& target_vars, bool do_locs, bool do_copy) const {
    /* Guard */
    auto guard_in_z3 = guard_to_z3(src_vars, do_locs);
    if (guard_in_z3->is_trivially_false()) { return guard_in_z3; } // False.
    /* Update */
    auto action_op_in_z3 = PLAJA_UTILS::cast_unique<Z3_IN_PLAJA::ConstraintVec>(update_to_z3(update_index, src_vars, target_vars, do_locs, do_copy));
    if (guard_in_z3->is_trivially_true()) { return action_op_in_z3; } // True
    action_op_in_z3->append(PLAJA_UTILS::cast_unique<Z3_IN_PLAJA::ConstraintVec>(std::move(guard_in_z3)));
    return action_op_in_z3;
}

std::unique_ptr<Z3_IN_PLAJA::Constraint> ActionOpToZ3::to_z3(const Z3_IN_PLAJA::StateVarsInZ3& src_vars, const Z3_IN_PLAJA::StateVarsInZ3& target_vars, bool do_locs, bool do_copy) const {
    /* Guard */
    auto guard_in_z3 = guard_to_z3(src_vars, do_locs);
    if (guard_in_z3->is_trivially_false()) { return guard_in_z3; } // False.

    std::vector<z3::expr> updates_in_z3;
    for (const auto& update_to_z3: updates) {
        const z3::expr_vector update_in_z3 = update_to_z3->to_z3(src_vars, target_vars, do_locs, do_copy);
        PLAJA_ASSERT(not update_in_z3.empty())
        updates_in_z3.push_back(Z3_IN_PLAJA::to_conjunction(update_in_z3));
    }

    PLAJA_ASSERT(not updates_in_z3.empty())
    if (guard_in_z3->is_trivially_true()) { return std::make_unique<Z3_IN_PLAJA::ConstraintExpr>(Z3_IN_PLAJA::to_disjunction(updates_in_z3)); }
    return std::make_unique<Z3_IN_PLAJA::ConstraintExpr>(Z3_IN_PLAJA::to_conjunction(PLAJA_UTILS::cast_ref<Z3_IN_PLAJA::ConstraintVec>(*guard_in_z3).operator()()) && Z3_IN_PLAJA::to_disjunction(updates_in_z3));
}
