//
// This file is part of the PlaJA code base.
// Copyright (c) (2019 - 2020) Marcel Vinzent.
// See README.md in the top-level directory for licensing information.
//

#ifndef PLAJA_ACTION_OP_TO_MARABOU_H
#define PLAJA_ACTION_OP_TO_MARABOU_H

#include <memory>
#include <unordered_set>
#include <vector>
#include "../../../parser/using_parser.h"
#include "../../../assertions.h"
#include "../../using_search.h"
#include "../using_marabou.h"

// forward declaration:
class ActionOp;
class ConjunctionInMarabou;
class Model;
class StateIndexesInMarabou;
class Update;


struct UpdateToMarabou {

protected:
    const Update& update;

public:
    explicit UpdateToMarabou(const Update& update_);
    virtual ~UpdateToMarabou();
    DELETE_CONSTRUCTOR(UpdateToMarabou)

    [[nodiscard]] inline const Update& _concrete() const { return update; }

    [[nodiscard]] std::unordered_set<VariableIndex_type> collect_updated_state_indexes(bool include_locs = true) const;
    [[nodiscard]] std::unordered_set<VariableIndex_type> collect_updating_state_indexes() const;
    [[nodiscard]] std::unordered_set<MarabouVarIndex_type> collect_updated_in_marabou(const StateIndexesInMarabou& target_vars, bool include_locs = true) const;
    [[nodiscard]] std::unordered_set<MarabouVarIndex_type> collect_updating_in_marabou(const StateIndexesInMarabou& src_vars) const;

    [[nodiscard]] std::unique_ptr<ConjunctionInMarabou> to_marabou_loc(const StateIndexesInMarabou& src_vars, const StateIndexesInMarabou& target_vars, bool do_copy) const;
    [[nodiscard]] std::unique_ptr<ConjunctionInMarabou> to_marabou_loc_copy(const StateIndexesInMarabou& src_vars, const StateIndexesInMarabou& target_vars) const;
    [[nodiscard]] std::unique_ptr<ConjunctionInMarabou> to_marabou_copy(const StateIndexesInMarabou& src_vars, const StateIndexesInMarabou& target_vars, bool do_locs) const;
    [[nodiscard]] std::unique_ptr<ConjunctionInMarabou> to_marabou(const StateIndexesInMarabou& src_vars, const StateIndexesInMarabou& target_vars, bool do_locs, bool do_copy) const;

};

/**********************************************************************************************************************/

class ActionOpToMarabou {

protected:
    const ActionOp& actionOp;
    std::vector<std::unique_ptr<UpdateToMarabou>> updates;

public:
    explicit ActionOpToMarabou(const ActionOp& action_op, bool compute_updates = true);
    virtual ~ActionOpToMarabou();
    DELETE_CONSTRUCTOR(ActionOpToMarabou)

    [[nodiscard]] inline const ActionOp& _concrete() const { return actionOp; }
    [[nodiscard]] ActionLabel_type _action_label() const;
    [[nodiscard]] ActionOpID_type _op_id() const;

    [[nodiscard]] inline std::size_t size() const { return updates.size(); }
    [[nodiscard]] inline const UpdateToMarabou& get_update(UpdateIndex_type index) const { PLAJA_ASSERT(index < updates.size()) return *updates[index]; }

    [[nodiscard]] std::unordered_set<MarabouVarIndex_type> collect_guard_vars_in_marabou(const StateIndexesInMarabou& vars) const;
    [[nodiscard]] std::unique_ptr<ConjunctionInMarabou> guard_to_marabou_loc(const StateIndexesInMarabou& vars) const;
    [[nodiscard]] std::unique_ptr<ConjunctionInMarabou> guard_to_marabou(const StateIndexesInMarabou& vars, bool do_locs = true) const;
    [[nodiscard]] std::unique_ptr<ConjunctionInMarabou> update_to_marabou(UpdateIndex_type update_index, const StateIndexesInMarabou& src_vars, const StateIndexesInMarabou& target_vars, bool do_locs, bool do_copy) const;
    [[nodiscard]] std::unique_ptr<ConjunctionInMarabou> to_marabou(UpdateIndex_type update_index, const StateIndexesInMarabou& src_vars, const StateIndexesInMarabou& target_vars, bool do_locs, bool do_copy) const;
    [[nodiscard]] std::unique_ptr<ConjunctionInMarabou> to_marabou(const StateIndexesInMarabou& src_vars, const StateIndexesInMarabou& target_vars, bool do_locs, bool do_copy) const;

};

#endif //PLAJA_ACTION_OP_TO_MARABOU_H
