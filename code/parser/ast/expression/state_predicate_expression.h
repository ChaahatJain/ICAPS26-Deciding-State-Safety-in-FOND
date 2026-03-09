//
// This file is part of the PlaJA code base.
// Copyright (c) (2019 - 2019) Marcel Vinzent.
// See README.md in the top-level directory for licensing information.
//

#ifndef PLAJA_STATE_PREDICATE_EXPRESSION_H
#define PLAJA_STATE_PREDICATE_EXPRESSION_H

#include "property_expression.h"


class StatePredicateExpression final: public PropertyExpression {
public:
    enum StatePredicate { INITIAL, DEADLOCK, TIMELOCK };
private:
    StatePredicate op;
public:
    explicit StatePredicateExpression(StatePredicate sp_op);
    ~StatePredicateExpression() override;

    // static:
    static const std::string& op_to_str(StatePredicate op);
    static std::unique_ptr<StatePredicate> str_to_op(const std::string& op_str);

    // setter:
    [[maybe_unused]] inline void set_op(StatePredicate sp_op) { op = sp_op; }

    // getter:
    [[nodiscard]] inline StatePredicate get_op() const { return op; }
    [[nodiscard]] inline const std::string& get_name() const { return op_to_str(op); }

    // override:
    const DeclarationType* determine_type() override;
    void accept(AstVisitor* astVisitor) override;
	void accept(AstVisitorConst* astVisitor) const override;
    [[nodiscard]] std::unique_ptr<PropertyExpression> deepCopy_PropExp() const override;
};


#endif //PLAJA_STATE_PREDICATE_EXPRESSION_H
