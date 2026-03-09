//
// This file is part of the PlaJA code base.
// Copyright (c) (2019 - 2023) Marcel Vinzent.
// See README.md in the top-level directory for licensing information.
//

#include "vars_in_z3.h"
#include "../../include/ct_config_const.h"
#include "../../parser/ast/expression/array_value_expression.h"
#include "../../parser/ast/type/array_type.h"
#include "../../parser/ast/constant_declaration.h"
#include "../../parser/ast/model.h"
#include "../../parser/ast/variable_declaration.h"
#include "../../utils/floating_utils.h"
#include "../../utils/utils.h"
#include "../information/model_information.h"
#include "../states/state_values.h"
#include "model/model_z3.h"
#include "solver/solution_z3.h"
#include "visitor/extern/to_z3_visitor.h"
#include "context_z3.h"

const std::string Z3_IN_PLAJA::locPrefix { "z3_loc_" }; // NOLINT(cert-err58-cpp)

Z3_IN_PLAJA::StateVarsInZ3::StateVarsInZ3(const ModelZ3& parent):
    context(&parent.get_context())
    , parent(&parent) {
}

Z3_IN_PLAJA::StateVarsInZ3::~StateVarsInZ3() = default;

Z3_IN_PLAJA::StateVarsInZ3::StateVarsInZ3(const Z3_IN_PLAJA::StateVarsInZ3& other) = default;

inline const Model& Z3_IN_PLAJA::StateVarsInZ3::get_model() const { return parent->get_model(); }

const ModelInformation& Z3_IN_PLAJA::StateVarsInZ3::get_model_info() const { return parent->get_model_info(); }

FCT_IF_DEBUG(bool Z3_IN_PLAJA::StateVarsInZ3::is_loc(VariableIndex_type state_index) const { return get_model_info().is_location(state_index); })

/**********************************************************************************************************************/

std::unique_ptr<z3::expr> Z3_IN_PLAJA::StateVarsInZ3::generate_bound(const VariableDeclaration& state_var) const {
    const auto& model_info = get_model_info();
    const auto* decl_type = state_var.get_type();
    PLAJA_ASSERT(decl_type)

    const auto var_index = state_var.get_index();
    const auto var_expr = to_z3_expr(state_var.get_id());

    switch (decl_type->get_kind()) {

        case DeclarationType::Bounded: {
            if (decl_type->is_floating_type()) {
                return std::make_unique<z3::expr>((context->real_val(model_info.get_lower_bound_float(var_index), Z3_IN_PLAJA::floatingPrecision) <= var_expr) && (context->real_val(model_info.get_upper_bound_float(var_index), Z3_IN_PLAJA::floatingPrecision) >= var_expr));
            } else {
                PLAJA_ASSERT(decl_type->is_integer_type())
                return std::make_unique<z3::expr>((context->int_val(model_info.get_lower_bound_int(var_index)) <= var_expr) && (context->int_val(model_info.get_upper_bound_int(var_index)) >= var_expr));
            }
        }

        case DeclarationType::Array : {

            const auto* elem_type = PLAJA_UTILS::cast_ptr<ArrayType>(decl_type)->get_element_type();

            if (elem_type->is_bounded_type()) {
                if (elem_type->is_floating_type()) {
                    // FORALL w/o bound restriction // TODO what is beneficial to Z3
                    const z3::expr aux_var = context->get_var(context->add_int_var(Z3_IN_PLAJA::auxiliaryVarPrefix));
                    return std::make_unique<z3::expr>(
                        z3::forall(aux_var, // z3::implies(0 <= aux_var && aux_var <= get_array_size(i), // disabling the within-bound-premise strengthens condition; this is however invariant towards sat as predicates and assignments only refer within bounds
                                   context->real_val(model_info.get_lower_bound_float(var_index), Z3_IN_PLAJA::floatingPrecision) <= z3::select(var_expr, aux_var) &&
                                   context->real_val(model_info.get_upper_bound_float(var_index), Z3_IN_PLAJA::floatingPrecision) >= z3::select(var_expr, aux_var) // )
                        ));
                } else {
                    PLAJA_ASSERT(elem_type->is_integer_type())
                    // FORALL w/o bound restriction
                    const z3::expr aux_var = context->get_var(context->add_int_var(Z3_IN_PLAJA::auxiliaryVarPrefix));
                    return std::make_unique<z3::expr>(
                        z3::forall(aux_var, // z3::implies(0 <= aux_var && aux_var <= get_array_size(i), // disabling the within-bound-premise strengthens condition; this is however invariant towards sat as predicates and assignments only refer within bounds
                                   context->int_val(model_info.get_lower_bound_int(var_index)) <= z3::select(var_expr, aux_var) &&
                                   context->int_val(model_info.get_upper_bound_int(var_index)) >= z3::select(var_expr, aux_var) // )
                        ));
                }

            } else {
                PLAJA_ASSERT(elem_type->is_boolean_type())
                return nullptr; // no bounding for bools
            }

        }

        default: {
            PLAJA_ASSERT(decl_type->is_boolean_type())
            return nullptr; // no bounding for bools
        }

    }

    PLAJA_ABORT
}

