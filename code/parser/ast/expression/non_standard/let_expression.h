//
// This file is part of the PlaJA code base.
// Copyright (c) (2019 - 2021) Marcel Vinzent.
// See README.md in the top-level directory for licensing information.
//

#ifndef PLAJA_LET_EXPRESSION_H
#define PLAJA_LET_EXPRESSION_H

#include "../../iterators/ast_iterator.h"
#include "../../forward_ast.h"
#include "../expression.h"

class LetExpression: public Expression {

private:
    std::vector<std::unique_ptr<FreeVariableDeclaration>> freeVariables;
    std::unique_ptr<Expression> expression;

public:
    LetExpression();
    ~LetExpression() override;
    DELETE_CONSTRUCTOR(LetExpression)

    /* Static. */

    [[nodiscard]] static const std::string& get_op_string();

    /* Construction. */

    void reserve(std::size_t free_variables_cap);

    void add_free_variable(std::unique_ptr<FreeVariableDeclaration>&& free_variable);

    /* Setter. */

    void set_free_variable(std::unique_ptr<FreeVariableDeclaration>&& free_variable, std::size_t index);

    inline void set_expression(std::unique_ptr<Expression>&& expression_m) { expression = std::move(expression_m); }

    // Getter. */

    [[nodiscard]] inline std::size_t get_number_of_free_variables() const { return freeVariables.size(); }

    [[nodiscard]] inline const FreeVariableDeclaration* get_free_variable(std::size_t index) const {
        PLAJA_ASSERT(index < freeVariables.size())
        return freeVariables[index].get();
    }

    [[nodiscard]] inline FreeVariableDeclaration* get_free_variable(std::size_t index) {
        PLAJA_ASSERT(index < freeVariables.size())
        return freeVariables[index].get();
    }

    [[nodiscard]] inline const Expression* get_expression() const { return expression.get(); }

    [[nodiscard]] inline Expression* get_expression() { return expression.get(); }

    [[nodiscard]] inline AstConstIterator<FreeVariableDeclaration> init_free_variable_iterator() const { return AstConstIterator(freeVariables); }

    [[nodiscard]] inline AstIterator<FreeVariableDeclaration> init_free_variable_iterator() { return AstIterator(freeVariables); }

    // Override. */
    PLAJA::integer evaluateInteger(const StateBase* state) const override;
    [[nodiscard]] bool is_constant() const override;
    bool equals(const Expression* exp) const override;
    [[nodiscard]] std::size_t hash() const override;
    const DeclarationType* determine_type() override;
    [[nodiscard]] std::unique_ptr<Expression> deepCopy_Exp() const override;
    [[nodiscard]] std::unique_ptr<Expression> move_exp() override;
    void accept(AstVisitor* ast_visitor) override;
    void accept(AstVisitorConst* ast_visitor) const override;

    /* */

    [[nodiscard]] std::unique_ptr<LetExpression> deep_copy() const;

    [[nodiscard]] std::unique_ptr<LetExpression> move();

};

#endif //PLAJA_LET_EXPRESSION_H
