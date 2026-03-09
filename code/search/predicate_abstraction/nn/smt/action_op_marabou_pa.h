//
// This file is part of the PlaJA code base.
// Copyright (c) (2019 - 2020) Marcel Vinzent.
// See README.md in the top-level directory for licensing information.
//

#ifndef PLAJA_ACTION_OP_MARABOU_PA_H
#define PLAJA_ACTION_OP_MARABOU_PA_H

#include "../../../using_search.h"
#include "../../using_predicate_abstraction.h"
#include "../../../smt_nn/successor_generation/action_op_marabou.h"
#include "../../../smt_nn/forward_smt_nn.h"
#include "../../pa_states/forward_pa_states.h"

class ActionOpPA;

class ModelMarabouPA;

class UpdatePA;

/**********************************************************************************************************************/

struct UpdateMarabouPA final: public UpdateMarabou {

private:
    std::vector<std::unique_ptr<PredicateConstraint>> predicatesInMarabou;

    void step_predicates(bool reset);

public:
    UpdateMarabouPA(const Update& parent_, const ModelMarabouPA& model_);
    ~UpdateMarabouPA() override;
    DELETE_CONSTRUCTOR(UpdateMarabouPA)

    [[nodiscard]] const ModelMarabouPA& get_model_marabou_pa() const;

    void generate_steps() override;

    void increment() override;

    void add_target_predicate_state(const AbstractState& target, MARABOU_IN_PLAJA::QueryConstructable& query) const;
};

/**********************************************************************************************************************/

class ActionOpMarabouPA final: public ActionOpMarabou {

public:
    ActionOpMarabouPA(const ActionOp& parent_, const ModelMarabouPA& model_);
    ~ActionOpMarabouPA() override;
    DELETE_CONSTRUCTOR(ActionOpMarabouPA)

    [[nodiscard]] inline const UpdateMarabouPA& get_update_pa(UpdateIndex_type index) const { return static_cast<const UpdateMarabouPA&>(get_update(index)); } // NOLINT(cppcoreguidelines-pro-type-static-cast-downcast)

    void add_target_predicate_state(const AbstractState& target, MARABOU_IN_PLAJA::QueryConstructable& query, UpdateIndex_type index) const { get_update_pa(index).add_target_predicate_state(target, query); }

};

#endif //PLAJA_ACTION_OP_MARABOU_PA_H
