//
// Created by Daniel Sherbakov in 2024.
//

#include "bias_to_z3.h"

#include "../../../parser/ast/expression/special_cases/nary_expression.h"
#include "../../../parser/ast/expression/variable_expression.h"
#include "../../../exception/not_implemented_exception.h"
#include "../../../parser/ast/expression/constant_value_expression.h"
#include "../../../parser/ast/expression/non_standard/state_condition_expression.h"
#include "../../../parser/ast/expression/special_cases/linear_expression.h"
#include "../../../parser/ast/expression/unary_op_expression.h"

#include "../../../search/smt/vars_in_z3.h"

#include "../../../search/smt/visitor/to_z3_visitor.h"

#include <set>



/**
* @brief Constructs a function that computes the distance of a state to the avoid condition.
* @returns a z3 expression representing the bias function.
*/
z3::expr BiasToZ3::distance_to_condition(Expression* avoid, const std::shared_ptr<Z3_IN_PLAJA::Context>& ctx) {
    if (not dynamic_cast<StateConditionExpression*>(avoid)) {
        throw std::runtime_error("Expected StateConditionExpression");
    }
    auto* state_condition = dynamic_cast<StateConditionExpression*>(avoid);
    auto bound_infos = get_info_state_condition(state_condition);

    // create the distance function
    z3::expr bias = ctx->operator()().int_val(0);
    for (const auto& [id, op, constant] : bound_infos) {
        const z3::expr& var = ctx->get_var(id);
        auto constant_expr = ctx->int_val(constant);
        bias = bias + distance_to_boundary(var, op, constant, ctx);
    }
    return bias;
}


z3::expr BiasToZ3::distance_to_boundary(const z3::expr& var, const  BinaryOpExpression::BinaryOp op, const int constant, const std::shared_ptr<Z3_IN_PLAJA::Context>& ctx) {
    z3::expr distance = ctx->operator()().int_val(0);
    auto constant_expr = ctx->int_val(constant);
    switch (op) {
        case BinaryOpExpression::EQ: {
            distance = distance + z3::abs(var - constant);
            break;
        }
        case BinaryOpExpression::LT: {
            constant_expr = ctx->operator()().int_val(constant+1);
            distance = distance + max(var - constant_expr, ctx->operator()().int_val(0));
            break;
        }
        case BinaryOpExpression::LE: {
            distance = distance + max(var - constant_expr, ctx->operator()().int_val(0));
            break;
        }
        case BinaryOpExpression::GT: {
            constant_expr = ctx->operator()().int_val(constant+1);
            distance = distance + max(constant_expr - var, ctx->operator()().int_val(0));
            break;
        }
        case BinaryOpExpression::GE: {
            distance = distance + max(constant_expr - var, ctx->operator()().int_val(0));
            break;
        }
        default: {
            throw std::runtime_error("Unsupported operator");
        }
    }
    return distance;
}


std::vector<BiasToZ3::BoundInfo> BiasToZ3::get_info_state_condition(StateConditionExpression* cond) {
    if (auto arr = dynamic_cast<NaryExpression*>(cond->get_constraint())) {
        return get_info_narray(arr);
    } if (auto lin = dynamic_cast<LinearExpression*>(cond->get_constraint())) {
        return {get_info_linear(lin)};
    }
    const std::string expr_type = typeid(cond->get_constraint()).name();
    throw std::runtime_error("Expected LinearExpression or NaryExpression but was: " + expr_type);
}

std::vector<BiasToZ3::BoundInfo> BiasToZ3::get_info_narray(NaryExpression* conjunction) {
    if (const auto op = conjunction->get_op(); op != BinaryOpExpression::AND) {
        throw std::runtime_error("Expected Conjunction");
    }
    std::vector<BoundInfo> infos;
    for (int index = 0; index < conjunction->get_size(); ++index) {
        auto clause = conjunction->get_sub(index);
        if (auto* lin = dynamic_cast<LinearExpression*>(clause)) {
            infos.push_back(get_info_linear(lin));
        } else if (auto* arr = dynamic_cast<NaryExpression*>(clause)) {
            infos.insert(std::end(infos), std::begin(get_info_narray(arr)), std::end(get_info_narray(arr)));
        } else if (auto* bin = dynamic_cast<BinaryOpExpression*>(clause)) {
            infos.push_back(get_info_binary(bin));
        } else if (auto* uni = dynamic_cast<UnaryOpExpression*>(clause)) {
            infos.push_back(get_info_unary(uni));
        } else {
            const std::string clause_type = typeid(clause).name();
            throw std::runtime_error("Expression of type" + clause_type + "not covered");
        }
    }
    return infos;
}

