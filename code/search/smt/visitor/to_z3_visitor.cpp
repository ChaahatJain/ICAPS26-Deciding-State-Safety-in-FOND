//
// This file is part of the PlaJA code base.
// Copyright (c) (2019 - 2020) Marcel Vinzent.
// See README.md in the top-level directory for licensing information.
//

#include <z3++.h>
#include "to_z3_visitor.h"
#include "../../../exception/not_implemented_exception.h"
#include "../../../parser/ast/expression/non_standard/constant_array_access_expression.h"
#include "../../../parser/ast/expression/non_standard/let_expression.h"
#include "../../../parser/ast/expression/non_standard/location_value_expression.h"
#include "../../../parser/ast/expression/non_standard/state_condition_expression.h"
#include "../../../parser/ast/expression/non_standard/state_values_expression.h"
#include "../../../parser/ast/expression/non_standard/states_values_expression.h"
#include "../../../parser/ast/expression/special_cases/nary_expression.h"
#include "../../../parser/ast/expression/special_cases/linear_expression.h"
#include "../../../parser/ast/expression/array_access_expression.h"
#include "../../../parser/ast/expression/bool_value_expression.h"
#include "../../../parser/ast/expression/free_variable_expression.h"
#include "../../../parser/ast/expression/integer_value_expression.h"
#include "../../../parser/ast/expression/ite_expression.h"
#include "../../../parser/ast/expression/real_value_expression.h"
#include "../../../parser/ast/expression/unary_op_expression.h"
#include "../../../parser/ast/non_standard/free_variable_declaration.h"
#include "../../../parser/ast/type/bounded_type.h"
#include "../../../parser/visitor/extern/to_normalform.h"
#include "../../../search/information/model_information.h"
#include "../../../search/states/state_values.h"
#include "../utils/to_z3_expr_splits.h"
#include "../context_z3.h"
#include "../vars_in_z3.h"

ToZ3Visitor::ToZ3Visitor(const Z3_IN_PLAJA::StateVarsInZ3& var_to_z3):
    context(&var_to_z3.get_context())
    , varToZ3(&var_to_z3)
    , rltExpr(*context)
    , freeVars() {

}

ToZ3Visitor::~ToZ3Visitor() = default;

/* */

ToZ3Expr ToZ3Visitor::to_z3(const Expression& exp) {
    exp.accept(this);
    return rltExpr;
}

z3::expr ToZ3Visitor::to_z3_condition(const Expression& exp) { return to_z3(exp).to_conjunction(); }

ToZ3Expr ToZ3Visitor::to_z3(const Expression& exp, const Z3_IN_PLAJA::StateVarsInZ3& var_to_z3) {
    ToZ3Visitor to_z3_visitor(var_to_z3);
    return to_z3_visitor.to_z3(exp);
}

z3::expr ToZ3Visitor::to_z3_condition(const Expression& exp, const Z3_IN_PLAJA::StateVarsInZ3& var_to_z3) {
    PLAJA_ASSERT(PLAJA_UTILS::is_dynamic_type<StatesValuesExpression>(exp) or exp.get_type()->is_boolean_type())
    ToZ3Visitor to_z3_visitor(var_to_z3);
    return to_z3_visitor.to_z3_condition(exp);
}

std::unique_ptr<ToZ3ExprSplits> ToZ3Visitor::to_z3_condition_splits(const Expression& exp, const Z3_IN_PLAJA::StateVarsInZ3& var_to_z3) {
    // Deprecated: // PLAJA_ASSERT(not PLAJA_UTILS::is_dynamic_type<StateConditionExpression>(exp) or not var_to_z3.has_loc()) // Not implemented.
    std::unique_ptr<ToZ3ExprSplits> to_z3_expr_splits(new ToZ3ExprSplits(TO_NORMALFORM::normalize_and_split(exp, false), var_to_z3.get_context()));
    /* Set splits in z3. */
    for (const auto& split: to_z3_expr_splits->splits) { to_z3_expr_splits->splitsInZ3.push_back(Z3_IN_PLAJA::to_z3_condition(*split, var_to_z3)); }
    /* */
    return to_z3_expr_splits;
}

/* */

