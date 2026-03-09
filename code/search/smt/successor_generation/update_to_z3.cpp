//
// This file is part of the PlaJA code base.
// Copyright (c) (2019 - 2021) Marcel Vinzent.
// See README.md in the top-level directory for licensing information.
//

#include <unordered_set>
#include <z3++.h>
#include "update_to_z3.h"
#include "../../successor_generation/update.h"
#include "../model/model_z3.h"
#include "../visitor/extern/to_z3_visitor.h"
#include "../utils/to_z3_expr.h"
#include "../context_z3.h"
#include "../vars_in_z3.h"


/**Auxiliary structures*************************************************************************************************/

namespace UPDATE_TO_Z3 {

    struct UpdateInfo {
    private:
        std::unordered_set<VariableIndex_type> nonUpdatedLocs;
        std::unordered_set<VariableID_type> nonUpdatedVars;

    public:
        explicit UpdateInfo(const Update& update):
            nonUpdatedLocs(update.infer_non_updated_locs())
            , nonUpdatedVars(update.infer_non_updated_vars(0)) {}

        ~UpdateInfo() = default;
        DEFAULT_CONSTRUCTOR(UpdateInfo)

        // extract information:
        [[nodiscard]] inline const std::unordered_set<VariableIndex_type>& _non_updated_locs() const { return nonUpdatedLocs; }

        [[nodiscard]] inline const std::unordered_set<VariableID_type>& _non_updated_vars() const { return nonUpdatedVars; }

    };

}

/**********************************************************************************************************************/


UpdateToZ3::UpdateToZ3(const Update& update_, const ModelZ3& model_z3, bool cache_non_updated):
    update(update_)
    , updateInfo(cache_non_updated ? new UPDATE_TO_Z3::UpdateInfo(update_) : nullptr)
    , modelZ3(model_z3) {
    PLAJA_ASSERT(update.get_num_sequences() <= 1)
}

UpdateToZ3::~UpdateToZ3() = default;

/**********************************************************************************************************************/

std::unordered_set<VariableID_type> UpdateToZ3::collect_updated_variables() const {
    PLAJA_ASSERT(update.get_num_sequences() <= 1)
    return update.collect_updated_vars(0);
}

std::unordered_set<VariableID_type> UpdateToZ3::collect_updated_state_indexes(bool include_locs) const {
    PLAJA_ASSERT(update.get_num_sequences() <= 1)
    return update.collect_updated_state_indexes(0, include_locs);
}

std::unordered_set<VariableID_type> UpdateToZ3::collect_updating_variables() const {
    PLAJA_ASSERT(update.get_num_sequences() <= 1)
    return update.collect_updating_vars(0);
}

std::unordered_set<VariableID_type> UpdateToZ3::collect_updating_state_indexes() const {
    PLAJA_ASSERT(update.get_num_sequences() <= 1)
    return update.collect_updating_state_indexes(0);
}

//

z3::expr UpdateToZ3::compute_unused_indexes(VariableID_type var_id, const std::vector<const Expression*>& updated_indexes, const Z3_IN_PLAJA::StateVarsInZ3& src_vars, const Z3_IN_PLAJA::StateVarsInZ3& target_vars) const {

    auto& context = modelZ3.get_context();
    const auto& aux_var = context.get_var(context.add_int_var(Z3_IN_PLAJA::auxiliaryVarPrefix));

    PLAJA_ASSERT(!updated_indexes.empty())
    auto updated_indexes_it = updated_indexes.cbegin();
    auto end = updated_indexes.cend();

    ToZ3Expr not_updated_indexes_z3 = Z3_IN_PLAJA::to_z3(**updated_indexes_it, src_vars);
    not_updated_indexes_z3.set(aux_var != not_updated_indexes_z3.release());
    for (; updated_indexes_it != end; ++updated_indexes_it) {
        ToZ3Expr to_z3_expr_tmp = Z3_IN_PLAJA::to_z3(**updated_indexes_it, src_vars);
        to_z3_expr_tmp.set(aux_var != to_z3_expr_tmp.release());
        not_updated_indexes_z3.conjunct(to_z3_expr_tmp);
    }

    // for all k: k != used index_1 && ... && k != used index_n (&& k within bounds) => target_array[k] = src_array[k]
    // && 0 <= aux_var && aux_var < context.int_val(model_z3.get_array_size(var_id))
    not_updated_indexes_z3.set(z3::forall(aux_var, z3::implies(not_updated_indexes_z3.release(), z3::select(target_vars.to_z3_expr(var_id), aux_var) == z3::select(src_vars.to_z3_expr(var_id), aux_var)))); // TODO check if w/o bounds is beneficial
    return not_updated_indexes_z3.to_conjunction(); // auxiliary should be conjuncted

}

