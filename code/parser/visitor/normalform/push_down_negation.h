//
// This file is part of the PlaJA code base.
// Copyright (c) (2019 - 2023) Marcel Vinzent.
// See README.md in the top-level directory for licensing information.
//

#ifndef PLAJA_PUSH_DOWN_NEGATION_H
#define PLAJA_PUSH_DOWN_NEGATION_H

#include <memory>
#include "../ast_visitor.h"

class PushDownNeg: public AstVisitor {

private:
    bool negationPushedDown;

    inline void push_negation_down() { negationPushedDown = true; }

    inline void cancel_negation() { negationPushedDown = false; }

    inline void reset_negation() { negationPushedDown = true; }

    [[nodiscard]] inline bool to_negate() const { return negationPushedDown; }

    [[nodiscard]] bool is_to_negate(const Expression& expr) const;
    [[nodiscard]] bool is_to_negate(const Expression* expr) const;
    [[nodiscard]] std::unique_ptr<Expression> negate(std::unique_ptr<Expression>&& exp);

    void visit(BinaryOpExpression* exp) override;
    void visit(UnaryOpExpression* exp) override;
    /* Non-standard: */
    void visit(StateConditionExpression* exp) override;
    /* Special case. */
    void visit(LinearExpression* exp) override;
    void visit(NaryExpression* exp) override;

    PushDownNeg();
public:
    ~PushDownNeg() override;
    DELETE_CONSTRUCTOR(PushDownNeg)

    static void push_down_negation(std::unique_ptr<AstElement>& ast_elem);
};

#endif //PLAJA_PUSH_DOWN_NEGATION_H
