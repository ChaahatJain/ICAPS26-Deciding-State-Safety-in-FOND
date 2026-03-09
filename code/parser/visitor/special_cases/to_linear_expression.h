//
// This file is part of the PlaJA code base.
// Copyright (c) (2019 - 2020) Marcel Vinzent.
// See README.md in the top-level directory for licensing information.
//

#ifndef PLAJA_TO_LINEAR_EXPRESSION_H
#define PLAJA_TO_LINEAR_EXPRESSION_H

#include <memory>
#include "../ast_visitor_const.h"

class ConstantValueExpression;
namespace LINEAR_CONSTRAINTS_CHECKER { struct Specification; }

class ToLinearExp {

private:
    std::unique_ptr<LINEAR_CONSTRAINTS_CHECKER::Specification> specification;
    std::unique_ptr<LinearExpression> linearExpression;

    std::unique_ptr<ConstantValueExpression> globalAddendFactor; // position of addend in case of linear constraints
    std::unique_ptr<ConstantValueExpression> globalScalarFactor; // position of scalar in case of linear constraints

    void set_right_of_op();
    void set_left_of_op();
    void scale_factor(const ConstantValueExpression& scale);
    void unset_global_factor();

    void to_scalar(const Expression& expr);
    void to_addend(const Expression& expr);
    void to_sum(const Expression& expr);
    void to_constraint(const Expression& expr);

    ToLinearExp();
public:
    ~ToLinearExp();
    DELETE_CONSTRUCTOR(ToLinearExp)

    static LINEAR_CONSTRAINTS_CHECKER::Specification get_specification();

    static std::unique_ptr<LinearExpression> to_linear_sum(const Expression& expr);

    static std::unique_ptr<LinearExpression> to_linear_constraint(const Expression& expr);

    static std::unique_ptr<LinearExpression> to_linear_exp(const Expression& expr);

};

// extern:

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