BiasToZ3::BoundInfo BiasToZ3::get_info_linear(LinearExpression* lin) {
    if (not lin->is_bound()) { throw std::runtime_error("Expected bound"); }
    return {lin->addendIterator().var_id(), lin->get_op(), lin->get_scalar()->evaluateInteger(nullptr)};
}

BiasToZ3::BoundInfo BiasToZ3::get_info_binary(BinaryOpExpression* bin) {
    throw NotImplementedException("get_info_binary not implemented");
}

BiasToZ3::BoundInfo BiasToZ3::get_info_unary(UnaryOpExpression* uni) {
    throw NotImplementedException("get_info_unary not implemented");
}

/**
* This is a temporary workaround for getting a different start condition.
 */
z3::expr BiasToZ3::get_not_unsafe_start(const Expression* start,const std::shared_ptr<const ModelZ3>& model) {
    auto state_vars = model->get_state_vars();
    auto ctx = model->share_context();
    auto z3_start = to_z3_condition(*start, state_vars);
    z3::expr neg_start = !z3_start;
    return neg_start;
}

/**
 * recursively constructs the distance function to a z3 condition.<br>
 *  [condition] = (v ⋈ c) | (condition1 ∧ condition2) | (condition1 ∨ condition2)<br>
 *  return (ite (v ⋈ c) 0 |v - c|) | (sum of distances) | (min of distances)
 */
z3::expr BiasToZ3::distance_to_condition(const z3::expr& condition, const std::shared_ptr<Z3_IN_PLAJA::Context>& ctx) {
    // Base case: check if `condition` is an atomic comparison (v ⋈ c)
    if (condition.is_app()) {
        auto op = condition.decl().decl_kind();
        if (op == Z3_OP_EQ || op == Z3_OP_LE || op == Z3_OP_GE || op == Z3_OP_LT || op == Z3_OP_GT) {
            return distance_to_z3bound(condition, ctx);
        }
    }

    // If it's a conjunction (∧), return the sum of distances
    if (condition.is_and()) {
        z3::expr total_distance = ctx->int_val(0);
        for (unsigned i = 0; i < condition.num_args(); ++i) {
            total_distance = (total_distance + distance_to_condition(condition.arg(i), ctx)).simplify();
        }
        return total_distance.simplify();
    }

    // If it's a disjunction (∨), return the minimum of distances
    if (condition.is_or()) {
        z3::expr min_distance = distance_to_condition(condition.arg(0), ctx);  // Start with the first argument
        for (unsigned i = 1; i < condition.num_args(); ++i) {
            min_distance = z3::min(min_distance, distance_to_condition(condition.arg(i), ctx));
        }
        return min_distance;
    }

    // If not a simple bound, cnf, or dnf then throw an error.
    throw std::runtime_error("Unsupported expression in distance_to_condition()");
}

z3::expr BiasToZ3::distance_to_expression(const Expression* condition, const std::shared_ptr<const ModelZ3>& model) {
    auto state_vars = model->get_state_vars();
    auto ctx = model->share_context();
    auto z3_condition = to_z3_condition(*condition, state_vars);
    return distance_to_condition(z3_condition, ctx);
}

z3::expr BiasToZ3::distance_to_z3bound(const z3::expr& bound, const std::shared_ptr<Z3_IN_PLAJA::Context>& ctx) {
    // Extract the variable and constant
    const z3::expr var = bound.arg(0);       // the variable v
    const z3::expr constant = bound.arg(1);  // the constant c
    z3::expr dist = ctx->int_val(0); // placeholder

    const auto real_valued = var.get_sort().is_real();
    const z3::expr epsilon = real_valued ? ctx->real_val(0.00001, Z3_IN_PLAJA::floatingPrecision) : ctx->int_val(1);

    // Get the kind of the expression (e.g., <=, >=, <, >, ==)
    switch (bound.decl().decl_kind()) {
        case Z3_OP_EQ: { dist = z3::abs(var - constant); break; }
        case Z3_OP_LE: { dist = max((var - constant), dist); break; }
        case Z3_OP_GE: { dist = max((constant - var), dist); break; }
        case Z3_OP_LT: { dist = z3::ite(var < constant,dist,var-constant+epsilon); break; }
        case Z3_OP_GT: { dist = z3::ite(var > constant, dist, constant-var+epsilon); break; }
        default: {throw std::runtime_error("Unsupported operator: expected EQ, LE, or GE but was: " + bound.to_string());
        }
    }
    return dist.simplify();
}