z3::expr UpdateToZ3::compute_unused_indexes(VariableID_type var_id, const std::vector<z3::expr>& updated_indexes, const Z3_IN_PLAJA::StateVarsInZ3& src_vars, const Z3_IN_PLAJA::StateVarsInZ3& target_vars) const {

    auto& context = modelZ3.get_context();
    const auto& aux_var = context.get_var(context.add_int_var(Z3_IN_PLAJA::auxiliaryVarPrefix));

    PLAJA_ASSERT(!updated_indexes.empty())
    auto updated_indexes_it = updated_indexes.cbegin();
    auto end = updated_indexes.cend();

    z3::expr not_updated_indexes_z3 = aux_var != *updated_indexes_it;
    for (; updated_indexes_it != end; ++updated_indexes_it) { not_updated_indexes_z3 = not_updated_indexes_z3 && aux_var != *updated_indexes_it; }

    // for all k: k != used index_1 && ... && k != used index_n (&& k within bounds) => target_array[k] = src_array[k]
    // && 0 <= aux_var && aux_var < context.int_val(model_z3.get_array_size(var_id))
    return z3::forall(aux_var, z3::implies(not_updated_indexes_z3, z3::select(target_vars.to_z3_expr(var_id), aux_var) == z3::select(src_vars.to_z3_expr(var_id), aux_var))); // TODO check if w/o bounds is beneficial
}

/**********************************************************************************************************************/

z3::expr_vector UpdateToZ3::to_z3_loc(const Z3_IN_PLAJA::StateVarsInZ3& target_vars) const {
    z3::expr_vector update_locs_in_z3(modelZ3.get_context_z3());
    auto& c = modelZ3.get_context();

    for (auto loc_it = update.locationIterator(); !loc_it.end(); ++loc_it) {
        update_locs_in_z3.push_back(target_vars.loc_to_z3_expr(loc_it.loc()) == c.int_val(loc_it.dest()));
    }

    return update_locs_in_z3;

}

z3::expr_vector UpdateToZ3::to_z3_loc_copy(const Z3_IN_PLAJA::StateVarsInZ3& src_vars, const Z3_IN_PLAJA::StateVarsInZ3& target_vars) const {
    PLAJA_ASSERT(src_vars.has_loc() == target_vars.has_loc())
    z3::expr_vector update_locs_in_z3(modelZ3.get_context_z3());

    PLAJA_ASSERT(updateInfo)
    for (auto loc_index: updateInfo->_non_updated_locs()) {
        update_locs_in_z3.push_back(target_vars.loc_to_z3_expr(loc_index) == src_vars.loc_to_z3_expr(loc_index));
    }

    return update_locs_in_z3;
}

z3::expr_vector UpdateToZ3::to_z3_var(const Z3_IN_PLAJA::StateVarsInZ3& src_vars, const Z3_IN_PLAJA::StateVarsInZ3& target_vars) const {
    PLAJA_ASSERT(src_vars.has_loc() == target_vars.has_loc())
    z3::expr_vector update_in_z3(modelZ3.get_context_z3());

    // add constraints for updated variables
    std::unordered_map<VariableID_type, std::vector<z3::expr>> updated_array_indexes;
    for (auto assignment_it = update.assignmentIterator(0); !assignment_it.end(); ++assignment_it) {
        update_in_z3.push_back(modelZ3.compute_assignment(*assignment_it, src_vars, target_vars, &updated_array_indexes, nullptr));
    }

    // add constraint for unused indexes of updated array variables
    for (const auto& [var_id, updated_indexes]: updated_array_indexes) { update_in_z3.push_back(compute_unused_indexes(var_id, updated_indexes, src_vars, target_vars)); }

    return update_in_z3;
}

z3::expr_vector UpdateToZ3::to_z3_var_copy(const Z3_IN_PLAJA::StateVarsInZ3& src_vars, const Z3_IN_PLAJA::StateVarsInZ3& target_vars) const {
    PLAJA_ASSERT(src_vars.has_loc() == target_vars.has_loc())
    z3::expr_vector update_in_z3(modelZ3.get_context_z3());

    PLAJA_ASSERT(updateInfo)
    for (auto var_id: updateInfo->_non_updated_vars()) { update_in_z3.push_back(target_vars.to_z3_expr(var_id) == src_vars.to_z3_expr(var_id)); }

    return update_in_z3;
}

z3::expr_vector UpdateToZ3::to_z3(const Z3_IN_PLAJA::StateVarsInZ3& src_vars, const Z3_IN_PLAJA::StateVarsInZ3& target_vars, bool do_locs, bool do_copy) const {
    PLAJA_ASSERT(src_vars.size() == target_vars.size())

    z3::expr_vector update_in_z3 = to_z3_var(src_vars, target_vars);

    if (do_copy) { for (const auto& e: to_z3_var_copy(src_vars, target_vars)) { update_in_z3.push_back(e); } }

    if (do_locs) {

        for (const auto& e: to_z3_loc(target_vars)) { update_in_z3.push_back(e); }

        if (do_copy) { for (const auto& e: to_z3_loc_copy(src_vars, target_vars)) { update_in_z3.push_back(e); } }

    }

    return update_in_z3;
}
