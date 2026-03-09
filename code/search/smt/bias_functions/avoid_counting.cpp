//
// Created by avoid_counting on 2024.
//

#include "avoid_counting.h"
#include "../../../search/smt/vars_in_z3.h"
#include "../../../search/smt/visitor/to_z3_visitor.h"

/**
* @brief Constructs a function that computes the number of [avoid] satisfying assignments.
*/
z3::expr AvoidCounting::get_avoid_count_bias(Expression* avoid, const std::shared_ptr<Z3_IN_PLAJA::Context>& ctx, const std::shared_ptr<const ModelZ3>& model) {
    // init bias function
    z3::expr bias = ctx->operator()().int_val(0);

    auto state_vars = model->get_state_vars();
    auto z3_avoid = to_z3_condition(*avoid, state_vars);

    if (z3_avoid.is_app()) {

        return ite(z3_avoid, ctx->operator()().int_val(1), ctx->operator()().int_val(0));
    }

    // If the condition is a conjunction (AND expression)
    // todo: replace with recurisve call (similar to distance_to_condition).
    if (z3_avoid.is_and()) {
        unsigned num_args = z3_avoid.num_args();

        // Iterate over each clause
        for (unsigned i = 0; i < num_args; ++i) {
            z3::expr conjunct = z3_avoid.arg(i);

            // Sum the number of satisfying assignments
            // If the conjunct is true, add 1 to the bias otherwise add 0.
            bias = bias + ite(conjunct, ctx->operator()().int_val(1), ctx->operator()().int_val(0));
        }
    }

    return bias;

}