//
// This file is part of the PlaJA code base.
// Copyright (c) (2019 - 2021) Marcel Vinzent.
// See README.md in the top-level directory for licensing information.
//

#ifndef PLAJA_UPDATE_ABSTRACT_H
#define PLAJA_UPDATE_ABSTRACT_H

#include <vector>
#include <unordered_set>
#include "../../../using_search.h"
#include "../../using_predicate_abstraction.h"
#include "../../../smt/successor_generation/update_to_z3.h"

class ModelZ3;

class State;

namespace UPDATE_ABSTRACT { struct UpdateInZ3; }

class UpdateAbstract: public UpdateToZ3 {

protected:
    // finalization flags
    static bool useCopyAssignments;
    static bool addTargetBounds;

    // Z3 cache:
    std::unique_ptr<UPDATE_ABSTRACT::UpdateInZ3> updateInZ3;

public:

    /**
     * @param parent_
     * @param solver asserts guard of the underlying action operator (as well as relevant variable bounds).
     */
    explicit UpdateAbstract(const Update& parent_, const ModelZ3& model);
    ~UpdateAbstract() override;
    DELETE_CONSTRUCTOR(UpdateAbstract)

    [[nodiscard]] const std::vector<VariableID_type>& _updating_vars() const;
    [[nodiscard]] const std::vector<VariableID_type>& _updated_vars() const;
    [[nodiscard]] const ModelZ3& get_model_z3() const;

    static void set_flags(bool use_copy_assignments, bool add_target_bounds);

    [[nodiscard]] const z3::expr_vector& _assignments() const;

    /**
     * Set the location values in the target state according to this update.
     * @param target
     */
    void evaluate_locations(StateValues& loc_target) const;

    /**
     * Adds static structures to solver.
     * @param solver
     */
    void add_to_solver(Z3_IN_PLAJA::SMTSolver& solver) const;

};

#endif //PLAJA_UPDATE_ABSTRACT_H
