//
// Created by avoid_counting on 2024.
//

#ifndef AVOID_COUNTING_H
#define AVOID_COUNTING_H

#include "../../../parser/ast/expression/expression.h"
#include "../context_z3.h"
#include "../model/model_z3.h"
#include <z3++.h>


/**
 * Bias function that maximizes the number of facts that agree with the avoid condition.
 */
class AvoidCounting {
public:
    static z3::expr get_avoid_count_bias(Expression* avoid, const std::shared_ptr<Z3_IN_PLAJA::Context>& ctx, const std::shared_ptr<const ModelZ3>& model);
};



#endif //AVOID_COUNTING_H
