//
// This file is part of the PlaJA code base.
// Copyright (c) (2019 - 2019) Marcel Vinzent.
// See README.md in the top-level directory for licensing information.
//

#ifndef PLAJA_ITE_EXPRESSION_H
#define PLAJA_ITE_EXPRESSION_H

#include <memory>
#include "expression.h"

class ITE_Expression: public Expression {
private:
    // Operands
    std::unique_ptr<Expression> condition; // if
    std::unique_ptr<Expression> consequence; // then
    std::unique_ptr<Expression> alternative; // else

public:
    ITE_Expression();
    ~ITE_Expression() override;

    // setter:
    inline std::unique_ptr<Expression> set_condition(std::unique_ptr<Expression>&& cond = nullptr) { std::swap(condition, cond); return std::move(cond); }
    inline std::unique_ptr<Expression> set_consequence(std::unique_ptr<Expression>&& con = nullptr) { std::swap(consequence, con); return std::move(con); }
    inline std::unique_ptr<Expression> set_alternative(std::unique_ptr<Expression>&& alt = nullptr) { std::swap(alternative, alt); return std::move(alt); }

    // getter:
    inline Expression* get_condition() { return condition.get(); }
    [[nodiscard]] inline const Expression* get_condition() const { return condition.get(); }
    inline Expression* get_consequence() { return consequence.get();}
    [[nodiscard]] inline const Expression* get_consequence() const { return consequence.get(); }
    inline Expression* get_alternative() { return alternative.get(); }
    [[nodiscard]] inline const Expression* get_alternative() const { return alternative.get(); }

    // override:
    PLAJA::integer evaluateInteger(const StateBase* state) const override;
    PLAJA::floating evaluateFloating(const StateBase* state) const override;
    [[nodiscard]] bool is_constant() const override;
    bool equals(const Expression* exp) const override;
    [[nodiscard]] std::size_t hash() const override;
    const DeclarationType* determine_type() override;
    void accept(AstVisitor* astVisitor) override;
	void accept(AstVisitorConst* astVisitor) const override;
    [[nodiscard]] std::unique_ptr<Expression> deepCopy_Exp() const override;
};


#endif //PLAJA_ITE_EXPRESSION_H