z3::expr Z3_IN_PLAJA::StateVarsInZ3::generate_bound_loc(VariableIndex_type loc) const {
    const auto& model_info = get_model_info();
    const auto& loc_expr = loc_to_z3_expr(loc);
    return (context->int_val(model_info.get_lower_bound_int(loc)) <= loc_expr) && (context->int_val(model_info.get_upper_bound_int(loc)) >= loc_expr);
}

z3::expr Z3_IN_PLAJA::StateVarsInZ3::generate_fix(const VariableDeclaration& state_var) const {
    PLAJA_ABORT // Should be unused.

    const auto& model_info = get_model_info();
    auto& c = (*context);

    const auto* decl_type = state_var.get_type();
    PLAJA_ASSERT(decl_type)

    const auto var_index = state_var.get_index();
    const auto& var_expr = to_z3_expr(state_var.get_id());

    switch (decl_type->get_kind()) {

        case DeclarationType::Bool: { return model_info.get_initial_value_int(var_index) ? var_expr : !var_expr; }

        case DeclarationType::Bounded : {
            if (decl_type->is_floating_type()) { return var_expr == c.real_val(model_info.get_initial_value_float(var_index), Z3_IN_PLAJA::floatingPrecision); }
            else {
                PLAJA_ASSERT(decl_type->is_integer_type())
                return var_expr == c.int_val(model_info.get_initial_value_int(var_index));
            }
        }

        case DeclarationType::Array: {

            const std::size_t arr_size = model_info.get_array_size(state_var.get_id());
            PLAJA_ASSERT(arr_size > 0)

            bool values_equal_flag = true; // For special case encoding.

            const auto* elem_type = PLAJA_UTILS::cast_ref<ArrayType>(*decl_type).get_element_type();
            z3::expr default_value_z3(context->operator()());
            z3::expr array_value_z3(context->operator()());

            if (elem_type->is_bounded_type()) {

                if (elem_type->is_floating_type()) {

                    const auto default_value = model_info.get_initial_value_float(var_index);
                    default_value_z3 = c.real_val(default_value, Z3_IN_PLAJA::floatingPrecision); // For special case encoding.
                    array_value_z3 = z3::select(var_expr, c.int_val(0)) == default_value_z3;
                    for (std::size_t i = 1; i < arr_size; ++i) {
                        const auto value_i = model_info.get_initial_value_float(var_index + i);
                        if (PLAJA_FLOATS::non_equal(value_i, default_value, PLAJA::floatingPrecision)) { values_equal_flag = false; } // For special case encoding.
                        array_value_z3 = array_value_z3 && z3::select(var_expr, c.int_val(PLAJA_UTILS::cast_numeric<PLAJA::integer>(i))) == c.real_val(value_i, Z3_IN_PLAJA::floatingPrecision);
                    }

                } else {

                    PLAJA_ASSERT(elem_type->is_integer_type())
                    const auto default_value = model_info.get_initial_value_int(var_index);
                    default_value_z3 = context->int_val(default_value); // For special case encoding.
                    array_value_z3 = z3::select(var_expr, c.int_val(0)) == default_value_z3;
                    for (std::size_t i = 1; i < arr_size; ++i) {
                        const auto value_i = model_info.get_initial_value_int(var_index + i);
                        if (value_i != default_value) { values_equal_flag = false; } // For special case encoding.
                        array_value_z3 = array_value_z3 && z3::select(var_expr, c.int_val(PLAJA_UTILS::cast_numeric<PLAJA::integer>(i))) == c.int_val(value_i);
                    }

                }

            } else {

                PLAJA_ASSERT(elem_type->is_boolean_type()) // BOOL
                const auto default_value = model_info.get_initial_value_int(var_index);
                default_value_z3 = context->bool_val(default_value); // For special case encoding.
                array_value_z3 = default_value ? z3::select(var_expr, c.int_val(0)) : !z3::select(var_expr, c.int_val(0));
                for (std::size_t i = 1; i < arr_size; ++i) {
                    const auto value_i = model_info.get_initial_value_int(var_index + i);
                    if (value_i != default_value) { values_equal_flag = false; } // For special case encoding.
                    array_value_z3 = array_value_z3 && (value_i ? z3::select(var_expr, c.int_val(PLAJA_UTILS::cast_numeric<PLAJA::integer>(i))) : !z3::select(var_expr, c.int_val(PLAJA_UTILS::cast_numeric<PLAJA::integer>(i))));
                }

            }

            if (values_equal_flag) { // For special case encoding.
                const z3::expr aux_var = context->get_var(context->add_int_var(Z3_IN_PLAJA::auxiliaryVarPrefix));
                return z3::forall(aux_var, z3::select(var_expr, aux_var) == default_value_z3);
            } else {
                return array_value_z3;
            }

        }

        default: PLAJA_ABORT
    }
}

