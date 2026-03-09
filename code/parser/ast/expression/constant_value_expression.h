//
// This file is part of the PlaJA code base.
// Copyright (c) (2019 - 2019) Marcel Vinzent.
// See README.md in the top-level directory for licensing information.
//

#ifndef PLAJA_CONSTANT_VALUE_EXPRESSION_H
#define PLAJA_CONSTANT_VALUE_EXPRESSION_H

#include "expression.h"

/**
 * Constants in AST but also used for dynamic-typ values during search.
 */
class ConstantValueExpression: public Expression {
public:
    ConstantValueExpression();
    ~ConstantValueExpression() override = 0;

    // to value
    [[nodiscard]] virtual bool is_boolean() const;
    [[nodiscard]] virtual bool is_integer() const;
    [[nodiscard]] virtual bool is_floating() const;
    // arithmetic
    [[nodiscard]] virtual std::unique_ptr<ConstantValueExpression> add(const ConstantValueExpression& addend) const;
    [[nodiscard]] virtual std::unique_ptr<ConstantValueExpression> multiply(const ConstantValueExpression& factor) const;
    // round
    [[nodiscard]] virtual std::unique_ptr<ConstantValueExpression> round() const;
    [[nodiscard]] virtual std::unique_ptr<ConstantValueExpression> floor() const;
    [[nodiscard]] virtual std::unique_ptr<ConstantValueExpression> ceil() const;

    // override:
    const DeclarationType* determine_type() override;
    [[nodiscard]] bool is_constant() const override;
    [[nodiscard]] std::unique_ptr<Expression> deepCopy_Exp() const override;
    //
    [[nodiscard]] virtual std::unique_ptr<ConstantValueExpression> deepCopy_ConstantValueExp() const = 0;
};

#endif //PLAJA_CONSTANT_VALUE_EXPRESSION_H
