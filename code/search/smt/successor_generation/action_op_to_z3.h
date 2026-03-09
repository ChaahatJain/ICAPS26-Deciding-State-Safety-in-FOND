//
// This file is part of the PlaJA code base.
// Copyright (c) (2019 - 2021) Marcel Vinzent.
// See README.md in the top-level directory for licensing information.
//

#ifndef PLAJA_ACTION_OP_TO_Z3_H
#define PLAJA_ACTION_OP_TO_Z3_H

#include <memory>
#include <unordered_set>
#include "../../../assertions.h"
#include "../../successor_generation/forward_successor_generation.h"
#include "../../using_search.h"
#include "../forward_smt_z3.h"
#include "../using_smt.h"

class ActionOpToZ3 {
    friend class SuccessorGeneratorToZ3;

protected:
    const ActionOp& actionOp;
    std::vector<std::unique_ptr<UpdateToZ3>> updates;
    const ModelZ3& modelZ3;

    ActionOpToZ3(const ActionOp& action_op, const ModelZ3& model_z3, bool compute_updates = true);
public:
    ~ActionOpToZ3();
    DELETE_CONSTRUCTOR(ActionOpToZ3)

    [[nodiscard]] inline const ActionOp& _concrete() const { return actionOp; }

    [[nodiscard]] ActionLabel_type _action_label() const;

    [[nodiscard]] ActionOpID_type _op_id() const;

    FCT_IF_DEBUG(void dump() const;)

    [[nodiscard]] inline std::size_t size() const { return updates.size(); }

    [[nodiscard]] const ModelZ3& get_model_z3() const { return modelZ3; }

    /* */

    [[nodiscard]] std::unordered_set<VariableID_type> collect_guard_variables() const;

    [[nodiscard]] std::unordered_set<VariableID_type> collect_guard_state_indexes() const;

    /* */

    [[nodiscard]] std::unique_ptr<Z3_IN_PLAJA::Constraint> guard_to_z3_loc(const Z3_IN_PLAJA::StateVarsInZ3& vars) const;

    [[nodiscard]] std::unique_ptr<Z3_IN_PLAJA::Constraint> guard_to_z3(const Z3_IN_PLAJA::StateVarsInZ3& vars, bool do_locs) const;

    [[nodiscard]] std::unique_ptr<ToZ3ExprSplits> guard_to_z3_splits(const Z3_IN_PLAJA::StateVarsInZ3& vars) const;

    [[nodiscard]] std::unique_ptr<Z3_IN_PLAJA::Constraint> update_to_z3(UpdateIndex_type update_index, const Z3_IN_PLAJA::StateVarsInZ3& src_vars, const Z3_IN_PLAJA::StateVarsInZ3& target_vars, bool do_locs, bool do_copy) const;

    [[nodiscard]] std::unique_ptr<Z3_IN_PLAJA::Constraint> to_z3(UpdateIndex_type update_index, const Z3_IN_PLAJA::StateVarsInZ3& src_vars, const Z3_IN_PLAJA::StateVarsInZ3& target_vars, bool do_locs, bool do_copy) const;

    [[nodiscard]] std::unique_ptr<Z3_IN_PLAJA::Constraint> to_z3(const Z3_IN_PLAJA::StateVarsInZ3& src_vars, const Z3_IN_PLAJA::StateVarsInZ3& target_vars, bool do_locs, bool do_copy) const;

    [[nodiscard]] inline const UpdateToZ3& get_update(UpdateIndex_type index) const {
        PLAJA_ASSERT(index < updates.size())
        return *updates[index];
    }

};

/**********************************************************************************************************************/

template<typename Update_t, typename UpdateView_t>
class UpdateIteratorBase final {

private:
    const std::vector<std::unique_ptr<Update_t>>& updates;
    UpdateIndex_type updateIndex;

public:
    explicit UpdateIteratorBase(const std::vector<std::unique_ptr<Update_t>>& updates_):
        updates(updates_)
        , updateIndex(0) {}

    ~UpdateIteratorBase() = default;
    DELETE_CONSTRUCTOR(UpdateIteratorBase)

    inline void operator++() { ++updateIndex; }

    [[nodiscard]] inline bool end() const { return updateIndex >= updates.size(); }

    [[nodiscard]] inline const UpdateView_t& update() const { return static_cast<UpdateView_t&>(*updates[updateIndex]); } // NOLINT(cppcoreguidelines-pro-type-static-cast-downcast)

    [[nodiscard]] inline UpdateIndex_type update_index() const { return updateIndex; }

};

/**********************************************************************************************************************/

#endif //PLAJA_ACTION_OP_TO_Z3_H