/**********************************************************************************************************************/

bool Z3_IN_PLAJA::StateVarsInZ3::exists_index(VariableIndex_type state_index) const {
    const auto& model_info = get_model_info();
    return model_info.is_location(state_index) ? exists_loc(state_index) : exists(model_info.get_id(state_index));
}

bool Z3_IN_PLAJA::StateVarsInZ3::exists_const(ConstantIdType const_id) const { return parent->has_constant_var(const_id); }

Z3_IN_PLAJA::VarId_type Z3_IN_PLAJA::StateVarsInZ3::const_to_z3(ConstantIdType const_id) const {
    PLAJA_ASSERT(exists_const(const_id))
    return parent->get_constant_var(const_id);
}

/**********************************************************************************************************************/

Z3_IN_PLAJA::VarId_type Z3_IN_PLAJA::StateVarsInZ3::add(VariableID_type state_var, const std::string& postfix) { return add(*get_model().get_variable_by_id(state_var), postfix); }

Z3_IN_PLAJA::VarId_type Z3_IN_PLAJA::StateVarsInZ3::add(const VariableDeclaration& state_var, const std::string& postfix) {
    PLAJA_ASSERT(not exists(state_var.get_id()))

    const auto* decl_type = state_var.get_type();
    PLAJA_ASSERT(decl_type)
    Z3_IN_PLAJA::VarId_type z3_var = Z3_IN_PLAJA::noneVar;

    switch (decl_type->get_kind()) {

        case DeclarationType::Bool: {
            z3_var = context->add_bool_var(state_var.get_name() + postfix);
            break;
        }

        case DeclarationType::Bounded: {
            if (decl_type->is_floating_type()) { z3_var = context->add_real_var(state_var.get_name() + postfix); }
            else {
                PLAJA_ASSERT(decl_type->is_integer_type())
                z3_var = context->add_int_var(state_var.get_name() + postfix);
            }
            break;
        }

        case DeclarationType::Array: {
            const auto* element_type = PLAJA_UTILS::cast_ref<ArrayType>(*decl_type).get_element_type();
            if (element_type->is_bounded_type()) {
                if (element_type->is_floating_type()) { z3_var = context->add_real_array_var(state_var.get_name() + postfix); }
                else {
                    PLAJA_ASSERT(element_type->is_integer_type())
                    z3_var = context->add_int_array_var(state_var.get_name() + postfix);
                }
            } else {
                PLAJA_ASSERT(element_type->is_boolean_type())
                z3_var = context->add_bool_array_var(state_var.get_name() + postfix);
            }
            break;
        }

        default: PLAJA_ABORT
    }

    PLAJA_ASSERT(z3_var != Z3_IN_PLAJA::noneVar)
    stateVarToZ3.emplace(state_var.get_id(), z3_var);
    z3ToStateVar.emplace(z3_var, state_var.get_id());
    auto bound = generate_bound(state_var);
    if (bound) { context->add_bound(z3_var, std::move(*bound)); }
    return z3_var;
}

Z3_IN_PLAJA::VarId_type Z3_IN_PLAJA::StateVarsInZ3::add_loc(VariableIndex_type loc, const std::string& postfix) {
    const auto z3_var = context->add_int_var(locPrefix + std::to_string(loc) + postfix);
    locToZ3.emplace(loc, z3_var);
    z3ToLoc.emplace(z3_var, loc);
    context->add_bound(z3_var, generate_bound_loc(loc));
    return z3_var;
}

