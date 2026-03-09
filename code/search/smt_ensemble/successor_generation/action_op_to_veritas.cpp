//
// This file is part of the PlaJA code base.
// Copyright (c) (2023 - 2024) Chaahat Jain.
//

#include "action_op_to_veritas.h"
#include "../../../parser/ast/expression/expression.h"
#include "../../successor_generation/update.h"
#include "../../successor_generation/action_op.h"
#include "../visitor/extern/to_veritas_visitor.h"
#include "../constraints_in_veritas.h"
#include "../vars_in_veritas.h"

UpdateToVeritas::UpdateToVeritas(const Update& update_):
    update(update_) { PLAJA_ASSERT(update.get_num_sequences() <= 1) }

UpdateToVeritas::~UpdateToVeritas() = default;

/**********************************************************************************************************************/

std::unordered_set<VariableIndex_type> UpdateToVeritas::collect_updated_state_indexes(bool include_locs) const {
    PLAJA_ASSERT(update.get_num_sequences() <= 1)
    return update.collect_updated_state_indexes(0, include_locs);
}

std::unordered_set<VariableIndex_type> UpdateToVeritas::collect_updating_state_indexes() const {
    PLAJA_ASSERT(update.get_num_sequences() <= 1)
    return update.collect_updating_state_indexes(0);
}

std::unordered_set<VeritasVarIndex_type> UpdateToVeritas::collect_updated_in_veritas(const StateIndexesInVeritas& target_vars, bool include_locs) const {
    std::unordered_set<VeritasVarIndex_type> updated_in_veritas;

    for (auto state_index: collect_updated_state_indexes(include_locs)) {
        PLAJA_ASSERT(target_vars.exists(state_index))
        updated_in_veritas.insert(target_vars.to_veritas(state_index));
    }

    return updated_in_veritas;
}

std::unordered_set<VeritasVarIndex_type> UpdateToVeritas::collect_updating_in_veritas(const StateIndexesInVeritas& src_vars) const {
    std::unordered_set<VeritasVarIndex_type> updating_in_veritas;

    PLAJA_ASSERT(update.get_num_sequences() <= 1)
    for (auto state_index: collect_updating_state_indexes()) {
        PLAJA_ASSERT(src_vars.exists(state_index))
        updating_in_veritas.insert(src_vars.to_veritas(state_index));
    }

    return updating_in_veritas;
}

/**********************************************************************************************************************/

std::unique_ptr<ConjunctionInVeritas> UpdateToVeritas::to_veritas_loc(const StateIndexesInVeritas& src_vars, const StateIndexesInVeritas& target_vars, bool do_copy) const {
    auto assignments = std::make_unique<ConjunctionInVeritas>(target_vars._context());

    for (auto loc_it = update.locationIterator(); !loc_it.end(); ++loc_it) {
        PLAJA_ASSERT(target_vars.exists(loc_it.loc()))
        VERITAS_IN_PLAJA::to_assignment_constraint(target_vars._context(), target_vars.to_veritas(loc_it.loc()), loc_it.dest())->move_to_conjunction(*assignments);
    }

    if (do_copy) {
        to_veritas_loc_copy(src_vars, target_vars)->move_to_conjunction(*assignments);
    }

    return assignments;
}

std::unique_ptr<ConjunctionInVeritas> UpdateToVeritas::to_veritas_loc_copy(const StateIndexesInVeritas& src_vars, const StateIndexesInVeritas& target_vars) const {
    auto assignments = std::make_unique<ConjunctionInVeritas>(target_vars._context());

    for (auto loc: update.infer_non_updated_locs()) {
        PLAJA_ASSERT(target_vars.exists(loc))
        PLAJA_ASSERT(src_vars.exists(loc))
        VERITAS_IN_PLAJA::to_copy_constraint(target_vars._context(), target_vars.to_veritas(loc), src_vars.to_veritas(loc))->move_to_conjunction(*assignments);
    }

    return assignments;
}

std::unique_ptr<ConjunctionInVeritas> UpdateToVeritas::to_veritas_copy(const StateIndexesInVeritas& src_vars, const StateIndexesInVeritas& target_vars, bool do_locs) const {
    auto assignments = std::make_unique<ConjunctionInVeritas>(target_vars._context());

    if (do_locs) {
        to_veritas_loc_copy(src_vars, target_vars)->move_to_conjunction(*assignments);
    }

    for (auto state_index: update.infer_non_updated_state_indexes(0, false)) {
        if (target_vars.exists(state_index)) {
            PLAJA_ASSERT(src_vars.exists(state_index))
            VERITAS_IN_PLAJA::to_copy_constraint(target_vars._context(), target_vars.to_veritas(state_index), src_vars.to_veritas(state_index))->move_to_conjunction(*assignments);
        }
    }

    return assignments;
}

