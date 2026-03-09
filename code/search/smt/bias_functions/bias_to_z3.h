//
// Created by Daniel Sherbakov in 2024.
//

#ifndef BIAS_TO_Z3_H
#define BIAS_TO_Z3_H

#include "../../../parser/ast/expression/special_cases/linear_expression.h"
#include "../../../parser/ast/expression/binary_op_expression.h"
#include "../../../parser/ast/expression/expression.h"
#include "../context_z3.h"
#include "../model/model_z3.h"
#include "z3++.h"

//TODO: Currently only supports conjunctions of linear expressions, need to add support for more complex expressions and boolean variables.
class BiasToZ3 {

public:
    struct BoundInfo {
        Z3_IN_PLAJA::VarId_type var_id;
        BinaryOpExpression::BinaryOp op;
        int constant;
    };

    static z3::expr distance_to_condition(Expression* avoid, const std::shared_ptr<Z3_IN_PLAJA::Context>& ctx);
    static z3::expr distance_to_condition(const z3::expr& condition, const std::shared_ptr<Z3_IN_PLAJA::Context>& ctx);
    static z3::expr distance_to_expression(const Expression* condition, const std::shared_ptr<const ModelZ3>& model);
    static z3::expr distance_to_z3bound(const z3::expr& bound, const std::shared_ptr<Z3_IN_PLAJA::Context>& ctx);
    static z3::expr distance_to_boundary(const z3::expr& var, const BinaryOpExpression::BinaryOp op, int constant, const std::shared_ptr<Z3_IN_PLAJA::Context>& ctx);
    static z3::expr get_not_unsafe_start(const Expression* start, const std::shared_ptr<const ModelZ3>& model); // workaround for getting a different start condition

private:
    static std::vector<BoundInfo> get_info_state_condition(StateConditionExpression* cond);
    static std::vector<BoundInfo> get_info_narray(NaryExpression* conjunction);
    static BoundInfo get_info_linear(LinearExpression* lin);
    static BoundInfo get_info_binary(BinaryOpExpression* bin);
    static BoundInfo get_info_unary(UnaryOpExpression* uni);

};

#endif//BIAS_TO_Z3_H