void Z3_IN_PLAJA::StateVarsInZ3::set(VariableID_type state_var, Z3_IN_PLAJA::VarId_type z3_var) {
    PLAJA_ASSERT(context->exists(z3_var)) // Variable should be there already.
    STMT_IF_DEBUG(const auto* type = get_model().get_variable_by_id(state_var)->get_type();)
    PLAJA_ASSERT(type->has_boolean_base() or context->has_bound(z3_var)) // Bounds should be there already.
    PLAJA_ASSERT(not type->is_array_type() or context->get_var(z3_var).is_array())
    PLAJA_ASSERT(not type->is_integer_type() or context->get_var(z3_var).is_int())
    PLAJA_ASSERT(not type->is_boolean_type() or context->get_var(z3_var).is_bool())
    PLAJA_ASSERT(not type->is_floating_type() or context->get_var(z3_var).is_real())

    /* Delete old mappings. */
    auto it_state = stateVarToZ3.find(state_var);
    if (it_state != stateVarToZ3.end()) {
        PLAJA_ASSERT(exists_reverse(it_state->second))
        z3ToStateVar.erase(it_state->second);
    }

    auto it_z3 = z3ToStateVar.find(z3_var);
    if (it_z3 != z3ToStateVar.end()) {
        PLAJA_ASSERT(exists(it_z3->second))
        stateVarToZ3.erase(it_z3->second);
    }

    /* */

    PLAJA_ASSERT(not exists(state_var) or not exists_reverse(to_z3(state_var)))
    PLAJA_ASSERT(not exists_reverse(z3_var) or not exists(to_state_var(z3_var)))

    stateVarToZ3[state_var] = z3_var;
    z3ToStateVar[z3_var] = state_var;

}

void Z3_IN_PLAJA::StateVarsInZ3::set_loc(VariableIndex_type loc, Z3_IN_PLAJA::VarId_type z3_var) {
    PLAJA_ASSERT(context->exists(z3_var)) // Variable should be there already.
    PLAJA_ASSERT(context->has_bound(z3_var)) // Bounds should be there already.
    PLAJA_ASSERT(context->get_var(z3_var).is_int()) // Location is integer var.

    /* Delete old mappings. */

    auto it_loc = locToZ3.find(loc);
    if (it_loc != locToZ3.end()) {
        PLAJA_ASSERT(exists_reverse_loc(it_loc->second))
        z3ToLoc.erase(it_loc->second);
    }

    auto it_z3 = z3ToLoc.find(z3_var);
    if (it_z3 != z3ToLoc.end()) {
        PLAJA_ASSERT(exists_loc(it_z3->second))
        locToZ3.erase(it_z3->second);
    }

    /* */

    PLAJA_ASSERT(not exists_loc(loc) or not exists_reverse(loc_to_z3(loc)))
    PLAJA_ASSERT(not exists_reverse_loc(z3_var) or not exists(to_loc(z3_var)))
    locToZ3[loc] = z3_var;
    z3ToLoc[z3_var] = loc;
}

/**********************************************************************************************************************/

const z3::expr& Z3_IN_PLAJA::StateVarsInZ3::to_z3_expr(VariableID_type state_var) const { return context->get_var(to_z3(state_var)); }

const z3::expr& Z3_IN_PLAJA::StateVarsInZ3::loc_to_z3_expr(VariableIndex_type loc) const {
    PLAJA_ASSERT(get_model_info().is_location(loc))
    return context->get_var(loc_to_z3(loc));
}

z3::expr Z3_IN_PLAJA::StateVarsInZ3::var_to_z3_expr_index(VariableIndex_type state_index) const {
    const auto& model_info = get_model_info();
    PLAJA_ASSERT(not model_info.is_location(state_index))
    const auto id = model_info.get_id(state_index);
    if (model_info.is_array(id)) {
        PLAJA_ASSERT(get_model().get_variable_by_id(id)->is_array())
        return z3::select(to_z3_expr(id), context->int_val(PLAJA_UTILS::cast_numeric<PLAJA::integer>(state_index - get_model().get_variable_index_by_id(id))));
    } else {
        return to_z3_expr(id);
    }
}

z3::expr Z3_IN_PLAJA::StateVarsInZ3::to_z3_expr_index(VariableIndex_type state_index) const {
    if (get_model_info().is_location(state_index)) { return loc_to_z3_expr(state_index); }
    else { return var_to_z3_expr_index(state_index); }
}

const z3::expr& Z3_IN_PLAJA::StateVarsInZ3::const_to_z3_expr(ConstantIdType const_id) const { return context->get_var(const_to_z3(const_id)); }

/* */

std::unique_ptr<Z3_IN_PLAJA::StateIndexes> Z3_IN_PLAJA::StateVarsInZ3::cache_state_indexes(bool include_locs) const {
    std::unique_ptr<Z3_IN_PLAJA::StateIndexes> state_indexes(new Z3_IN_PLAJA::StateIndexes(*this));

    for (auto it = get_model_info().stateIndexIterator(include_locs); !it.end(); ++it) {
        const auto state_index = it.state_index();
        if (it.is_location()) {
            if (exists_loc(state_index)) {
                state_indexes->stateIndexes.emplace(state_index, std::make_unique<z3::expr>(loc_to_z3_expr(state_index)));
            }
        } else {
            if (exists(it.id())) {
                state_indexes->stateIndexes.emplace(state_index, std::make_unique<z3::expr>(var_to_z3_expr_index(state_index)));
            }
        }
    }

    return state_indexes;
}

