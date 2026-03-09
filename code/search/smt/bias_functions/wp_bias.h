//
// Created by wp_bias on 2024.
//

#ifndef WP_BIAS_H
#define WP_BIAS_H

#include "../../../parser/ast/expression/binary_op_expression.h"
#include "../../../parser/ast/expression/expression.h"
#include "../context_z3.h"
#include "../model/model_z3.h"
#include <z3++.h>

class WeakestPreconditionBias {
public:
    static z3::expr get_wp_bias(const Expression* avoid, const std::shared_ptr<Z3_IN_PLAJA::Context>& ctx, const std::shared_ptr<const ModelZ3>& model);

private:
    static z3::expr compute_wp(const Expression* assignment, VariableID_type var_id, z3::expr* avoid, const std::shared_ptr<Z3_IN_PLAJA::Context>& ctx, const std::shared_ptr<const ModelZ3>& model);
    static bool isAffecting(const Update& update, const Expression& expression);
    static std::vector<z3::expr> get_wps_of_operator(ActionOp& action_op, const Expression* avoid, const std::shared_ptr<Z3_IN_PLAJA::Context>& ctx, const std::shared_ptr<const ModelZ3>& model);
    static z3::expr extract_guards(ActionOp& action_op, const std::shared_ptr<Z3_IN_PLAJA::Context>& ctx, const std::shared_ptr<const ModelZ3>& model);
    static BinaryOpExpression::BinaryOp get_op(const std::string& op);
};

#endif//WP_BIAS_H
