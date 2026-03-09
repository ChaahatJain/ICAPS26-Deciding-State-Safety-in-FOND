//
// This file is part of the PlaJA code base.
// Copyright (c) (2019 - 2021) Marcel Vinzent.
// See README.md in the top-level directory for licensing information.
//

#ifndef PLAJA_ACTION_OP_PA_H
#define PLAJA_ACTION_OP_PA_H

#include "../update/update_pa.h"
#include "action_op_abstract.h"

class ActionOpPA final: public ActionOpAbstract {

private:
    void compute_updates(Z3_IN_PLAJA::SMTSolver& solver);

public:
    ActionOpPA(const ActionOp& parent, const ModelZ3PA& model_z3, Z3_IN_PLAJA::SMTSolver& solver);
    ~ActionOpPA() override;
    DELETE_CONSTRUCTOR(ActionOpPA)

    [[nodiscard]] const ModelZ3PA& get_model_z3_pa() const;

    void increment(const OpIncDataPa& inc_data);

    [[nodiscard]] inline const UpdatePA& get_update_pa(UpdateIndex_type index) const { return static_cast<const UpdatePA&>(get_update(index)); } // NOLINT(cppcoreguidelines-pro-type-static-cast-downcast)

    using UpdateIterator = UpdateIteratorBase<UpdateToZ3, UpdatePA>;

    [[nodiscard]] inline UpdateIterator updateIteratorPA() const { return UpdateIterator(updates); }

};

#endif //PLAJA_ACTION_OP_PA_H