void ToZ3Visitor::visit(const FreeVariableDeclaration* var_decl) {
    const auto* type = var_decl->get_type();
    JANI_ASSERT(type)

    if (type->is_boolean_type()) {

        rltExpr.set(context->get_var(context->add_bool_var(Z3_IN_PLAJA::freeVarPrefix)));

    } else if (type->is_bounded_type() && type->is_integer_type()) {

        /* Generate bounds TODO issue: for free variables lower_bound < upper_bound is not checked. */
        const z3::expr& var = context->get_var(context->add_int_var(Z3_IN_PLAJA::freeVarPrefix)); // cache variable
        rltExpr.set(var); // Provide bounds with variable.
        type->accept(this);
        //
        rltExpr.conjunct_auxiliary(rltExpr.release()); // add bounds to aux
        rltExpr.set(var); // Actually set variable.

    } else { PLAJA_ABORT }

}

/* */

void ToZ3Visitor::visit(const ArrayAccessExpression* exp) {
    JANI_ASSERT(exp->get_accessedArray() && exp->get_index())

    exp->get_accessedArray()->accept(this);
    const z3::expr tmp_array(rltExpr.release());
    exp->get_index()->accept(this);
    const z3::expr tmp_index(rltExpr.release());
    rltExpr.set(z3::select(tmp_array, tmp_index));

    /* Set array bounds if necessary. */
    if (not exp->get_index()->is_constant()) { // Constant indexes are checked to be within bounds at initialization.
        const z3::expr array_size_expr = context->int_val(PLAJA_UTILS::cast_numeric<PLAJA::integer>(varToZ3->get_model_info().get_array_size(exp->get_variable_id())));
        rltExpr.conjunct_auxiliary((0 <= tmp_index) && (tmp_index < array_size_expr));
    }
}

void ToZ3Visitor::visit(const BoolValueExpression* exp) { rltExpr.set(context->bool_val(exp->get_value())); }

void ToZ3Visitor::visit(const BinaryOpExpression* exp) {
    JANI_ASSERT(exp->get_left() && exp->get_right())
    exp->get_left()->accept(this);
    const z3::expr tmp_left(rltExpr.release());
    exp->get_right()->accept(this);
    const z3::expr tmp_right(rltExpr.release());

    switch (exp->get_op()) {

        case BinaryOpExpression::OR: {
            rltExpr.set(tmp_left || tmp_right);
            break;
        }

        case BinaryOpExpression::AND: {
            rltExpr.set(tmp_left && tmp_right);
            break;
        }

        case BinaryOpExpression::IMPLIES: {
            rltExpr.set(z3::implies(tmp_left, tmp_right));
            break;
        }

        case BinaryOpExpression::EQ: {
            rltExpr.set(tmp_left == tmp_right);
            break;
        }

        case BinaryOpExpression::NE: {
            rltExpr.set(tmp_left != tmp_right);
            break;
        }

        case BinaryOpExpression::LT: {
            rltExpr.set(tmp_left < tmp_right);
            break;
        }

        case BinaryOpExpression::LE: {
            rltExpr.set(tmp_left <= tmp_right);
            break;
        }

        case BinaryOpExpression::GT: {
            rltExpr.set(tmp_left > tmp_right);
            break;
        }

        case BinaryOpExpression::GE: {
            rltExpr.set(tmp_left >= tmp_right);
            break;
        }

        case BinaryOpExpression::PLUS: {
            rltExpr.set(tmp_left + tmp_right);
            break;
        }

        case BinaryOpExpression::MINUS: {
            rltExpr.set(tmp_left - tmp_right);
            break;
        }

        case BinaryOpExpression::TIMES: {
            rltExpr.set(tmp_left * tmp_right);
            break;
        }

        case BinaryOpExpression::DIV: {
            rltExpr.set((tmp_left.is_int() ? z3::to_real(tmp_left) : tmp_left) / (tmp_right.is_int() ? z3::to_real(tmp_right) : tmp_right)); // JANI division is always continuous.
            break;
        }

        case BinaryOpExpression::POW: {
            rltExpr.set(z3::pw(tmp_left, tmp_right));
            break;
        }

        case BinaryOpExpression::LOG: { throw NotImplementedException(__PRETTY_FUNCTION__); }

        case BinaryOpExpression::MIN: {
            rltExpr.set(z3::min(tmp_left, tmp_right));
            break;
        }

        case BinaryOpExpression::MAX: {
            rltExpr.set(z3::max(tmp_left, tmp_right));
            break;
        }

        case BinaryOpExpression::MOD: {
            rltExpr.set(z3::mod(tmp_left, tmp_right));
            break;
        }
    }

}

void ToZ3Visitor::visit(const ConstantExpression*) { PLAJA_ABORT } // In contrast to array access on constants, this should be optimized out.

void ToZ3Visitor::visit(const FreeVariableExpression* exp) { rltExpr.set((freeVars).at(exp->get_id())); }

