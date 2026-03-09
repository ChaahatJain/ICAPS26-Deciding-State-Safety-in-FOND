//
// This file is part of the PlaJA code base.
// Copyright (c) (2019 - 2023) Marcel Vinzent.
// See README.md in the top-level directory for licensing information.
//

#include "smt_based_restrictions_checker.h"
#include "../../../exception/semantics_exception.h"
#include "../../../parser/ast/assignment.h"
#include "../../../parser/ast/model.h"
#include "../../../parser/ast/variable_declaration.h"
#include "../../information/model_information.h"
#include "../../factories/configuration.h"
#include "../model/model_z3.h"
#include "../solver/smt_solver_z3.h"
#include "../utils/to_z3_expr.h"
#include "../context_z3.h"
#include "extern/to_z3_visitor.h"

SmtBasedRestrictionsChecker::SmtBasedRestrictionsChecker(const PLAJA::Configuration& config):
    modelZ3(nullptr) {

    PLAJA_ASSERT(config.has_sharable(PLAJA::SharableKey::MODEL_Z3))
    modelZ3 = config.get_sharable<ModelZ3>(PLAJA::SharableKey::MODEL_Z3).get();

    // prepare solver
    solver = std::make_unique<Z3_IN_PLAJA::SMTSolver>(modelZ3->share_context(), config.get_sharable_ptr<PLAJA::StatsBase>(PLAJA::SharableKey::STATS));
    modelZ3->register_bounds(*solver, 0, false);

}

SmtBasedRestrictionsChecker::~SmtBasedRestrictionsChecker() = default;

void SmtBasedRestrictionsChecker::visit(const Assignment* assignment) {

    /* MV:
     * If we drop any of these restrictions,
     * then we must handle it explicitly during abstract path spuriousness check in CEGAR.
     */

    if (assignment->is_non_deterministic()) {

        JANI_ASSERT(assignment->get_lower_bound())
        JANI_ASSERT(assignment->get_upper_bound())

        const auto& model_info = modelZ3->get_model_info();
        const auto* var_decl = modelZ3->get_model().get_variable_by_id(assignment->get_updated_var_id());
        const auto state_index = var_decl->get_index();

        const auto lb_to_z3 = modelZ3->to_z3(*assignment->get_lower_bound(), 0);
        const auto ub_to_z3 = modelZ3->to_z3(*assignment->get_upper_bound(), 0);

        // lower bound >= domain lower bound ?
        solver->push();

        if (lb_to_z3.has_auxiliary()) { solver->add(lb_to_z3.auxiliary); }

        solver->push();

        if (model_info.is_integer(state_index)) {
            solver->add(lb_to_z3.primary < modelZ3->get_context().int_val(model_info.get_lower_bound_int(state_index)));
        } else {
            solver->add(lb_to_z3.primary < modelZ3->get_context().real_val(model_info.get_lower_bound_float(state_index), Z3_IN_PLAJA::floatingPrecision));
        }

        if (solver->check_pop()) {

            PLAJA_LOG(PLAJA_UTILS::to_red_string("Warning: Non-deterministic assignment might allow for out-of-bounds."))
            assignment->dump();

            // lower bound <= domain upper bound (at least) ?
            if (model_info.is_integer(state_index)) {
                solver->add(lb_to_z3.primary > modelZ3->get_context().int_val(model_info.get_upper_bound_int(state_index)));
            } else {
                solver->add(lb_to_z3.primary > modelZ3->get_context().real_val(model_info.get_upper_bound_float(state_index), Z3_IN_PLAJA::floatingPrecision));
            }

            if (solver->check()) { throw SemanticsException(assignment->to_string()); }

        }

        solver->pop();

        // upper bound  <= domain upper bound ?
        solver->push();

        if (ub_to_z3.has_auxiliary()) { solver->add(ub_to_z3.auxiliary); }

        solver->push();

        if (model_info.is_integer(state_index)) {
            solver->add(ub_to_z3.primary > modelZ3->get_context().int_val(model_info.get_upper_bound_int(state_index)));
        } else {
            solver->add(ub_to_z3.primary > modelZ3->get_context().real_val(model_info.get_upper_bound_float(state_index), Z3_IN_PLAJA::floatingPrecision));
        }

        if (solver->check_pop()) {

            PLAJA_LOG(PLAJA_UTILS::to_red_string("Warning: Non-deterministic assignment might allow for out-of-bounds."))
            assignment->dump();

            // upper bound >= domain lower bound (at least) ?
            if (model_info.is_integer(state_index)) {
                solver->add(ub_to_z3.primary < modelZ3->get_context().int_val(model_info.get_lower_bound_int(state_index)));
            } else {
                solver->add(ub_to_z3.primary < modelZ3->get_context().real_val(model_info.get_lower_bound_float(state_index), Z3_IN_PLAJA::floatingPrecision));
            }

            if (solver->check()) { throw SemanticsException(assignment->to_string()); }

        }

        solver->pop();

        // lower bound <= upper bound ?
        solver->push();

        if (lb_to_z3.has_auxiliary()) { solver->add(lb_to_z3.auxiliary); }
        if (ub_to_z3.has_auxiliary()) { solver->add(ub_to_z3.auxiliary); }
        solver->add(lb_to_z3.primary > ub_to_z3.primary);

        if (solver->check()) { throw SemanticsException(assignment->to_string()); }

        solver->pop();

        // lower bound <= var <= upper bound is always satisfiable, i.e., for any state there would exist a successor ?
        // That is there does not exist a state such that for all domain values we have var < lower bound or upper bound < var.
        solver->push();

        if (lb_to_z3.has_auxiliary()) { solver->add(lb_to_z3.auxiliary); }
        if (ub_to_z3.has_auxiliary()) { solver->add(ub_to_z3.auxiliary); }
        const auto& aux_var = modelZ3->get_var(var_decl->get_id(), 1);
        solver->add(z3::forall(aux_var, aux_var < lb_to_z3.primary || ub_to_z3.primary < aux_var));

        if (solver->check()) { throw SemanticsException(assignment->to_string()); }

        solver->pop();

    }

}

namespace SMT_BASED_RESTRICTIONS_CHECKER {

    void check(const AstElement& ast_element, const PLAJA::Configuration& config) {
        SmtBasedRestrictionsChecker smt_based_restrictions_checker(config);
        ast_element.accept(&smt_based_restrictions_checker);
    }

}