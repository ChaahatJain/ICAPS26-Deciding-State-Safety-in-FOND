//
// This file is part of the PlaJA code base.
// Copyright (c) (2019 - 2021) Marcel Vinzent.
// See README.md in the top-level directory for licensing information.
//

#ifndef PLAJA_VARIABLE_VALUE_EXPRESSION_H
#define PLAJA_VARIABLE_VALUE_EXPRESSION_H

#include "../expression.h"
#include "../forward_expression.h"

class VariableValueExpression final: public Expression {
private:
    std::unique_ptr<VariableExpression> var;
    std::unique_ptr<Expression> val;

public:
    VariableValueExpression();
    ~VariableValueExpression() override;
    DELETE_CONSTRUCTOR(VariableValueExpression)

    [[nodiscard]] std::unique_ptr<VariableValueExpression> deepCopy() const;

    /* Setter. */

    void set_var(std::unique_ptr<VariableExpression>&& var_);

    inline void set_val(std::unique_ptr<Expression>&& val_) { val = std::move(val_); }

    /* Getter. */

    [[nodiscard]] inline const VariableExpression* get_var() const { return var.get(); }

    inline VariableExpression* get_var() { return var.get(); }

    [[nodiscard]] inline const Expression* get_val() const { return val.get(); }

    inline Expression* get_val() { return val.get(); }

    /* Override. */

    PLAJA::integer evaluateInteger(const StateBase* state) const override;
    void assign(StateBase& target, const StateBase* source) const;
    bool equals(const Expression* exp) const override;
    [[nodiscard]] std::size_t hash() const override;
    void accept(AstVisitor* ast_visitor) override;
    void accept(AstVisitorConst* ast_visitor) const override;
    [[nodiscard]] std::unique_ptr<Expression> deepCopy_Exp() const override;

};

#endif //PLAJA_VARIABLE_VALUE_EXPRESSION_H
