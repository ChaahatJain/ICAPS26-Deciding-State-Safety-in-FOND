//
// This file is part of the PlaJA code base.
// Copyright (c) (2019 - 2019) Marcel Vinzent.
// See README.md in the top-level directory for licensing information.
//

#ifndef PLAJA_REAL_VALUE_EXPRESSION_H
#define PLAJA_REAL_VALUE_EXPRESSION_H


#include <unordered_map>
#include "constant_value_expression.h"

class RealValueExpression final: public ConstantValueExpression {
public:
    enum ConstantName { EULER_C, PI_C, NONE_C };
private:
    ConstantName name;
    PLAJA::floating value;

public:
    /**
     * Constructs a real value expression.
     * If constant_name = NONE_C it is set to value, otherwise to the value of the respective constant.
     * @param constant_name
     * @param value
     */
    explicit RealValueExpression(ConstantName constant_name, PLAJA::floating value_ = 0.0);
    explicit RealValueExpression(PLAJA::floating value_);
    ~RealValueExpression() override;

    // setter:
    /**
     * Sets value. If constant_name = NONE_C, value is set to val, otherwise to the value of the respective constant.
     * @param constant_name
     */
    void set(ConstantName constant_name, PLAJA::floating val);

    // getter:
    [[nodiscard]] inline ConstantName get_name() const { return name; }
    [[nodiscard]] inline PLAJA::floating get_value() const { return value; }

    // override:
    [[nodiscard]] bool is_floating() const override;
    [[nodiscard]] std::unique_ptr<ConstantValueExpression> add(const ConstantValueExpression& addend) const override;
    [[nodiscard]] std::unique_ptr<ConstantValueExpression> multiply(const ConstantValueExpression &factor) const override;
    [[nodiscard]] std::unique_ptr<ConstantValueExpression> round() const override;
    [[nodiscard]] std::unique_ptr<ConstantValueExpression> floor() const override;
    [[nodiscard]] std::unique_ptr<ConstantValueExpression> ceil() const override;
    [[nodiscard]] PLAJA::floating evaluateFloating(const StateBase* state) const override;
    [[nodiscard]] bool equals(const Expression* exp) const override;
    [[nodiscard]] std::size_t hash() const override;
    void accept(AstVisitor* astVisitor) override;
	void accept(AstVisitorConst* astVisitor) const override;
    [[nodiscard]] std::unique_ptr<ConstantValueExpression> deepCopy_ConstantValueExp() const override;
};

extern const std::string constantNameToString[];
extern const std::unordered_map<std::string, RealValueExpression::ConstantName> stringToConstantName;

#endif //PLAJA_REAL_VALUE_EXPRESSION_H
