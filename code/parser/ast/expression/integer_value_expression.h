//
// This file is part of the PlaJA code base.
// Copyright (c) (2019 - 2019) Marcel Vinzent.
// See README.md in the top-level directory for licensing information.
//

#ifndef PLAJA_INTEGER_VALUE_EXPRESSION_H
#define PLAJA_INTEGER_VALUE_EXPRESSION_H

#include "constant_value_expression.h"

class IntegerValueExpression final: public ConstantValueExpression {
private:
    PLAJA::integer value;
public:
    explicit IntegerValueExpression(PLAJA::integer val);
    ~IntegerValueExpression() override;

    // setter:
    inline void set_value(PLAJA::integer val) {value = val;}

    // getter:
    [[nodiscard]] inline PLAJA::integer get_value() const {return value;}

    // override:
    [[nodiscard]] bool is_integer() const override;
    [[nodiscard]] std::unique_ptr<ConstantValueExpression> add(const ConstantValueExpression& addend) const override;
    [[nodiscard]] std::unique_ptr<ConstantValueExpression> multiply(const ConstantValueExpression &factor) const override;
    [[nodiscard]] std::unique_ptr<ConstantValueExpression> round() const override;
    [[nodiscard]] std::unique_ptr<ConstantValueExpression> floor() const override;
    [[nodiscard]] std::unique_ptr<ConstantValueExpression> ceil() const override;
    PLAJA::integer evaluateInteger(const StateBase* state) const override;
    bool equals(const Expression* exp) const override;
    [[nodiscard]] std::size_t hash() const override;
    void accept(AstVisitor* astVisitor) override;
	void accept(AstVisitorConst* astVisitor) const override;
    [[nodiscard]] std::unique_ptr<ConstantValueExpression> deepCopy_ConstantValueExp() const override;
};


#endif //PLAJA_INTEGER_VALUE_EXPRESSION_H
