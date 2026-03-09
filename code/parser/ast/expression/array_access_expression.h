//
// This file is part of the PlaJA code base.
// Copyright (c) (2019 - 2019) Marcel Vinzent.
// See README.md in the top-level directory for licensing information.
//

#ifndef PLAJA_ARRAY_ACCESS_EXPRESSION_H
#define PLAJA_ARRAY_ACCESS_EXPRESSION_H

#include <memory>
#include "variable_expression.h"

class ArrayAccessExpression final: public LValueExpression {
private:
    std::unique_ptr<VariableExpression> accessedArray; // Currently restricted, in general arbitrary expression of array type.
    std::unique_ptr<Expression> index;

    /* Auxiliary for runtime checks. */
    void check_offset(PLAJA::integer offset) const;

public:
    ArrayAccessExpression();
    ~ArrayAccessExpression() override;
    DELETE_CONSTRUCTOR(ArrayAccessExpression)

    /* Setter. */

    std::unique_ptr<VariableExpression> set_accessedArray(std::unique_ptr<VariableExpression>&& accessed_array);

    std::unique_ptr<Expression> set_index(std::unique_ptr<Expression>&& index);

    /* Special aux. */

    std::unique_ptr<VariableExpression> set_accessedArray(const VariableDeclaration& var_decl);

    std::unique_ptr<Expression> set_index(PLAJA::integer index);

    /* Getter. */

    [[nodiscard]] inline Expression* get_accessedArray() { return accessedArray.get(); }

    [[nodiscard]] inline const Expression* get_accessedArray() const { return accessedArray.get(); }

    [[nodiscard]] inline Expression* get_index() { return index.get(); }

    [[nodiscard]] inline const Expression* get_index() const { return index.get(); }

    /* Override. */
    void assign(StateBase& target, const StateBase* source, const Expression* value) const override;
    PLAJA::integer access_integer(const StateBase& target, const StateBase* source) const override;
    PLAJA::floating access_floating(const StateBase& target, const StateBase* source) const override;
    [[nodiscard]] VariableID_type get_variable_id() const override;
    [[nodiscard]] const Expression* get_array_index() const override;
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

    /* Auxiliary. */

    [[nodiscard]] std::unique_ptr<ArrayAccessExpression> deep_copy() const;

};

#endif //PLAJA_ARRAY_ACCESS_EXPRESSION_H
