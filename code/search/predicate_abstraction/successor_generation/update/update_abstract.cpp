//
// This file is part of the PlaJA code base.
// Copyright (c) (2019 - 2021) Marcel Vinzent.
// See README.md in the top-level directory for licensing information.
//

#include "update_abstract.h"
#include "../../../smt/model/model_z3.h"
#include "../../../smt/solver/smt_solver_z3.h"
#include "../../../successor_generation/update.h"

/**********************************************************************************************************************/

namespace UPDATE_ABSTRACT {

    struct UpdateInZ3 {
        std::vector<VariableID_type> updatingVars;
        std::vector<VariableID_type> updatedVars;
        z3::expr_vector assignments; // possibly including for each assigned array an expr stating its unused/unassigned target indexes must be equal to the source array

        // for readability, i.p. in copy constructor
        [[nodiscard]] inline z3::context& cxt() const { return assignments.ctx(); }

        explicit UpdateInZ3(z3::context& c):
            assignments(c) {}

        ~UpdateInZ3() = default;
        DELETE_CONSTRUCTOR(UpdateInZ3)

    };

}

/**********************************************************************************************************************/

// class flags
bool UpdateAbstract::useCopyAssignments = true;
bool UpdateAbstract::addTargetBounds = false;

UpdateAbstract::UpdateAbstract(const Update& parent_, const ModelZ3& model):
    UpdateToZ3(parent_, model, useCopyAssignments)
    , updateInZ3(new UPDATE_ABSTRACT::UpdateInZ3(model.get_context_z3())) {

    const auto& model_z3 = get_model_z3();

    // cache source variables
    const auto updating_vars = collect_updating_variables();
    updateInZ3->updatingVars = std::vector<VariableID_type>(updating_vars.cbegin(), updating_vars.cend());

    // cache target variables
    for (auto it = update.assignmentIterator(0); !it.end(); ++it) { updateInZ3->updatedVars.push_back(it.var()); }

    // compute assignments (copy assignments will be handled in base class)
    updateInZ3->assignments = to_z3(model_z3.get_src_vars(), model_z3.get_target_vars(), false, useCopyAssignments);

}

UpdateAbstract::~UpdateAbstract() = default;

//

const std::vector<VariableID_type>& UpdateAbstract::_updating_vars() const { return updateInZ3->updatingVars; }

const std::vector<VariableID_type>& UpdateAbstract::_updated_vars() const { return updateInZ3->updatedVars; }

const ModelZ3& UpdateAbstract::get_model_z3() const { return static_cast<const ModelZ3&>(_model_z3()); } // NOLINT(cppcoreguidelines-pro-type-static-cast-downcast)


//

void UpdateAbstract::set_flags(bool use_copy_assignments, bool add_target_bounds) {
    useCopyAssignments = use_copy_assignments;
    addTargetBounds = add_target_bounds;
}

//

const z3::expr_vector& UpdateAbstract::_assignments() const { return updateInZ3->assignments; }

void UpdateAbstract::evaluate_locations(StateValues& loc_target) const { update.evaluate_locations(&loc_target); }

void UpdateAbstract::add_to_solver(Z3_IN_PLAJA::SMTSolver& solver) const {
    for (const auto var_id: updateInZ3->updatingVars) { get_model_z3().register_src_bound(solver, var_id); }
    if (addTargetBounds) { for (const auto var_id: updateInZ3->updatedVars) { get_model_z3().register_target_bound(solver, var_id); } }
    solver.add(updateInZ3->assignments);
}
