//
// This file is part of the PlaJA code base.
// Copyright (c) (2019 - 2023) Marcel Vinzent.
// See README.md in the top-level directory for licensing information.
//

#ifndef PLAJA_NARY_EXPRESSION_H
#define PLAJA_NARY_EXPRESSION_H

#include <list>
#include "special_case_expression.h"
#include "../../iterators/ast_iterator.h"
#include "../binary_op_expression.h"

class NaryExpression: public SpecialCaseExpression {

private:
    BinaryOpExpression::BinaryOp op;
    std::vector<std::unique_ptr<Expression>> subExpressions;

public:
    explicit NaryExpression(BinaryOpExpression::BinaryOp op);
    ~NaryExpression() override;
    DELETE_CONSTRUCTOR(NaryExpression)

    bool static is_op(BinaryOpExpression::BinaryOp op);

    /* Construction. */
    void reserve(std::size_t cap);
    void add_sub(std::unique_ptr<Expression>&& sub);

    /* Setter. */

    inline void set_op(BinaryOpExpression::BinaryOp op_new) { op = op_new; }

    inline void set_sub(std::unique_ptr<Expression>&& sub, std::size_t index) {
        PLAJA_ASSERT(index < subExpressions.size())
        subExpressions[index] = std::move(sub);
    }

    /* Getter. */

    [[nodiscard]] inline BinaryOpExpression::BinaryOp get_op() const { return op; }

    [[nodiscard]] inline std::size_t get_size() const { return subExpressions.size(); }

    [[nodiscard]] inline const Expression* get_sub(std::size_t index) const {
        PLAJA_ASSERT(index < get_size())
        return subExpressions[index].get();
    }

    [[nodiscard]] inline Expression* get_sub(std::size_t index) {
        PLAJA_ASSERT(index < get_size())
        return subExpressions[index].get();
    }

    [[nodiscard]] inline AstConstIterator<Expression> iterator() const { return AstConstIterator(subExpressions); }

    [[nodiscard]] inline AstIterator<Expression> iterator() { return AstIterator(subExpressions); }

    /* Override. */
    [[nodiscard]] std::unique_ptr<Expression> to_standard() const override;
    PLAJA::integer evaluateInteger(const StateBase* state) const override;
    [[nodiscard]] bool is_constant() const override;
    bool equals(const Expression* exp) const override;
    [[nodiscard]] std::size_t hash() const override;
    void accept(AstVisitor* ast_visitor) override;
    void accept(AstVisitorConst* ast_visitor) const override;
    const DeclarationType* determine_type() override;
    [[nodiscard]] std::unique_ptr<Expression> deepCopy_Exp() const override;
    [[nodiscard]] std::unique_ptr<Expression> move_exp() override;

    /* Auxiliary */

    [[nodiscard]] std::unique_ptr<NaryExpression> deep_copy() const;

    [[nodiscard]] std::unique_ptr<NaryExpression> move();

    [[nodiscard]] std::list<std::unique_ptr<Expression>> split() const;

    [[nodiscard]] std::list<std::unique_ptr<Expression>> move_to_split();

};

#endif //PLAJA_NARY_EXPRESSION_H