void ToZ3Visitor::visit(const IntegerValueExpression* exp) { rltExpr.set(context->int_val(exp->get_value())); }

void ToZ3Visitor::visit(const ITE_Expression* exp) {
    JANI_ASSERT(exp->get_condition() and exp->get_consequence() and exp->get_alternative())

    exp->get_condition()->accept(this);
    const z3::expr cond_tmp(rltExpr.release());

    exp->get_consequence()->accept(this);
    const z3::expr cons_tmp(rltExpr.release());

    exp->get_alternative()->accept(this);
    const z3::expr alt_tmp(rltExpr.release());

    rltExpr.set(z3::ite(cond_tmp, cons_tmp, alt_tmp));
}

#ifndef NDEBUG

#include "../../../utils/floating_utils.h"

#endif

void ToZ3Visitor::visit(const RealValueExpression* exp) {
    PLAJA_ASSERT(PLAJA_FLOATS::equal(PLAJA_UTILS::precision_to_tolerance(Z3_IN_PLAJA::floatingPrecision), PLAJA::floatingPrecision * 0.1))
    PLAJA_ASSERT(PLAJA_UTILS::tolerance_to_precision(PLAJA::floatingPrecision) == Z3_IN_PLAJA::floatingPrecision - 1)
    rltExpr.set(context->real_val(exp->get_value(), Z3_IN_PLAJA::floatingPrecision)); // so far I have not found a better way, note that Z3 does reject/warn if we directly give the value
}

void ToZ3Visitor::visit(const UnaryOpExpression* exp) {
    JANI_ASSERT(exp->get_operand())
    exp->get_operand()->accept(this);
    const z3::expr tmp(rltExpr.release());

    switch (exp->get_op()) {
        case UnaryOpExpression::NOT: {
            rltExpr.set(!tmp);
            break;
        }

        case UnaryOpExpression::FLOOR: {
            // aux variable
            const z3::expr aux_var = context->get_var(context->add_int_var(Z3_IN_PLAJA::auxiliaryVarPrefix));
            // aux expression: floor(sub_expr) = aux_var, i.e., aux_var <= sub_expr && sub_expr <= aux_var + 1
            rltExpr.set(aux_var);
            rltExpr.conjunct_auxiliary(aux_var <= tmp && tmp <= aux_var + 1);
            break;
        }

        case UnaryOpExpression::CEIL: {
            const z3::expr aux_var = context->get_var(context->add_int_var(Z3_IN_PLAJA::auxiliaryVarPrefix));
            rltExpr.set(aux_var);
            rltExpr.conjunct_auxiliary(aux_var - 1 <= tmp && tmp <= aux_var);
            break;
        }

        case UnaryOpExpression::ABS: {
            static bool print(true);
            if (print) {
                print = false;
                PLAJA_LOG(PLAJA_UTILS::to_red_string("Using z3::abs for which unstable behavior has been observed."))
                PLAJA_ABORT
            }
            rltExpr.set(z3::abs(tmp));
            break;
        }

        case UnaryOpExpression::SGN:
        case UnaryOpExpression::TRC: { throw NotImplementedException(__PRETTY_FUNCTION__); }
    }
}

void ToZ3Visitor::visit(const VariableExpression* exp) { rltExpr.set(varToZ3->to_z3_expr(exp->get_id())); }

/* Non-standard. */

void ToZ3Visitor::visit(const ConstantArrayAccessExpression* exp) {
    JANI_ASSERT(exp->get_accessed_array() and exp->get_index())

    exp->get_index()->accept(this);
    const z3::expr tmp_index(rltExpr.release());
    rltExpr.set(z3::select(varToZ3->const_to_z3_expr(exp->get_id()), tmp_index));

    /* Set array bounds if necessary. */
    if (not exp->get_index()->is_constant()) { // Constant indexes are checked to be within bounds at initialization.
        const z3::expr array_size_expr = context->int_val(PLAJA_UTILS::cast_numeric<PLAJA::integer>(exp->get_array_size()));
        rltExpr.conjunct_auxiliary((0 <= tmp_index) && (tmp_index < array_size_expr));
    }
}

void ToZ3Visitor::visit(const LetExpression* exp) {
    //* Add free variables. */
    for (auto it = exp->init_free_variable_iterator(); !it.end(); ++it) {
        it->accept(this);
        freeVars.emplace(it->get_id(), rltExpr.release());
        // *bounds* are added to auxiliaries silently
    }
    /* Actual expression. */
    exp->get_expression()->accept(this);
    /* Delete free variables. */
    for (auto it = exp->init_free_variable_iterator(); !it.end(); ++it) { freeVars.erase(it->get_id()); }
}

