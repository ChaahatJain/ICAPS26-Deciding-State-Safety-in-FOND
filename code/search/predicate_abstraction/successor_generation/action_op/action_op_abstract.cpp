//
// This file is part of the PlaJA code base.
// Copyright (c) (2019 - 2020) Marcel Vinzent.
// See README.md in the top-level directory for licensing information.
//

#include "action_op_abstract.h"
#include "../../../successor_generation/action_op.h"
#include "../../../smt/visitor/extern/to_z3_visitor.h"
#include "../../../smt/solver/smt_solver_z3.h"
#include "../../../smt/model/model_z3.h"
#include "../../../smt/constraint_z3.h"

ActionOpAbstract::ActionOpAbstract(const ActionOp& parent, const ModelZ3& model_z3, bool initialize, PLAJA::StatsBase* shared_stats):
    ActionOpToZ3(parent, model_z3, false)
    , guards(guard_to_z3(get_model_z3().get_src_vars(), false)->move_to_expr())
    /* , opInZ3(get_model_z3().get_context_z3()) */ {

    const auto op_constants = actionOp.collect_op_constants();
    opConstants = std::vector<ConstantIdType>(op_constants.cbegin(), op_constants.cend());

    const auto guard_vars = actionOp.collect_guard_vars();
    guardVars = std::vector<VariableID_type>(guard_vars.cbegin(), guard_vars.cend());

    Z3_IN_PLAJA::SMTSolver solver(model_z3.share_context(), shared_stats);

    solver.push();

    optimize_guards(solver); // Optimize guards

    if (initialize) {

        compute_updates(); // updates (modified in derived class)

        // compute_op_in_z3(); // Cache operator encoding in z3.

    }

    /* Finalize. */
    solver.pop();

}

ActionOpAbstract::~ActionOpAbstract() = default;

/* */

void ActionOpAbstract::optimize_guards(Z3_IN_PLAJA::SMTSolver& solver) {
    if (Z3_IN_PLAJA::expr_is(guards)) {
        solver.add(guards);
        if (not solver.check()) { guards = guards.ctx().bool_val(false); } // Set guard trivially false.
    }
}

void ActionOpAbstract::compute_updates() {
    updates.reserve(actionOp.size());
    for (auto it = actionOp.updateIterator(); !it.end(); ++it) { updates.push_back(std::make_unique<UpdateAbstract>(it.update(), get_model_z3())); }
}

#if 0
void ActionOpAbstract::compute_op_in_z3() {
    PLAJA_ASSERT(Z3_IN_PLAJA::expr_is_none(opInZ3))
    PLAJA_ASSERT(not updates.empty()) // Sanity check.
    // Guards.
    if (Z3_IN_PLAJA::expr_is(guards)) { opInZ3 = guards; }
    // Updates.
    z3::expr disjunction_of_updates(opInZ3.ctx());
    for (const auto& update: updates) {
        const z3::expr assignment = Z3_IN_PLAJA::to_conjunction(PLAJA_UTILS::cast_ref<UpdateAbstract>(*update)._assignments());
        Z3_EXPR_ADD_IF(disjunction_of_updates, assignment, ||)
    }
    Z3_EXPR_ADD_IF(opInZ3, disjunction_of_updates, &&)
}
#endif

/* */

void ActionOpAbstract::add_src_bounds(Z3_IN_PLAJA::SMTSolver& solver) const {
    const auto& model_z3 = get_model_z3();
    for (const auto constant: opConstants) { model_z3.register_constant(solver, constant); }
    for (const auto var: guardVars) { model_z3.register_src_bound(solver, var); }
}

void ActionOpAbstract::add_to_solver(Z3_IN_PLAJA::SMTSolver& solver) const {
    add_src_bounds(solver);
    solver.add_if(guards);
}

bool ActionOpAbstract::guard_enabled(Z3_IN_PLAJA::SMTSolver& solver) const {
    solver.push();
    add_to_solver(solver);
    return solver.check_pop();
}

bool ActionOpAbstract::guard_enabled_inc(Z3_IN_PLAJA::SMTSolver& solver) const {
    solver.push();
    add_to_solver(solver);
    return solver.check();
}

bool ActionOpAbstract::guard_entailed(Z3_IN_PLAJA::SMTSolver& solver) const {
    if (not Z3_IN_PLAJA::expr_is(guards)) { return true; } // Trivially true.
    solver.push();
    add_src_bounds(solver);
    solver.add(!guards);
    return not solver.check_pop();
}