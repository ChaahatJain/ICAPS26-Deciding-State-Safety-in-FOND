//
// This file is part of the PlaJA code base.
// Copyright (c) (2023 - 2024) Chaahat Jain.
//

#ifndef PLAJA_ACTION_OP_VERITAS_PA_H
#define PLAJA_ACTION_OP_VERITAS_PA_H

#include "../../../using_search.h"
#include "../../using_predicate_abstraction.h"
#include "../../../smt_ensemble/successor_generation/action_op_veritas.h"
#include "../../../smt_ensemble/forward_smt_veritas.h"
#include "../../pa_states/forward_pa_states.h"

class ActionOpPA;

class ModelVeritasPA;

class UpdatePA;

/**********************************************************************************************************************/

struct UpdateVeritasPA final: public UpdateVeritas {

private:
    std::vector<std::unique_ptr<VERITAS_IN_PLAJA::PredicateConstraint>> predicatesInVeritas;

    void step_predicates(bool reset);

public:
    UpdateVeritasPA(const Update& parent_, const ModelVeritasPA& model_);
    ~UpdateVeritasPA() override;
    DELETE_CONSTRUCTOR(UpdateVeritasPA)

    [[nodiscard]] const ModelVeritasPA& get_model_veritas_pa() const;

    void generate_steps() override;

    void increment() override;

    void add_target_predicate_state(const AbstractState& target, VERITAS_IN_PLAJA::QueryConstructable& query) const;
};

/**********************************************************************************************************************/

class ActionOpVeritasPA final: public ActionOpVeritas {

public:
    ActionOpVeritasPA(const ActionOp& parent_, const ModelVeritasPA& model_);
    ~ActionOpVeritasPA() override;
    DELETE_CONSTRUCTOR(ActionOpVeritasPA)

    [[nodiscard]] inline const UpdateVeritasPA& get_update_pa(UpdateIndex_type index) const { return static_cast<const UpdateVeritasPA&>(get_update(index)); } // NOLINT(cppcoreguidelines-pro-type-static-cast-downcast)

    void add_target_predicate_state(const AbstractState& target, VERITAS_IN_PLAJA::QueryConstructable& query, UpdateIndex_type index) const { get_update_pa(index).add_target_predicate_state(target, query); }

};

#endif //PLAJA_ACTION_OP_VERITAS_PA_H
