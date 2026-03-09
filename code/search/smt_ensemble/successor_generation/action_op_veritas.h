//
// This file is part of the PlaJA code base.
// Copyright (c) (2023 - 2024) Chaahat Jain.
//

#ifndef PLAJA_ACTION_OP_VERITAS_H
#define PLAJA_ACTION_OP_VERITAS_H

#include "action_op_to_veritas.h"
#include "../forward_smt_veritas.h"
#include "../../../parser/ast/model.h"
#include "../../../utils/utils.h"


struct UpdateVeritas: public UpdateToVeritas {

protected:
    const ModelVeritas& model;
    std::unique_ptr<ConjunctionInVeritas> assignments;

    // void set_additional_updating_vars(ModelVeritas& model_veritas);
    inline void step_updates() {}

public:
    UpdateVeritas(const Update& update_, const ModelVeritas& model_);
    ~UpdateVeritas() override;
    DELETE_CONSTRUCTOR(UpdateVeritas)

    virtual void generate_steps();
    virtual void increment();

    void add(VERITAS_IN_PLAJA::QueryConstructable& query, bool do_locs, bool do_copy, StepIndex_type step = 0) const;

    [[nodiscard]] std::unique_ptr<ConjunctionInVeritas> to_veritas(bool do_locs, bool do_copy, StepIndex_type step) const;

};

/**********************************************************************************************************************/


class ActionOpVeritas: public ActionOpToVeritas {

protected:
    const ModelVeritas& model;
    std::unique_ptr<ConjunctionInVeritas> guards;
    std::unique_ptr<VeritasConstraint> guardNegations;
    veritas::AddTree applicabilityTrees = veritas::AddTree(0, veritas::AddTreeType::REGR);
    // void set_additional_guard_vars(ModelVeritas& model_veritas);
    inline void step_guards() {}

public:
    ActionOpVeritas(const ActionOp& action_op, const ModelVeritas& model_, bool compute_updates = true);
    ~ActionOpVeritas() override;
    DELETE_CONSTRUCTOR(ActionOpVeritas)

    void generate_steps();
    void increment();

    [[nodiscard]] inline const UpdateVeritas& get_update_veritas(UpdateIndex_type index) const { return PLAJA_UTILS::cast_ref<UpdateVeritas>(get_update(index)); }

    /**
     * @return true to indicate whether query was modified
     */
    bool add_guard(VERITAS_IN_PLAJA::QueryConstructable& query, bool do_locs, StepIndex_type step = 0) const;

    /**
     * @return false if query has become trivially unsat (i.e., guard negation is false)
     */
    bool add_guard_negation(VERITAS_IN_PLAJA::QueryConstructable& query, bool do_locs, StepIndex_type step = 0) const;

    inline void add_update(VERITAS_IN_PLAJA::QueryConstructable& query, UpdateIndex_type index, bool do_locs, bool do_copy, StepIndex_type step = 0) const { get_update_veritas(index).add(query, do_locs, do_copy, step); }

    [[nodiscard]] std::unique_ptr<ConjunctionInVeritas> guard_to_veritas(bool do_locs, StepIndex_type step = 0) const;

    [[nodiscard]] std::unique_ptr<VeritasConstraint> guard_negation_to_veritas(bool do_locs, StepIndex_type step = 0) const;

    veritas::AddTree get_applicability_tree() const;
};

#endif //PLAJA_ACTION_OP_VERITAS_H