/**********************************************************************************************************************/

// void Z3_IN_PLAJA::StateVarsInZ3::fix(VariableID_type state_var) {
//     PLAJA_ASSERT_EXPECT(not is_fixed(state_var))
//     context->update_bound(to_z3(state_var), generate_fix(*get_model().get_variable_by_id(state_var)));
//     fixed.insert(state_var);
// }

/* */

bool Z3_IN_PLAJA::StateVarsInZ3::has_bound(VariableID_type state_var) const { return context->has_bound(to_z3(state_var)); }

const z3::expr& Z3_IN_PLAJA::StateVarsInZ3::get_bound(VariableID_type state_var) const { return context->get_bound(to_z3(state_var)); }

const z3::expr& Z3_IN_PLAJA::StateVarsInZ3::get_bound_loc(VariableIndex_type loc) const { return context->get_bound(loc_to_z3(loc)); }

const z3::expr& Z3_IN_PLAJA::StateVarsInZ3::get_value_const(ConstantIdType const_id) const { return context->get_bound(const_to_z3(const_id)); }

z3::expr_vector Z3_IN_PLAJA::StateVarsInZ3::bounds_to_vec() const {
    z3::expr_vector vec((*context)());

    for (const auto& loc_z3_var: locToZ3) {
        const auto* bound = context->get_bound_if(loc_z3_var.second);
        if (bound) { vec.push_back(*bound); }
    }

    for (const auto& state_var_z3_var: stateVarToZ3) {
        const auto* bound = context->get_bound_if(state_var_z3_var.second);
        if (bound) { vec.push_back(*bound); }
    }

    return vec;
}

/**********************************************************************************************************************/

void Z3_IN_PLAJA::StateVarsInZ3::extract_solution(const Z3_IN_PLAJA::Solution& solution, StateValues& solution_state, bool include_locs) const {
    const auto& model_info = get_model_info();
    auto& c = (*context)();

    // locations:
    if (include_locs) {
        for (const auto& [loc, var_z3]: locToZ3) {
            solution_state.assign_int<true>(loc, solution.eval(context->get_var(var_z3), true).get_numeral_int());
        }
    }

    // vars:
    for (const auto& [var_id, var_z3]: stateVarToZ3) {
        // if (is_fixed(var_id)) { continue; }

        const auto& var_decl = get_model().get_variable_by_id(var_id);
        const auto& decl_type = *var_decl->get_type();
        const auto state_index = var_decl->get_index();

        if (model_info.is_floating(state_index)) {
            PLAJA_ASSERT(decl_type.has_floating_base())

            if (var_decl->is_array()) {

                const auto array_size = var_decl->get_size();
                auto array_eval = solution.eval(context->get_var(var_z3), true);
                for (std::size_t index = 0; index < array_size; ++index) {
                    solution_state.assign_float<true>(state_index + index, solution.eval(z3::select(array_eval, c.int_val(PLAJA_UTILS::cast_numeric<PLAJA::integer>(index))), true).as_double());
                }

            } else {

                solution_state.assign_float<true>(state_index, solution.eval(context->get_var(var_z3), true).as_double());

            }

        } else {

            PLAJA_ASSERT(model_info.is_integer(var_decl->get_index()))

            if (var_decl->is_array()) {

                const auto array_size = var_decl->get_size();
                auto array_eval = solution.eval(context->get_var(var_z3), true);

                if (PLAJA_UTILS::cast_ref<ArrayType>(decl_type).is_integer_array()) {
                    PLAJA_ASSERT(decl_type.has_integer_base())
                    for (std::size_t index = 0; index < array_size; ++index) {
                        solution_state.assign_int<true>(state_index + index, solution.eval(z3::select(array_eval, c.int_val(PLAJA_UTILS::cast_numeric<PLAJA::integer>(index))), true).get_numeral_int());
                    }
                } else {
                    PLAJA_ASSERT(decl_type.has_boolean_base())
                    for (std::size_t index = 0; index < array_size; ++index) {
                        solution_state.assign_int<true>(state_index + index, solution.eval(z3::select(array_eval, c.int_val(PLAJA_UTILS::cast_numeric<PLAJA::integer>(index))), true).is_true());
                    }
                }

            } else {

                if (decl_type.is_integer_type()) {
                    solution_state.assign_int<true>(state_index, solution.eval(context->get_var(var_z3), true).get_numeral_int());
                } else {
                    PLAJA_ASSERT(decl_type.is_boolean_type())
                    solution_state.assign_int<true>(state_index, solution.eval(context->get_var(var_z3), true).is_true());
                }

            }

        }

    }

    PLAJA_ASSERT(solution_state.is_valid())

}

