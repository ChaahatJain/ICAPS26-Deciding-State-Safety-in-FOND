//
// This file is part of the PlaJA code base.
// Copyright (c) (2019 - 2023) Marcel Vinzent.
// See README.md in the top-level directory for licensing information.
//

#ifndef PLAJA_TO_LINEAR_EXPRESSION_H
#define PLAJA_TO_LINEAR_EXPRESSION_H

#include <memory>

class Expression;
class LinearExpression;


namespace TO_LINEAR_EXP {

    extern bool as_linear_exp(const Expression& expr);
    extern bool is_linear_exp(const Expression& expr);
    extern bool is_linear_sum(const Expression& expr);
    extern bool is_linear_assignment(const Expression& expr);
    extern bool is_linear_constraint(const Expression& expr);

    extern std::unique_ptr<LinearExpression> to_linear_sum(const Expression& expr);
    extern std::unique_ptr<LinearExpression> to_linear_constraint(const Expression& expr);

    extern std::unique_ptr<LinearExpression> to_linear_exp(const Expression& expr);
    extern std::unique_ptr<Expression> to_exp(const Expression& expr);

    extern std::unique_ptr<Expression> to_standard(const Expression& expr);
    extern std::unique_ptr<Expression> to_standard_predicate(const Expression& expr);

}

#endif //PLAJA_TO_LINEAR_EXPRESSION_H
