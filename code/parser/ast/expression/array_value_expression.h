//
// This file is part of the PlaJA code base.
// Copyright (c) (2019 - 2019) Marcel Vinzent.
// See README.md in the top-level directory for licensing information.
//

#ifndef PLAJA_ARRAY_VALUE_EXPRESSION_H
#define PLAJA_ARRAY_VALUE_EXPRESSION_H

#include "expression.h"
#include "../iterators/ast_iterator.h"

class ArrayValueExpression final: public Expression {
private:
    std::vector<std::unique_ptr<Expression>> elements;
public:
    ArrayValueExpression();
    ~ArrayValueExpression() override;
    DELETE_CONSTRUCTOR(ArrayValueExpression)

    /* Construction. */

    void reserve(std::size_t elements_cap);

    void add_element(std::unique_ptr<Expression>&& element);

    /* Setter. */
    inline void set_element(std::unique_ptr<Expression>&& element, std::size_t index) {
        PLAJA_ASSERT(index < elements.size())
        elements[index] = std::move(element);
    }

    /* Getter. */

    [[nodiscard]] inline std::size_t get_number_elements() const { return elements.size(); }

    inline Expression* get_element(std::size_t index) {
        PLAJA_ASSERT(index < elements.size())
        return elements[index].get();
    }

    [[nodiscard]] inline const Expression* get_element(std::size_t index) const {
        PLAJA_ASSERT(index < elements.size())
        return elements[index].get();
    }

    [[nodiscard]] inline AstConstIterator<Expression> init_element_it() const { return AstConstIterator(elements); }

    [[nodiscard]] inline AstIterator<Expression> init_element_it() { return AstIterator(elements); }

    /* Override. */

    std::vector<PLAJA::integer> evaluateIntegerArray(const StateBase* state) const override;

    std::vector<PLAJA::floating> evaluateFloatingArray(const StateBase* state) const override;

    [[nodiscard]] bool is_constant() const override;

    bool equals(const Expression* exp) const override;

    [[nodiscard]] std::size_t hash() const override;

    const DeclarationType* determine_type() override;

    void accept(AstVisitor* ast_visitor) override;

    void accept(AstVisitorConst* ast_visitor) const override;

    [[nodiscard]] std::unique_ptr<Expression> deepCopy_Exp() const override;

    [[nodiscard]] std::unique_ptr<ArrayValueExpression> deep_copy() const;
};

#endif //PLAJA_ARRAY_VALUE_EXPRESSION_H
