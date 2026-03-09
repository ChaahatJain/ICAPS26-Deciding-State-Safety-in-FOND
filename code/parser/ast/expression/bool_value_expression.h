//
// This file is part of the PlaJA code base.
// Copyright (c) (2019 - 2019) Marcel Vinzent.
// See README.md in the top-level directory for licensing information.
//

#ifndef PLAJA_BOOL_VALUE_EXPRESSION_H
#define PLAJA_BOOL_VALUE_EXPRESSION_H

#include "constant_value_expression.h"

class BoolValueExpression: public ConstantValueExpression {
private:
    bool value;
public:
    explicit BoolValueExpression(bool value);
    ~BoolValueExpression() override;

    // setter:
    inline void set_value(bool val) {value = val;}

    // getter:
    [[nodiscard]] inline bool get_value() const {return value;}

    // override:
    [[nodiscard]] bool is_boolean() const override;
    PLAJA::integer evaluateInteger(const StateBase* state) const override;
    bool equals(const Expression* exp) const override;
    [[nodiscard]] std::size_t hash() const override;
    void accept(AstVisitor* astVisitor) override;
	void accept(AstVisitorConst* astVisitor) const override;
    [[nodiscard]] std::unique_ptr<ConstantValueExpression> deepCopy_ConstantValueExp() const override;
};

#endif //PLAJA_BOOL_VALUE_EXPRESSION_H
