//
// This file is part of the PlaJA code base.
// Copyright (c) (2019 - 2021) Marcel Vinzent.
// See README.md in the top-level directory for licensing information.
//

#ifndef PLAJA_UPDATE_TO_Z3_H
#define PLAJA_UPDATE_TO_Z3_H


#include <memory>
#include "../../../parser/using_parser.h"
#include "../../../utils/default_constructors.h"
#include "../using_smt.h"

// forward declaration:
class Expression;
class ModelZ3;
class Update;
namespace UPDATE_TO_Z3 { struct UpdateInfo; }

class UpdateToZ3 {
    friend class ActionOpToZ3;

protected:
    const Update& update;
    std::unique_ptr<UPDATE_TO_Z3::UpdateInfo> updateInfo;
    const ModelZ3& modelZ3;

    UpdateToZ3(const Update& update_, const ModelZ3& model_z3, bool cache_non_updated = true);
public:
    virtual ~UpdateToZ3();
    DELETE_CONSTRUCTOR(UpdateToZ3)

    [[nodiscard]] inline const Update& _concrete() const { return update; }
    [[nodiscard]] inline const ModelZ3& _model_z3() const { return modelZ3; }

    [[nodiscard]] std::unordered_set<VariableID_type> collect_updated_variables() const;
    [[nodiscard]] std::unordered_set<VariableID_type> collect_updated_state_indexes(bool include_locs = false) const;
    [[nodiscard]] std::unordered_set<VariableID_type> collect_updating_variables() const;
    [[nodiscard]] std::unordered_set<VariableID_type> collect_updating_state_indexes() const;

    [[nodiscard]] z3::expr compute_unused_indexes(VariableID_type var_id, const std::vector<const Expression*>& used_indexes, const Z3_IN_PLAJA::StateVarsInZ3& src_vars, const Z3_IN_PLAJA::StateVarsInZ3& target_vars) const;
    [[nodiscard]] z3::expr compute_unused_indexes(VariableID_type var_id, const std::vector<z3::expr>& used_indexes, const Z3_IN_PLAJA::StateVarsInZ3& src_vars, const Z3_IN_PLAJA::StateVarsInZ3& target_vars) const;

    [[nodiscard]] z3::expr_vector to_z3_loc(const Z3_IN_PLAJA::StateVarsInZ3& target_vars) const;
    [[nodiscard]] z3::expr_vector to_z3_loc_copy(const Z3_IN_PLAJA::StateVarsInZ3& src_vars, const Z3_IN_PLAJA::StateVarsInZ3& target_vars) const;
    [[nodiscard]] z3::expr_vector to_z3_var(const Z3_IN_PLAJA::StateVarsInZ3& src_vars, const Z3_IN_PLAJA::StateVarsInZ3& target_vars) const;
    [[nodiscard]] z3::expr_vector to_z3_var_copy(const Z3_IN_PLAJA::StateVarsInZ3& src_vars, const Z3_IN_PLAJA::StateVarsInZ3& target_vars) const;
    [[nodiscard]] z3::expr_vector to_z3(const Z3_IN_PLAJA::StateVarsInZ3& src_vars, const Z3_IN_PLAJA::StateVarsInZ3& target_vars, bool do_locs, bool do_copy) const;

};

#endif //PLAJA_UPDATE_TO_Z3_H
