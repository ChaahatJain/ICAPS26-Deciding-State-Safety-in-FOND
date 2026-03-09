//
// This file is part of the PlaJA code base.
// Copyright (c) (2019 - 2021) Marcel Vinzent.
// See README.md in the top-level directory for licensing information.
//

#ifndef PLAJA_SUCCESSOR_GENERATOR_TO_Z3_H
#define PLAJA_SUCCESSOR_GENERATOR_TO_Z3_H

#include <list>
#include <memory>
#include "../../../utils/default_constructors.h"
#include "../../information/forward_information.h"
#include "../../successor_generation/forward_successor_generation.h"
#include "../../using_search.h"
#include "../forward_smt_z3.h"
#include "../using_smt.h"

class SuccessorGeneratorToZ3 {

private:
    const ModelZ3* modelSmt;
    const ModelInformation* modelInformation;
    const SuccessorGeneratorC* successorGenerator;
    std::vector<std::unique_ptr<ActionOpToZ3>> nonSyncOps;
    std::vector<std::vector<std::unique_ptr<ActionOpToZ3>>> syncOps;

    std::list<const ActionOpToZ3*> silentOps;
    std::vector<std::list<const ActionOpToZ3*>> labeledOps;

    /* Internal aux. */
    void compute_action_structure(ActionLabel_type action_label);

public:
    explicit SuccessorGeneratorToZ3(const ModelZ3& model_z3);
    ~SuccessorGeneratorToZ3();
    DELETE_CONSTRUCTOR(SuccessorGeneratorToZ3)

    [[nodiscard]] inline const ModelZ3& get_model_smt() const { return *modelSmt; }

    [[nodiscard]] const ActionOpToZ3& get_action_op_structure(ActionOpID_type action_op_id) const;
    [[nodiscard]] const std::list<const ActionOpToZ3*>& get_action_structure(ActionLabel_type action_label) const;

    [[nodiscard]] std::unique_ptr<Z3_IN_PLAJA::Constraint> generate_action_op(ActionOpID_type action_op_id, const Z3_IN_PLAJA::StateVarsInZ3& src_vars, const Z3_IN_PLAJA::StateVarsInZ3& target_vars) const;
    [[nodiscard]] std::unique_ptr<Z3_IN_PLAJA::Constraint> generate_guards(ActionLabel_type label, StepIndex_type step) const;

    [[nodiscard]] std::unique_ptr<Z3_IN_PLAJA::Constraint> generate_action(const std::list<const ActionOpToZ3*>& action_structure, const Z3_IN_PLAJA::StateVarsInZ3& src_vars, const Z3_IN_PLAJA::StateVarsInZ3& target_vars) const;
    [[nodiscard]] std::unique_ptr<Z3_IN_PLAJA::Constraint> generate_action(ActionLabel_type action_label, const Z3_IN_PLAJA::StateVarsInZ3& src_vars, const Z3_IN_PLAJA::StateVarsInZ3& target_vars) const;

    [[nodiscard]] std::unique_ptr<Z3_IN_PLAJA::Constraint> generate_expansion(const Z3_IN_PLAJA::StateVarsInZ3& src_vars, const Z3_IN_PLAJA::StateVarsInZ3& target_vars) const;

};

#endif //PLAJA_SUCCESSOR_GENERATOR_TO_Z3_H
