//
// This file is part of the PlaJA code base.
// Copyright (c) (2019 - 2022) Marcel Vinzent.
// See README.md in the top-level directory for licensing information.
//

#ifndef PLAJA_ACTION_OP_MARABOU_H
#define PLAJA_ACTION_OP_MARABOU_H

#include "action_op_to_marabou.h"
#include "../../../utils/utils.h"
#include "../forward_smt_nn.h"

struct UpdateMarabou: public UpdateToMarabou {

protected:
    const ModelMarabou& model;
    std::unique_ptr<ConjunctionInMarabou> assignments;

    // void set_additional_updating_vars(ModelMarabou& model_marabou);
    inline void step_updates() {}

public:
    UpdateMarabou(const Update& update_, const ModelMarabou& model_);
    ~UpdateMarabou() override;
    DELETE_CONSTRUCTOR(UpdateMarabou)

    virtual void generate_steps();
    virtual void increment();

    void add(MARABOU_IN_PLAJA::QueryConstructable& query, bool do_locs, bool do_copy, StepIndex_type step = 0) const;

    [[nodiscard]] std::unique_ptr<ConjunctionInMarabou> to_marabou(bool do_locs, bool do_copy, StepIndex_type step) const;

};

/**********************************************************************************************************************/


class ActionOpMarabou: public ActionOpToMarabou {

protected:
    const ModelMarabou& model;
    std::unique_ptr<ConjunctionInMarabou> guards;
    std::unique_ptr<MarabouConstraint> guardNegations;

    // void set_additional_guard_vars(ModelMarabou& model_marabou);
    inline void step_guards() {}

public:
    ActionOpMarabou(const ActionOp& action_op, const ModelMarabou& model_, bool compute_updates = true);
    ~ActionOpMarabou() override;
    DELETE_CONSTRUCTOR(ActionOpMarabou)

    void generate_steps();
    void increment();

    [[nodiscard]] inline const UpdateMarabou& get_update_marabou(UpdateIndex_type index) const { return PLAJA_UTILS::cast_ref<UpdateMarabou>(get_update(index)); }

    /** @return true to indicate whether query was modified. */
    bool add_guard(MARABOU_IN_PLAJA::QueryConstructable& query, bool do_locs, StepIndex_type step = 0) const;

    /** @return false if query has become trivially unsat (i.e., guard negation is false). */
    bool add_guard_negation(MARABOU_IN_PLAJA::QueryConstructable& query, bool do_locs, StepIndex_type step = 0) const;

    inline void add_update(MARABOU_IN_PLAJA::QueryConstructable& query, UpdateIndex_type index, bool do_locs, bool do_copy, StepIndex_type step = 0) const { get_update_marabou(index).add(query, do_locs, do_copy, step); }

    [[nodiscard]] std::unique_ptr<ConjunctionInMarabou> guard_to_marabou(bool do_locs, StepIndex_type step = 0) const;

    [[nodiscard]] std::unique_ptr<MarabouConstraint> guard_negation_to_marabou(bool do_locs, StepIndex_type step = 0) const;

    /** @return true if entailed. */
    [[nodiscard]] std::pair<std::unique_ptr<MarabouConstraint>, bool> guard_to_marabou(const MARABOU_IN_PLAJA::QueryConstructable& query, bool do_locs, StepIndex_type step = 0) const;
    [[nodiscard]] std::pair<std::unique_ptr<MarabouConstraint>, bool> guard_negation_to_marabou(const MARABOU_IN_PLAJA::QueryConstructable& query, bool do_locs, StepIndex_type step = 0) const;
};

#endif //PLAJA_ACTION_OP_MARABOU_H
