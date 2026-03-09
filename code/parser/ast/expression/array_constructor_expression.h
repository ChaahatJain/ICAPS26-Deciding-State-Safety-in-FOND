//
// This file is part of the PlaJA code base.
// Copyright (c) (2019 - 2019) Marcel Vinzent.
// See README.md in the top-level directory for licensing information.
//

#ifndef PLAJA_ARRAY_CONSTRUCTOR_EXPRESSION_H
#define PLAJA_ARRAY_CONSTRUCTOR_EXPRESSION_H

#include <memory>
#include "expression.h"

class FreeVariableDeclaration;

class ArrayConstructorExpression final: public Expression {
private:
    std::unique_ptr<FreeVariableDeclaration> freeVar;
    std::unique_ptr<Expression> length;
    std::unique_ptr<Expression> evalExp;

public:
    ArrayConstructorExpression();
    ~ArrayConstructorExpression() override;

    /* Setter. */

    void set_freeVar(std::unique_ptr<FreeVariableDeclaration>&& free_var);

    inline void set_length(std::unique_ptr<Expression>&& len) { length = std::move(len); }

    inline void set_evalExp(std::unique_ptr<Expression>&& eval_exp) { evalExp = std::move(eval_exp); }

    /* Getter. */

    [[nodiscard]] inline FreeVariableDeclaration* get_freeVar() { return freeVar.get(); }

    [[nodiscard]] inline const FreeVariableDeclaration* get_freeVar() const { return freeVar.get(); }

    inline Expression* get_length() { return length.get(); }

    [[nodiscard]] inline const Expression* get_length() const { return length.get(); }

    inline Expression* get_evalExp() { return evalExp.get(); }

    [[nodiscard]] inline const Expression* get_evalExp() const { return evalExp.get(); }

    /* Auxiliary. */
    [[nodiscard]] const std::string& get_free_var_name() const;

    /* Override. */
    [[nodiscard]] bool is_constant() const override;
    bool equals(const Expression* exp) const override;
    [[nodiscard]] std::size_t hash() const override;
    const DeclarationType* determine_type() override;
    void accept(AstVisitor* ast_visitor) override;
    void accept(AstVisitorConst* ast_visitor) const override;
    [[nodiscard]] std::unique_ptr<Expression> deepCopy_Exp() const override;
};

#endif //PLAJA_ARRAY_CONSTRUCTOR_EXPRESSION_H