void Z3_IN_PLAJA::StateVarsInZ3::extract_solution(const Z3_IN_PLAJA::Solution& solution, StateValues& state, VariableIndex_type state_index) const {
    const auto state_index_z3 = to_z3_expr_index(state_index);

    if (get_model_info().is_floating(state_index)) {
        state.assign_float(state_index, solution.eval(state_index_z3, true).as_double());
    } else if (state_index_z3.is_int()) {
        state.assign_int(state_index, solution.eval(state_index_z3, true).get_numeral_int());
    } else {
        PLAJA_ASSERT(state_index_z3.is_bool())
        state.assign_int(state_index, solution.eval(state_index_z3, true).is_true());
    }

    PLAJA_ASSERT(state.is_valid(state_index))

}

z3::expr Z3_IN_PLAJA::StateVarsInZ3::encode_solution(const Z3_IN_PLAJA::Solution& solution, bool include_locs) const {
    const auto& model_info = get_model_info();

    std::vector<z3::expr> solution_vec;
    solution_vec.reserve(model_info.get_state_size());

    // locations:
    if (include_locs) {
        for (const auto& [loc, var_z3]: locToZ3) {
            const auto& loc_z3 = context->get_var(var_z3);
            solution_vec.push_back(loc_z3 == solution.eval(loc_z3, true));
        }
    }

    // vars:
    for (const auto& [var_id, var_z3]: stateVarToZ3) {
        // if (is_fixed(var_id)) { continue; }

        const auto& var_z3_expr = context->get_var(var_z3);
        solution_vec.push_back(var_z3_expr == solution.eval(var_z3_expr, true));
    }

    return Z3_IN_PLAJA::to_conjunction(solution_vec);
}

z3::expr Z3_IN_PLAJA::StateVarsInZ3::encode_state_value(const class StateBase& state, VariableIndex_type state_index) const {
    const auto& model_info = get_model_info();
    auto& c = (*context);

    if (model_info.is_location(state_index)) { return loc_to_z3_expr(state_index) == c.int_val(state.get_int(state_index)); }

    if (model_info.is_floating(state_index)) { return to_z3_expr_index(state_index) == c.real_val(state.get_float(state_index), Z3_IN_PLAJA::floatingPrecision); }

    PLAJA_ASSERT(model_info.is_integer(state_index))

    const auto var_expr = to_z3_expr_index(state_index);

    if (var_expr.is_bool()) { return var_expr == c.bool_val(state.get_int(state_index)); }

    PLAJA_ASSERT(var_expr.is_int())

    return var_expr == c.int_val(state.get_int(state_index));

}

z3::expr Z3_IN_PLAJA::StateVarsInZ3::encode_state(const StateValues& state, bool include_locs) const {
    const auto& model_info = get_model_info();
    auto& c = (*context);

    std::vector<z3::expr> state_vec;
    state_vec.reserve(model_info.get_state_size());

    // locations:
    if (include_locs) {
        for (const auto& [loc, var_z3]: locToZ3) {
            state_vec.push_back(context->get_var(var_z3) == c.int_val(state.get_int(loc)));
        }
    }

    // vars:
    for (const auto& [var_id, var_z3]: stateVarToZ3) {

        // if (is_fixed(var_id)) { continue; }

        const auto& var_decl = get_model().get_variable_by_id(var_id);
        const auto& decl_type = *var_decl->get_type();
        const auto state_index = var_decl->get_index();

        if (model_info.is_floating(state_index)) {
            PLAJA_ASSERT(decl_type.has_floating_base())

            if (var_decl->is_array()) {
                const auto array_size = var_decl->get_size();
                const auto& array_var = context->get_var(var_z3);
                for (std::size_t index = 0; index < array_size; ++index) {
                    state_vec.push_back(z3::select(array_var, c.int_val(PLAJA_UTILS::cast_numeric<PLAJA::integer>(index))) == c.real_val(state.get_float(state_index + index), Z3_IN_PLAJA::floatingPrecision));
                }
            } else {
                state_vec.push_back(context->get_var(var_z3) == c.real_val(state.get_float(state_index), Z3_IN_PLAJA::floatingPrecision));
            }

        } else {

            PLAJA_ASSERT(model_info.is_integer(var_decl->get_index()))

            if (var_decl->is_array()) {
                const auto array_size = var_decl->get_size();
                const auto& array_var = context->get_var(var_z3);

                if (PLAJA_UTILS::cast_ref<ArrayType>(decl_type).is_integer_array()) {
                    PLAJA_ASSERT(decl_type.has_integer_base())
                    for (std::size_t index = 0; index < array_size; ++index) {
                        state_vec.push_back(z3::select(array_var, c.int_val(PLAJA_UTILS::cast_numeric<PLAJA::integer>(index))) == c.int_val(state.get_int(state_index + index)));
                    }
                } else {
                    PLAJA_ASSERT(decl_type.has_boolean_base())
                    for (std::size_t index = 0; index < array_size; ++index) {
                        state_vec.push_back(z3::select(array_var, c.int_val(PLAJA_UTILS::cast_numeric<PLAJA::integer>(index))) == c.bool_val(state.get_int(state_index + index)));
                    }
                }

            } else {

                if (decl_type.is_integer_type()) {
                    state_vec.push_back(context->get_var(var_z3) == c.int_val(state.get_int(state_index)));
                } else {
                    state_vec.push_back(state.get_int(state_index) ? context->get_var(var_z3) : !context->get_var(var_z3));
                }

            }

        }

    }

    return Z3_IN_PLAJA::to_conjunction(state_vec);
}