void ToZ3Visitor::visit(const StateConditionExpression* exp) {
    std::vector<z3::expr> state_condition;

    /* Locations. */

    if (varToZ3->has_loc()) {
        for (auto loc_it = exp->init_loc_value_iterator(); !loc_it.end(); ++loc_it) {
            state_condition.push_back(varToZ3->loc_to_z3_expr(loc_it->get_location_index()) == context->int_val(loc_it->get_location_value()));
        }
    }


    /* Constraint. */

    const auto* constraint = exp->get_constraint();

    if (constraint) {
        constraint->accept(this);
        state_condition.push_back(rltExpr.release());
    }

    PLAJA_ASSERT_EXPECT(varToZ3->has_loc() or not state_condition.empty())

    rltExpr.set(state_condition.empty() ? context->bool_val(true) : Z3_IN_PLAJA::to_conjunction(state_condition));
}

void ToZ3Visitor::visit(const StateValuesExpression* exp) {
    const auto state_values = exp->construct_state_values(varToZ3->get_model_info().get_initial_values());
    rltExpr.set(varToZ3->encode_state(state_values, true));
}

void ToZ3Visitor::visit(const StatesValuesExpression* exp) {
    PLAJA_ASSERT(exp->get_size_states_values() > 0)

    const auto& model_info = varToZ3->get_model_info();
    const auto& initial_state = model_info.get_initial_values();

    /* Factor out unused state-value pairs. */
    auto cached_state_indexes = varToZ3->cache_state_indexes(true);
    auto cached_state_indexes_default(*cached_state_indexes);
    const auto used_state_indexes = exp->retrieve_state_indexes();

    for (auto it = model_info.stateIndexIterator(true); !it.end(); ++it) {
        const auto state_index = it.state_index();

        PLAJA_ASSERT(cached_state_indexes->exists(state_index) == cached_state_indexes_default.exists(state_index))

        if (not cached_state_indexes->exists(state_index)) { continue; }

        if (used_state_indexes.count(state_index)) { cached_state_indexes_default.remove(it.state_index()); }
        else { cached_state_indexes->remove(state_index); }
    }

    /* Disjunction over used state-value pairs. */
    std::vector<z3::expr> states_values_in_z3;
    states_values_in_z3.reserve(exp->get_size_states_values() + states_values_in_z3.size()); // Latter is in case we add model's initial state here.
    // Appears to be beneficial for BMC on racetrack (while disabling the conjunction in ModelZ3Base::generate_state_condition ...).
    // states_values_in_z3.push_back(varToZ3->encode_state(initial_state));
    //
    for (auto it = exp->init_state_values_iterator(); !it.end(); ++it) {
        states_values_in_z3.push_back(cached_state_indexes->encode_state(it->construct_state_values(initial_state)));
    }

    /* Conjunction with unused state-value pairs. */
    rltExpr.set(cached_state_indexes_default.encode_state(initial_state) && Z3_IN_PLAJA::to_disjunction(states_values_in_z3));

}

/* Special case. */

void ToZ3Visitor::visit(const NaryExpression* expr) {

    std::vector<z3::expr> subs_z3;
    subs_z3.reserve(expr->get_size());

    for (auto it = expr->iterator(); !it.end(); ++it) {
        it->accept(this);
        subs_z3.push_back(rltExpr.release());
    }

    switch (expr->get_op()) {

        case BinaryOpExpression::OR: {
            rltExpr.set(Z3_IN_PLAJA::to_disjunction(subs_z3));
            break;
        }

        case BinaryOpExpression::AND: {
            rltExpr.set(Z3_IN_PLAJA::to_conjunction(subs_z3));
            break;
        }

        default: { PLAJA_ABORT }

    }

}

