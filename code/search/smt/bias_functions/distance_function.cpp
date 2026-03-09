//
// Created by Daniel Sherbakov on 2024.
//

#include "distance_function.h"

#include "../../../exception/not_supported_exception.h"
#include "../../../search/smt/vars_in_z3.h"
#include "../context_z3.h"
#include "../visitor/to_z3_visitor.h"
#include "bias_to_z3.h"
#include "wp_bias.h"

namespace Bias {

    DistanceFunction::DistanceFunction(const Expression& target_condition,
        const ModelZ3& model,
        const DistanceFunctionType type):
        model(model),
        target_condition(model.share_context()->operator()().int_val(0)),
        distance_function(model.share_context()->operator()().int_val(0)) {
        // construct the distance function
        const auto& vars = model.get_state_vars();
        this->target_condition = to_z3_condition(target_condition, vars);
        switch (type) {
            case DistanceFunctionType::DistanceToTarget:
                distance_function = BiasToZ3::distance_to_condition(this->target_condition, model.share_context());
                break;
            case DistanceFunctionType::DistanceToTargetWP:
                // distance_function = WeakestPreconditionBias::get_wp_bias(target_condition, model.share_context(), model);
                    throw NotSupportedException("Distance function type not supported.");
                break;
            default:;
        }
    }

    int DistanceFunction::evaluate(const State& state) const {
        const auto state_vars = model.get_state_vars();
        const auto ctx = model.share_context();
        z3::expr evaluated_function = distance_function;// copy function.
        // std::cout << "Evaluating distance function: " << evaluated_function.to_string() << std::endl;

        // iterate over variables and substitute them with their values.
        /*for (auto it_var = vars.varIterator(); !it_var.end(); ++it_var) {*/
        for (VariableIndex_type state_index = 0; state_index < state.size(); ++state_index) {
            if (not state_vars.exists_index(state_index)) {
                PLAJA_ASSERT(state_vars.is_loc(state_index))
                continue;
            }
            // substitution initialization.
            z3::expr_vector from(ctx->operator()());
            z3::expr_vector to(ctx->operator()());

            // get variable and its value.
            // const auto var_id = it_var.var_z3();
            // std::cout << "Variable id: " << var_id << std::endl;
            const z3::expr& var_expr = state_vars.to_z3_expr_index(state_index);
            // std::cout << "var_expr: " << var_expr.to_string() << std::endl;
            const auto var_val = state.get_int(state_index);
            // std::cout << "Variable value: " << var_val << std::endl;
            auto val_expr = ctx->operator()().int_val(var_val);

            // substitute variable with its value.
            from.push_back(var_expr);
            to.push_back(val_expr);
            evaluated_function = evaluated_function.substitute(from, to);
            // std::cout << "Evaluated function: " << evaluated_function.to_string() << std::endl;
        }
        // check if the result is an integer.
        if (const auto res = evaluated_function.simplify(); res.is_int()) {
            return res.get_numeral_int();
        }
        throw std::runtime_error("Distance function evaluation failed.");
    }

}// namespace Bias