std::unique_ptr<ConjunctionInVeritas> UpdateToVeritas::to_veritas(const StateIndexesInVeritas& src_vars, const StateIndexesInVeritas& target_vars, bool do_locs, bool do_copy) const {
    PLAJA_ASSERT(&src_vars._context() == &target_vars._context()) // assert by ptr rather than class equality for convenience

    auto assignments = std::make_unique<ConjunctionInVeritas>(target_vars._context());

    if (do_locs) { to_veritas_loc(src_vars, target_vars, do_copy)->move_to_conjunction(*assignments); }

    for (auto it = update.assignmentIterator(0); !it.end(); ++it) {

        // update state index
        const VariableIndex_type target_index = it.variable_index();
        PLAJA_ASSERT(target_vars.exists(target_index))

        // assignment expression:
        if (it.is_non_deterministic()) {

            const auto* lb_expression = it.lower_bound();
            const auto* ub_expression = it.upper_bound();
            PLAJA_ASSERT_EXPECT(lb_expression or ub_expression)

            const auto target_var = target_vars.to_veritas(target_index);

            VERITAS_IN_PLAJA::to_lb_constraint(target_var, *lb_expression, src_vars)->move_to_conjunction(*assignments);
            VERITAS_IN_PLAJA::to_ub_constraint(target_var, *ub_expression, src_vars)->move_to_conjunction(*assignments);

        } else {

            const auto* assignment_expression = it.value();
            PLAJA_ASSERT(assignment_expression)
            VERITAS_IN_PLAJA::to_assignment_constraint(target_vars.to_veritas(target_index), *assignment_expression, src_vars)->move_to_conjunction(*assignments);

        }

    }

    if (do_copy) { to_veritas_copy(src_vars, target_vars, false)->move_to_conjunction(*assignments); }

    return assignments;
}

/**********************************************************************************************************************/


ActionOpToVeritas::ActionOpToVeritas(const ActionOp& action_op, bool compute_updates):
    actionOp(action_op) {
    updates.reserve(actionOp.size());
    if (compute_updates) { for (auto update_it = actionOp.updateIterator(); !update_it.end(); ++update_it) { updates.push_back(std::make_unique<UpdateToVeritas>(update_it.update())); } }
}

ActionOpToVeritas::~ActionOpToVeritas() = default;

//

ActionLabel_type ActionOpToVeritas::_action_label() const { return actionOp._action_label(); }

ActionOpID_type ActionOpToVeritas::_op_id() const { return actionOp._op_id(); }

//

std::unordered_set<VeritasVarIndex_type> ActionOpToVeritas::collect_guard_vars_in_veritas(const StateIndexesInVeritas& vars) const {
    std::unordered_set<VeritasVarIndex_type> guard_vars_in_veritas;

    for (auto state_index: actionOp.collect_guard_state_indexes()) {
        PLAJA_ASSERT(vars.exists(state_index))
        guard_vars_in_veritas.insert(vars.to_veritas(state_index));
    }

    return guard_vars_in_veritas;
}

std::unique_ptr<ConjunctionInVeritas> ActionOpToVeritas::guard_to_veritas_loc(const StateIndexesInVeritas& vars) const {
    auto guard = std::make_unique<ConjunctionInVeritas>(vars._context());

    for (auto it = actionOp.locationIterator(); !it.end(); ++it) { guard->add_equality_constraint(it.loc(), it.src()); }

    return guard;
}

std::unique_ptr<ConjunctionInVeritas> ActionOpToVeritas::guard_to_veritas(const StateIndexesInVeritas& vars, bool do_locs) const {
    auto guard = std::make_unique<ConjunctionInVeritas>(vars._context());

    // guard over locs
    if (do_locs) { guard_to_veritas_loc(vars)->move_to_conjunction(*guard); }

    // guard over vars
    for (auto it = actionOp.guardIterator(); !it.end(); ++it) {
        const auto* guard_exp = it();
        PLAJA_ASSERT(guard_exp)
        if (guard_exp->is_constant() and guard_exp->evaluate_integer_const()) { continue; }
        VERITAS_IN_PLAJA::to_veritas_constraint(*guard_exp, vars)->move_to_conjunction(*guard);
    }

    return guard;
}

std::unique_ptr<ConjunctionInVeritas> ActionOpToVeritas::update_to_veritas(UpdateIndex_type update_index, const StateIndexesInVeritas& src_vars, const StateIndexesInVeritas& target_vars, bool do_locs, bool do_copy) const {
    PLAJA_ASSERT(update_index < updates.size())
    return updates[update_index]->to_veritas(src_vars, target_vars, do_locs, do_copy);
}

std::unique_ptr<ConjunctionInVeritas> ActionOpToVeritas::to_veritas(UpdateIndex_type update_index, const StateIndexesInVeritas& src_vars, const StateIndexesInVeritas& target_vars, bool do_locs, bool do_copy) const {
    auto op_in_veritas = guard_to_veritas(src_vars, do_locs);
    update_to_veritas(update_index, src_vars, target_vars, do_locs, do_copy)->move_to_conjunction(*op_in_veritas);
    return op_in_veritas;
}

std::unique_ptr<ConjunctionInVeritas> ActionOpToVeritas::to_veritas(const StateIndexesInVeritas& src_vars, const StateIndexesInVeritas& target_vars, bool do_locs, bool do_copy) const {
    PLAJA_ASSERT(&src_vars._context() == &target_vars._context()) // assert by ptr rather than class equality for convenience
    //
    // guards
    auto op_in_veritas = guard_to_veritas(src_vars, do_locs);
    // updates
    if (updates.size() > 1) {
        std::cout << "Updates cannot have larger than 1 size since disjunction not supported" << std::endl;
        PLAJA_ABORT
        // auto updates_in_z3 = std::make_unique<DisjunctionInVeritas>(src_vars._context());
        // for (const auto& update_to_veritas: updates) { update_to_veritas->to_veritas(src_vars, target_vars, do_locs, do_copy)->move_to_disjunction(*updates_in_z3); }
        // updates_in_z3->move_to_conjunction(*op_in_veritas);
    } else {
        to_veritas(0, src_vars, target_vars, do_locs, do_copy)->move_to_conjunction(*op_in_veritas);
    }
    return op_in_veritas;
}