void ToZ3Visitor::visit(const LinearExpression* exp) {
    /* Special case. */
    if (exp->get_number_addends() == 0) {
        if (exp->is_linear_assignment()) {
            exp->get_scalar()->accept(this);
            return;
        } else {
            rltExpr.set(context->bool_val(exp->evaluate_integer_const()));
            return;
        }
    }

    /* Linear sum. */
    auto addends_it = exp->addendIterator();
    z3::expr sum = addends_it.factor()->evaluates_integer_const(1) ?
                   varToZ3->to_z3_expr(addends_it.var()->get_variable_id()) :
                   (to_z3(*addends_it.factor()).release()) * varToZ3->to_z3_expr(addends_it.var()->get_variable_id());
    /* */
    for (++addends_it; !addends_it.end(); ++addends_it) {
        PLAJA_ASSERT(not addends_it.factor()->evaluates_integer_const(0))
        if (addends_it.factor()->evaluates_integer_const(1)) {
            sum = sum + varToZ3->to_z3_expr(addends_it.var()->get_variable_id());
        } else {
            sum = sum + (to_z3(*addends_it.factor()).release()) * varToZ3->to_z3_expr(addends_it.var()->get_variable_id());
        }
    }

    /* Operator. */
    switch (exp->get_op()) {

        case BinaryOpExpression::BinaryOp::PLUS: {
            rltExpr.set(exp->get_scalar()->evaluates_integer_const(0) ? sum : sum + to_z3(*exp->get_scalar()).release());
            break;
        }

        case BinaryOpExpression::BinaryOp::LT: {
            rltExpr.set(sum < to_z3(*exp->get_scalar()).release());
            break;
        }

        case BinaryOpExpression::BinaryOp::LE: {
            rltExpr.set(sum <= to_z3(*exp->get_scalar()).release());
            break;
        }

        case BinaryOpExpression::BinaryOp::EQ: {
            rltExpr.set(sum == to_z3(*exp->get_scalar()).release());
            break;
        }

        case BinaryOpExpression::BinaryOp::NE: {
            rltExpr.set(sum != to_z3(*exp->get_scalar()).release());
            break;
        }

        case BinaryOpExpression::BinaryOp::GE: {
            rltExpr.set(sum >= to_z3(*exp->get_scalar()).release());
            break;
        }

        case BinaryOpExpression::BinaryOp::GT: {
            rltExpr.set(sum > to_z3(*exp->get_scalar()).release());
            break;
        }

        default: { PLAJA_ABORT }

    }
}

/* */

void ToZ3Visitor::visit(const BoundedType* type) {
    JANI_ASSERT(type->get_lower_bound() && type->get_upper_bound())
    PLAJA_ASSERT(type->is_integer_type())
    const z3::expr var(rltExpr.release()); // Cache variable.
    /* Lower bound. */
    type->get_lower_bound()->accept(this);
    const z3::expr lower_bound = rltExpr.release() <= var;
    /* Upper bound. */
    type->get_upper_bound()->accept(this);
    rltExpr.set(lower_bound && var <= rltExpr.release());
}

/**********************************************************************************************************************/

/* Extern. */

namespace Z3_IN_PLAJA {

    ToZ3Expr to_z3(const Expression& exp, const Z3_IN_PLAJA::StateVarsInZ3& var_to_z3) { return ToZ3Visitor::to_z3(exp, var_to_z3); }

    z3::expr to_z3_condition(const Expression& exp, const Z3_IN_PLAJA::StateVarsInZ3& var_to_z3) { return ToZ3Visitor::to_z3_condition(exp, var_to_z3); }

    std::unique_ptr<ToZ3ExprSplits> to_z3_condition_splits(const Expression& exp, const Z3_IN_PLAJA::StateVarsInZ3& var_to_z3) { return ToZ3Visitor::to_z3_condition_splits(exp, var_to_z3); }

    template<typename Z3ExprVec_t, bool disjunctive>
    inline z3::expr concatenate_z3_expressions(const Z3ExprVec_t& vec) {
        auto it = vec.begin();
        auto end = vec.end();
        z3::expr e = *it;
        for (++it; it != end; ++it) {
            if constexpr (disjunctive) { e = e || *it; }
            else { e = e && *it; }
        }
        return e;
    }

    z3::expr to_conjunction(const z3::expr_vector& vec) {
        if (vec.empty()) { return { vec.ctx() }; }
        else { return z3::mk_and(vec); /*concatenate_z3_expressions<z3::expr_vector, false>(vec);*/ }
    }

    z3::expr to_conjunction(const std::vector<z3::expr>& vec) {
        PLAJA_ASSERT(not vec.empty())
        return concatenate_z3_expressions<std::vector<z3::expr>, false>(vec);
    }

    z3::expr to_disjunction(const z3::expr_vector& vec) {
        if (vec.empty()) { return { vec.ctx() }; }
        else { return z3::mk_or(vec); /*concatenate_z3_expressions<z3::expr_vector, true>(vec);*/ }
    }

    z3::expr to_disjunction(const std::vector<z3::expr>& vec) {
        PLAJA_ASSERT(not vec.empty())
        return concatenate_z3_expressions<std::vector<z3::expr>, true>(vec);
    }

}