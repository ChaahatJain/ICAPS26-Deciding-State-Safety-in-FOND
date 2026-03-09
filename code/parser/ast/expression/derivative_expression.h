//
// This file is part of the PlaJA code base.
// Copyright (c) (2019 - 2019) Marcel Vinzent.
// See README.md in the top-level directory for licensing information.
//

#ifndef PLAJA_DERIVATIVE_EXPRESSION_H
#define PLAJA_DERIVATIVE_EXPRESSION_H

#include "variable_expression.h"

class DerivativeExpression final: public Expression {

private:
    VariableExpression var;

public:
    explicit DerivativeExpression(const VariableDeclaration& decl);
    ~DerivativeExpression() override;
    DELETE_CONSTRUCTOR(DerivativeExpression)

    /* Setter. */
    inline void set_var(const VariableDeclaration& decl) { var.set_var(decl); }

    /* Getter. */
    [[nodiscard]] inline const std::string& get_name() const { return var.get_name(); }

    [[nodiscard]] inline VariableID_type get_id() const { return var.get_id(); }

    [[nodiscard]] inline VariableIndex_type get_index() const { return var.get_index(); }

    /* Override. */
    [[nodiscard]] bool is_constant() const override;
    [[nodiscard]] bool is_proposition() const override;
    const DeclarationType* determine_type() override;
    void accept(AstVisitor* ast_visitor) override;
    void accept(AstVisitorConst* ast_visitor) const override;
    [[nodiscard]] std::unique_ptr<Expression> deepCopy_Exp() const override;
};

#endif //PLAJA_DERIVATIVE_EXPRESSION_H
