//
// This file is part of the PlaJA code base.
// Copyright (c) (2019 - 2021) Marcel Vinzent.
// See README.md in the top-level directory for licensing information.
//

#include "nn_vars_in_z3.h"
#include "../../information/model_information.h"
#include "../../smt/context_z3.h"
#include "../../smt/vars_in_z3.h"
#include "../marabou_context.h"
#include "../vars_in_marabou.h"
#include "marabou_to_z3.h"

// extern:
const std::string Z3_IN_PLAJA::marabouPrefix {"z3_marabou_"}; // NOLINT(cert-err58-cpp)


Z3_IN_PLAJA::MarabouToZ3Vars::MarabouToZ3Vars(Z3_IN_PLAJA::Context& c, const MARABOU_IN_PLAJA::Context& context_m, std::unordered_map<MarabouVarIndex_type, Z3_IN_PLAJA::VarId_type>&& marabou_to_z3)
    : context(&c)
    , contextMarabou(&context_m)
    , marabouToZ3(std::move(marabou_to_z3)) {

    for (auto it = context_m.variableIterator(); !it.end(); ++it) {
        const auto marabou_var = it.var();
        if (not exists(it.var())) {
            const auto var_z3 = it.is_integer() ? context->add_real_var(Z3_IN_PLAJA::marabouPrefix) : context->add_int_var(Z3_IN_PLAJA::marabouPrefix);
            marabouToZ3.emplace(marabou_var, var_z3);
        }
    }
}

Z3_IN_PLAJA::MarabouToZ3Vars::~MarabouToZ3Vars() = default;

//

z3::context& Z3_IN_PLAJA::MarabouToZ3Vars::get_context_z3() const { return _context()(); }

//

Z3_IN_PLAJA::VarId_type Z3_IN_PLAJA::MarabouToZ3Vars::add(MarabouVarIndex_type marabou_var) {
    PLAJA_ASSERT(not exists(marabou_var))
    const auto var_z3 = contextMarabou->is_marked_integer(marabou_var) ? context->add_real_var(Z3_IN_PLAJA::marabouPrefix) : context->add_int_var(Z3_IN_PLAJA::marabouPrefix);
    marabouToZ3.emplace(marabou_var, var_z3);
    return var_z3;
}

Z3_IN_PLAJA::VarId_type Z3_IN_PLAJA::MarabouToZ3Vars::add_int_aux_var() const { return context->add_int_var(Z3_IN_PLAJA::auxiliaryVarPrefix); }

const z3::expr& Z3_IN_PLAJA::MarabouToZ3Vars::to_z3_expr(MarabouVarIndex_type marabou_var) const { return context->get_var(to_z3(marabou_var)); }

std::unordered_map<MarabouVarIndex_type, Z3_IN_PLAJA::VarId_type> Z3_IN_PLAJA::MarabouToZ3Vars::reuse(const Z3_IN_PLAJA::StateVarsInZ3& state_vars_z3, const StateIndexesInMarabou& state_vars_marabou) {

    const auto& model_info = state_vars_z3.get_model_info();
    auto& context = state_vars_z3.get_context();
    std::unordered_map<MarabouVarIndex_type, Z3_IN_PLAJA::VarId_type> marabou_to_z3_cache;
    for (auto it = model_info.stateIndexIterator(true); !it.end(); ++it) {

        const auto state_index = it.state_index();
        if (not state_vars_marabou.exists(state_index)) { continue; }
        const auto marabou_var = state_vars_marabou.to_marabou(state_index);

        if (model_info.is_location(state_index)) {

            PLAJA_ASSERT(state_vars_z3.exists_loc(state_index))

            const auto loc_z3 = state_vars_z3.loc_to_z3(state_index);
            marabou_to_z3_cache.emplace(marabou_var, loc_z3);

        } else if (state_vars_z3.exists(model_info.get_id(state_index))) {

            if (model_info.belongs_to_array(state_index)) {

                const auto var_z3 = context.add_var(Z3_IN_PLAJA::bool_to_int_if(state_vars_z3.var_to_z3_expr_index(state_index)));
                marabou_to_z3_cache.emplace(marabou_var, var_z3);

            } else {

                const auto var_z3 = state_vars_z3.to_z3(model_info.get_id(state_index));
                const auto& var_expr = context.get_var(var_z3);
                if (var_expr.is_bool()) {
                    const auto var_z3_fresh = context.add_var(Z3_IN_PLAJA::bool_to_int(var_expr));
                    marabou_to_z3_cache.emplace(marabou_var, var_z3_fresh);
                } else {
                    marabou_to_z3_cache.emplace(marabou_var, state_vars_z3.to_z3(model_info.get_id(state_index)));
                }

            }

        } else {

            PLAJA_ABORT

        }

    }

    return marabou_to_z3_cache;

}