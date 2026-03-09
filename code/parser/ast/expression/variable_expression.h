//
// This file is part of the PlaJA code base.
// Copyright (c) (2019 - 2019) Marcel Vinzent.
// See README.md in the top-level directory for licensing information.
//

#ifndef PLAJA_VARIABLE_EXPRESSION_H
#define PLAJA_VARIABLE_EXPRESSION_H

#include <memory>
#include "../variable_declaration.h"
#include "lvalue_expression.h"

/** Reference to a variable (declaration). */
class VariableExpression final: public LValueExpression {

private:
    const VariableDeclaration* decl;

public:
    explicit VariableExpression(const VariableDeclaration& decl);
    ~VariableExpression() override;
    DELETE_CONSTRUCTOR(VariableExpression)

    /* Setter. */

    void set_var(const VariableDeclaration& var_decl);

    /* Getter. */

    [[nodiscard]] const VariableDeclaration* get_decl() const { return decl; }

    [[nodiscard]] const std::string& get_name() const;

    [[nodiscard]] inline VariableID_type get_id() const {
        PLAJA_ASSERT(decl)
        return decl->get_id();
    }

    [[nodiscard]] inline VariableIndex_type get_index() const {
        PLAJA_ASSERT(decl)
        return decl->get_index();
    }

    /* Override. */
    void assign(StateBase& target, const StateBase* source, const Expression* value) const override; // as LValue
    PLAJA::integer access_integer(const StateBase& target, const StateBase* source) const override;
    PLAJA::floating access_floating(const StateBase& target, const StateBase* source) const override;
    [[nodiscard]] VariableID_type get_variable_id() const override;
    [[nodiscard]] VariableIndex_type get_variable_index(const StateBase* state) const override;
    using LValueExpression::get_variable_index;
    PLAJA::integer evaluateInteger(const StateBase* state) const override;
    PLAJA::floating evaluateFloating(const StateBase* state) const override;
    [[nodiscard]] bool is_constant() const override;
    bool equals(const Expression* exp) const override;
    [[nodiscard]] std::size_t hash() const override;
    const DeclarationType* determine_type() override;
    void accept(AstVisitor* ast_visitor) override;
    void accept(AstVisitorConst* ast_visitor) const override;
    [[nodiscard]] std::unique_ptr<LValueExpression> deep_copy_lval_exp() const override;
    //
    [[nodiscard]] std::unique_ptr<VariableExpression> deep_copy() const;

};

#endif //PLAJA_VARIABLE_EXPRESSION_H