/**********************************************************************************************************************/

const z3::expr& Z3_IN_PLAJA::StateVarsInZ3::LocIterator::loc_z3_expr() const { return stateVars->context->get_var(loc_z3()); }

const z3::expr& Z3_IN_PLAJA::StateVarsInZ3::VarIterator::var_z3_expr() const { return stateVars->context->get_var(var_z3()); }

/**********************************************************************************************************************/

Z3_IN_PLAJA::StateIndexes::StateIndexes(const StateVarsInZ3& state_vars_to_z3):
    parent(&state_vars_to_z3) {}

Z3_IN_PLAJA::StateIndexes::~StateIndexes() = default;

Z3_IN_PLAJA::StateIndexes::StateIndexes(const Z3_IN_PLAJA::StateIndexes& other):
    parent(other.parent) {
    stateIndexes.reserve(other.parent->size());
    for (const auto& [state_index, expr_z3]: other.stateIndexes) { stateIndexes.emplace(state_index, std::make_unique<z3::expr>(*expr_z3)); }
}

void Z3_IN_PLAJA::StateIndexes::remove(VariableIndex_type state_index) {
    PLAJA_ASSERT(exists(state_index))
    stateIndexes.erase(state_index);
}

/* */

void Z3_IN_PLAJA::StateIndexes::extract_solution(const Z3_IN_PLAJA::Solution& solution, StateValues& solution_state) const {
    const auto& model_info = parent->get_model_info();
    // auto solution_state = model_info.get_initial_values();

    for (const auto& [state_index, state_expr]: stateIndexes) {

        // if (not model_info.is_location(state_index) and parent->is_fixed(model_info.get_id(state_index))) { continue; }

        if (model_info.is_floating(state_index)) {
            PLAJA_ASSERT(state_expr->is_real())
            solution_state.assign_float<true>(state_index, solution.eval(*state_expr, true).as_double());
        } else {
            PLAJA_ASSERT(model_info.is_integer(state_index))
            if (state_expr->is_int()) {
                solution_state.assign_int<true>(state_index, solution.eval(*state_expr, true).get_numeral_int());
            } else {
                PLAJA_ASSERT(state_expr->is_bool())
                solution_state.assign_int<true>(state_index, solution.eval(*state_expr, true).is_true());
            }
        }

    }

}

z3::expr Z3_IN_PLAJA::StateIndexes::encode_solution(const Z3_IN_PLAJA::Solution& solution) const {

    std::vector<z3::expr> solution_vec;
    solution_vec.reserve(stateIndexes.size());

    for (const auto& [state_index, state_expr]: stateIndexes) {

        // if (not model_info.is_location(state_index) and parent->is_fixed(model_info.get_id(state_index))) { continue; }

        solution_vec.push_back(*state_expr == solution.eval(*state_expr, true));

    }

    return Z3_IN_PLAJA::to_conjunction(solution_vec);
}

