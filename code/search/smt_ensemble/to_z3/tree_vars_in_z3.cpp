//
// This file is part of the PlaJA code base.
// Copyright (c) (2023 - 2024) Chaahat Jain.
//

#include "tree_vars_in_z3.h"
#include "../../information/model_information.h"
#include "../../smt/context_z3.h"
#include "../../smt/vars_in_z3.h"
#include "../veritas_context.h"
#include "veritas_to_z3.h"

// extern:
const std::string Z3_IN_PLAJA::veritasPrefix {"z3_veritas_"}; // NOLINT(cert-err58-cpp)
const std::string Z3_IN_PLAJA::treePrefix {"tree_z3_"}; // NOLINT(cert-err58-cpp)
const std::string Z3_IN_PLAJA::outputPrefix {"result_z3_"}; // NOLINT(cert-err58-cpp)

Z3_IN_PLAJA::VeritasToZ3Vars::VeritasToZ3Vars(Z3_IN_PLAJA::Context& c, const VERITAS_IN_PLAJA::Context& context_m, std::unordered_map<VeritasVarIndex_type, Z3_IN_PLAJA::VarId_type>&& veritas_to_z3, veritas::AddTree trees)
    : context(c)
    , veritasToZ3(std::move(veritas_to_z3)) {

    add(&context_m); add_operator_aux(&context_m);
}

Z3_IN_PLAJA::VeritasToZ3Vars::~VeritasToZ3Vars() = default;

//

z3::context& Z3_IN_PLAJA::VeritasToZ3Vars::get_context_z3() const { return _context()(); }

//

void Z3_IN_PLAJA::VeritasToZ3Vars::add(const VERITAS_IN_PLAJA::Context* contextVeritas) {
    for (auto it = contextVeritas->variableIterator(); !it.end(); ++it) {
        const auto veritas_var = it.var();
        Z3_IN_PLAJA::VarId_type var_z3;
        if (veritasToZ3.count(veritas_var)) {
            continue;
        } else {
            var_z3 = it.is_integer() ? context.add_real_var(Z3_IN_PLAJA::veritasPrefix) : context.add_int_var(Z3_IN_PLAJA::veritasPrefix);
            veritasToZ3.emplace(veritas_var, var_z3);
        }
    }
}

void Z3_IN_PLAJA::VeritasToZ3Vars::add_operator_aux(const VERITAS_IN_PLAJA::Context* contextVeritas) {
    const int offset = contextVeritas->get_number_of_variables();
    const auto aux_vars = contextVeritas->get_aux_vars();
    for (auto i = 0; i < aux_vars.size(); ++i) {
        if (veritasToZ3.count(offset + i)) continue;
        const auto var_z3 = context.add_int_var(Z3_IN_PLAJA::veritasPrefix);
        veritasToZ3.emplace(offset + i, var_z3);
    }
}

z3::expr_vector Z3_IN_PLAJA::VeritasToZ3Vars::get_bounds(const std::shared_ptr<VERITAS_IN_PLAJA::Context> verContext) {
    z3::expr_vector overall_bounds(get_context_z3());
    for (auto it = verContext->variableIterator(); !it.end(); ++it) {
        const auto veritas_var = it.var();
        auto lb = verContext->get_lower_bound(veritas_var);
        auto ub = verContext->get_upper_bound(veritas_var);
        PLAJA_ASSERT(exists(it.var()))
        auto bound = VERITAS_TO_Z3::generate_bounds(to_z3_expr(veritas_var), lb, ub);
        overall_bounds.push_back(bound);
    }
    return overall_bounds;
}


void Z3_IN_PLAJA::VeritasToZ3Vars::add_tree_vars(veritas::AddTree trees) {
    auto num_outputs = trees.num_leaf_values();
    auto num_trees = (trees.get_type() == veritas::AddTreeType::CLF_SOFTMAX) ? trees.size()/num_outputs : trees.size();
    for (auto label = 0; label < num_outputs; ++label) {
        for (auto tree = 0; tree < num_trees; ++tree) {
            if (treesToZ3.count({label, tree})) {
                continue;
            } else {
                const auto tree_z3 = context.add_real_var(Z3_IN_PLAJA::treePrefix);
                treesToZ3[{label, tree}] = tree_z3;
            }
        }
        if (label_values.count(label)) {
            continue;
        } else {
            const auto label_z3 = context.add_real_var(Z3_IN_PLAJA::outputPrefix);
            label_values[label] = label_z3;
        }
    }
}


Z3_IN_PLAJA::VarId_type Z3_IN_PLAJA::VeritasToZ3Vars::add_int_aux_var() const { return context.add_int_var(Z3_IN_PLAJA::auxiliaryVarPrefix); }

Z3_IN_PLAJA::VarId_type Z3_IN_PLAJA::VeritasToZ3Vars::add_real_aux_var() const { return context.add_real_var(Z3_IN_PLAJA::auxiliaryVarPrefix); }


const z3::expr& Z3_IN_PLAJA::VeritasToZ3Vars::to_z3_expr(VeritasVarIndex_type veritas_var) const { return context.get_var(to_z3(veritas_var)); }

std::unordered_map<VeritasVarIndex_type, Z3_IN_PLAJA::VarId_type> Z3_IN_PLAJA::VeritasToZ3Vars::reuse(const Z3_IN_PLAJA::StateVarsInZ3& state_vars_z3, const StateIndexesInVeritas& state_vars_veritas) {

    const auto& model_info = state_vars_z3.get_model_info();
    auto& context = state_vars_z3.get_context();
    std::unordered_map<VeritasVarIndex_type, Z3_IN_PLAJA::VarId_type> veritas_to_z3_cache;
    for (auto it = model_info.stateIndexIterator(true); !it.end(); ++it) {

        const auto state_index = it.state_index();
        if (not state_vars_veritas.exists(state_index)) { continue; }
        const auto veritas_var = state_vars_veritas.to_veritas(state_index);

        if (model_info.is_location(state_index)) {

            PLAJA_ASSERT(state_vars_z3.exists_loc(state_index))

            const auto loc_z3 = state_vars_z3.loc_to_z3(state_index);
            veritas_to_z3_cache.emplace(veritas_var, loc_z3);

        } else if (state_vars_z3.exists(model_info.get_id(state_index))) {

            if (model_info.belongs_to_array(state_index)) {

                const auto var_z3 = context.add_var(Z3_IN_PLAJA::bool_to_int_if(state_vars_z3.var_to_z3_expr_index(state_index)));
                veritas_to_z3_cache.emplace(veritas_var, var_z3);

            } else {

                const auto var_z3 = state_vars_z3.to_z3(model_info.get_id(state_index));
                const auto& var_expr = context.get_var(var_z3);
                if (var_expr.is_bool()) {
                    const auto var_z3_fresh = context.add_var(Z3_IN_PLAJA::bool_to_int(var_expr));
                    veritas_to_z3_cache.emplace(veritas_var, var_z3_fresh);
                } else {
                    veritas_to_z3_cache.emplace(veritas_var, state_vars_z3.to_z3(model_info.get_id(state_index)));
                }

            }

        } else {

            PLAJA_ABORT

        }

    }

    return veritas_to_z3_cache;

}