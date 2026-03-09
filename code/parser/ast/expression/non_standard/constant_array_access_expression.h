//
// This file is part of the PlaJA code base.
// Copyright (c) (2019 - 2024) Marcel Vinzent.
// See README.md in the top-level directory for licensing information.
//

#ifndef PLAJA_CONSTANT_ARRAY_ACCESS_EXPRESSION_H
#define PLAJA_CONSTANT_ARRAY_ACCESS_EXPRESSION_H

#include "../../forward_ast.h"
#include "../expression.h"

class ConstantArrayAccessExpression final: public Expression {

private:
    const ConstantDeclaration* accessedArray;
    std::unique_ptr<Expression> index;

public:
    explicit ConstantArrayAccessExpression(const ConstantDeclaration* accessed_array);
    ~ConstantArrayAccessExpression() override;
    DELETE_CONSTRUCTOR(ConstantArrayAccessExpression)

    /* Setter. */

    inline void set_accessed_array(const ConstantDeclaration* accessed_array) { accessedArray = accessed_array; }

    inline void set_index(std::unique_ptr<Expression>&& index_m) { index = std::move(index_m); }

    /* Getter. */

    [[nodiscard]] inline const ConstantDeclaration* get_accessed_array() const { return accessedArray; }

    [[nodiscard]] inline Expression* get_index() { return index.get(); }

    [[nodiscard]] inline const Expression* get_index() const { return index.get(); }

    /* Short-cut. */

    [[nodiscard]] const std::string& get_name() const;

    [[nodiscard]] ConstantIdType get_id() const;

    [[nodiscard]] const Expression* get_array_value() const;

    [[nodiscard]] std::size_t get_array_size() const;

    /* Override. */

    PLAJA::integer evaluateInteger(const StateBase* state) const override;

    PLAJA::floating evaluateFloating(const StateBase* state) const override;

    [[nodiscard]] bool is_constant() const override;

    bool equals(const Expression* exp) const override;

    [[nodiscard]] std::size_t hash() const override;

    const DeclarationType* determine_type() override;

    void accept(AstVisitor* ast_visitor) override;

    void accept(AstVisitorConst* ast_visitor) const override;

    [[nodiscard]] std::unique_ptr<Expression> deepCopy_Exp() const override;

    [[nodiscard]] std::unique_ptr<Expression> move_exp() override;

    /* Auxiliary */

    [[nodiscard]] std::unique_ptr<ConstantArrayAccessExpression> deep_copy() const;

    [[nodiscard]] std::unique_ptr<ConstantArrayAccessExpression> move();

};

#endif //PLAJA_CONSTANT_ARRAY_ACCESS_EXPRESSION_H