// Note: This might be used to silently handle out-of-array-bounds indexes in predicates over an initial state (setting the predicate to false).
z3::expr Z3_IN_PLAJA::StateIndexes::encode_state(const StateValues& state) const {
    const auto& model_info = parent->get_model_info();
    auto& c = parent->get_context();

    std::vector<z3::expr> state_vec;
    state_vec.reserve(state.size());

    for (const auto& [state_index, state_expr]: stateIndexes) {

        // if (not model_info.is_location(state_index) and parent->is_fixed(model_info.get_id(state_index))) { continue; }

        if (model_info.is_floating(state_index)) {
            PLAJA_ASSERT(state_expr->is_real())
            state_vec.push_back(*state_expr == c.real_val(state.get_float(state_index), Z3_IN_PLAJA::floatingPrecision));
        } else {
            PLAJA_ASSERT(model_info.is_integer(state_index))
            if (state_expr->is_int()) {
                state_vec.push_back(*state_expr == c.int_val(state.get_int(state_index)));
            } else {
                PLAJA_ASSERT(state_expr->is_bool())
                state_vec.push_back(state.get_int(state_index) ? *state_expr : !(*state_expr));
            }
        }

    }

    return Z3_IN_PLAJA::to_conjunction(state_vec);
}


/**********************************************************************************************************************/

Z3_IN_PLAJA::VarId_type Z3_IN_PLAJA::StateVarsInZ3::add(const ConstantDeclaration& constant, Z3_IN_PLAJA::Context& context) {

    static const std::string postfix("_const");

    const auto* decl_type = constant.get_type();
    PLAJA_ASSERT(decl_type)
    Z3_IN_PLAJA::VarId_type z3_var; // NOLINT(*-init-variables)
    STMT_IF_DEBUG(z3_var = Z3_IN_PLAJA::noneVar)

    switch (decl_type->get_kind()) {

        case DeclarationType::Bool: {
            z3_var = context.add_bool_var(constant.get_name() + postfix);
            context.add_bound(z3_var, constant.get_value()->evaluate_integer_const() ? context.get_var(z3_var) : !context.get_var(z3_var));
            break;
        }

        case DeclarationType::Bounded: {

            if (decl_type->is_floating_type()) {
                z3_var = context.add_real_var(constant.get_name() + postfix);
                context.add_bound(z3_var, context.get_var(z3_var) == context.real_val(constant.get_value()->evaluate_floating_const(), Z3_IN_PLAJA::floatingPrecision));
            } else {
                PLAJA_ASSERT(decl_type->is_integer_type())
                z3_var = context.add_int_var(constant.get_name() + postfix);
                context.add_bound(z3_var, context.get_var(z3_var) == context.int_val(constant.get_value()->evaluate_integer_const()));
            }

            break;
        }

        case DeclarationType::Array: {

            const auto* array_value = PLAJA_UTILS::cast_ptr<ArrayValueExpression>(constant.get_value());
            const auto arr_size = array_value->get_number_elements();

            std::vector<z3::expr> array_values_z3;
            array_values_z3.reserve(arr_size);

            const auto* element_type = PLAJA_UTILS::cast_ref<ArrayType>(*decl_type).get_element_type();

            if (element_type->is_bounded_type()) {

                if (element_type->is_floating_type()) {
                    z3_var = context.add_real_array_var(constant.get_name() + postfix);
                    const auto& var_expr = context.get_var(z3_var);

                    PLAJA::integer index(0);
                    for (auto it = array_value->init_element_it(); !it.end(); ++it) {
                        array_values_z3.push_back(z3::select(var_expr, context.int_val(index++)) == context.real_val(it->evaluate_floating_const(), Z3_IN_PLAJA::floatingPrecision));
                    }

                } else {
                    PLAJA_ASSERT(element_type->is_integer_type())
                    z3_var = context.add_int_array_var(constant.get_name() + postfix);
                    const auto& var_expr = context.get_var(z3_var);

                    PLAJA::integer index(0);
                    for (auto it = array_value->init_element_it(); !it.end(); ++it) {
                        array_values_z3.push_back(z3::select(var_expr, context.int_val(index++)) == context.int_val(it->evaluate_integer_const()));
                    }

                }

            } else {
                PLAJA_ASSERT(element_type->is_boolean_type())
                z3_var = context.add_bool_array_var(constant.get_name() + postfix);
                const auto& var_expr = context.get_var(z3_var);

                PLAJA::integer index(0);
                for (auto it = array_value->init_element_it(); !it.end(); ++it) {
                    array_values_z3.push_back(it->evaluate_integer_const() ? z3::select(var_expr, context.int_val(index++)) : !z3::select(var_expr, context.int_val(index)));
                }

            }

            context.add_bound(z3_var, Z3_IN_PLAJA::to_conjunction(array_values_z3));

            break;
        }

        default: PLAJA_ABORT
    }

    PLAJA_ASSERT(z3_var != Z3_IN_PLAJA::noneVar)
    return z3_var;
}