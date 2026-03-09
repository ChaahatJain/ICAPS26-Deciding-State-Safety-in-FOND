//
// This file is part of the PlaJA code base.
// Copyright (c) (2023 - 2024) Chaahat Jain.
//

#ifndef PLAJA_ACTION_OP_TO_VERITAS_H
#define PLAJA_ACTION_OP_TO_VERITAS_H

#include <memory>
#include <unordered_set>
#include <vector>
#include "../../../parser/using_parser.h"
#include "../../../assertions.h"
#include "../../using_search.h"
#include "../using_veritas.h"

// forward declaration:
class ActionOp;
class ConjunctionInVeritas;
class Model;
class StateIndexesInVeritas;
class Update;


struct UpdateToVeritas {

protected:
    const Update& update;

public:
    explicit UpdateToVeritas(const Update& update_);
    virtual ~UpdateToVeritas();
    DELETE_CONSTRUCTOR(UpdateToVeritas)

    [[nodiscard]] inline const Update& _concrete() const { return update; }

    [[nodiscard]] std::unordered_set<VariableIndex_type> collect_updated_state_indexes(bool include_locs = true) const;
    [[nodiscard]] std::unordered_set<VariableIndex_type> collect_updating_state_indexes() const;
    [[nodiscard]] std::unordered_set<VeritasVarIndex_type> collect_updated_in_veritas(const StateIndexesInVeritas& target_vars, bool include_locs = true) const;
    [[nodiscard]] std::unordered_set<VeritasVarIndex_type> collect_updating_in_veritas(const StateIndexesInVeritas& src_vars) const;

    [[nodiscard]] std::unique_ptr<ConjunctionInVeritas> to_veritas_loc(const StateIndexesInVeritas& src_vars, const StateIndexesInVeritas& target_vars, bool do_copy) const;
    [[nodiscard]] std::unique_ptr<ConjunctionInVeritas> to_veritas_loc_copy(const StateIndexesInVeritas& src_vars, const StateIndexesInVeritas& target_vars) const;
    [[nodiscard]] std::unique_ptr<ConjunctionInVeritas> to_veritas_copy(const StateIndexesInVeritas& src_vars, const StateIndexesInVeritas& target_vars, bool do_locs) const;
    [[nodiscard]] std::unique_ptr<ConjunctionInVeritas> to_veritas(const StateIndexesInVeritas& src_vars, const StateIndexesInVeritas& target_vars, bool do_locs, bool do_copy) const;

};

/**********************************************************************************************************************/

class ActionOpToVeritas {

protected:
    const ActionOp& actionOp;
    std::vector<std::unique_ptr<UpdateToVeritas>> updates;

public:
    explicit ActionOpToVeritas(const ActionOp& action_op, bool compute_updates = true);
    virtual ~ActionOpToVeritas();
    DELETE_CONSTRUCTOR(ActionOpToVeritas)

    [[nodiscard]] inline const ActionOp& _concrete() const { return actionOp; }
    [[nodiscard]] ActionLabel_type _action_label() const;
    [[nodiscard]] ActionOpID_type _op_id() const;

    [[nodiscard]] inline std::size_t size() const { return updates.size(); }
    [[nodiscard]] inline const UpdateToVeritas& get_update(UpdateIndex_type index) const { PLAJA_ASSERT(index < updates.size()) return *updates[index]; }

    [[nodiscard]] std::unordered_set<VeritasVarIndex_type> collect_guard_vars_in_veritas(const StateIndexesInVeritas& vars) const;
    [[nodiscard]] std::unique_ptr<ConjunctionInVeritas> guard_to_veritas_loc(const StateIndexesInVeritas& vars) const;
    [[nodiscard]] std::unique_ptr<ConjunctionInVeritas> guard_to_veritas(const StateIndexesInVeritas& vars, bool do_locs = true) const;
    [[nodiscard]] std::unique_ptr<ConjunctionInVeritas> update_to_veritas(UpdateIndex_type update_index, const StateIndexesInVeritas& src_vars, const StateIndexesInVeritas& target_vars, bool do_locs, bool do_copy) const;
    [[nodiscard]] std::unique_ptr<ConjunctionInVeritas> to_veritas(UpdateIndex_type update_index, const StateIndexesInVeritas& src_vars, const StateIndexesInVeritas& target_vars, bool do_locs, bool do_copy) const;
    [[nodiscard]] std::unique_ptr<ConjunctionInVeritas> to_veritas(const StateIndexesInVeritas& src_vars, const StateIndexesInVeritas& target_vars, bool do_locs, bool do_copy) const;

};

#endif //PLAJA_ACTION_OP_TO_VERITAS_H
