//
// This file is part of the PlaJA code base.
// Copyright (c) (2019 - 2019) Marcel Vinzent.
// See README.md in the top-level directory for licensing information.
//

#ifndef PLAJA_EXPRESSION_H
#define PLAJA_EXPRESSION_H

#include <vector>
#include "property_expression.h"
#include "../../using_parser.h"

class StateBase;

class ConstantValueExpression;

namespace PLAJA_DEBUG { FIELD_IF_DEBUG(extern bool printEvalFalseExpression;) } // Auxiliary flag, to be deployed for debugging.

class Expression: public PropertyExpression {
public:
    Expression();
    ~Expression() override = 0;
    DELETE_CONSTRUCTOR(Expression)

    /**
     * Evaluate an expression over a state returning an integer value.
     * May throw a NotImplementedException if called on expressions not evaluating to an integer value.
     * @return
     */
    virtual PLAJA::integer evaluateInteger(const StateBase* state) const;

    [[nodiscard]] inline PLAJA::integer evaluate_integer(const StateBase& state) const { return evaluateInteger(&state); }

    [[nodiscard]] inline PLAJA::integer evaluate_integer_const() const {
        PLAJA_ASSERT(is_constant())
        return evaluateInteger(nullptr);
    }

    /**
     * Evaluate an expression over a state returning a floating value.
     * May throw a NotImplementedException.
     * @return
     */
    virtual PLAJA::floating evaluateFloating(const StateBase* state) const;

    [[nodiscard]] inline PLAJA::floating evaluate_floating(const StateBase& state) const { return evaluateFloating(&state); }

    [[nodiscard]] inline PLAJA::floating evaluate_floating_const() const {
        PLAJA_ASSERT(is_constant())
        return evaluateFloating(nullptr);
    }

    /**
     * Evaluate an expression over a state assigning to a target array.
     * @param target
     */
    [[nodiscard]] virtual std::vector<PLAJA::integer> evaluateIntegerArray(const StateBase* state) const;

    [[nodiscard]] inline std::vector<PLAJA::integer> evaluate_integer_array(const StateBase& state) const { return evaluateIntegerArray(&state); }

    [[nodiscard]] inline std::vector<PLAJA::integer> evaluate_integer_array_const() const { return evaluateIntegerArray(nullptr); }

    [[nodiscard]] virtual std::vector<PLAJA::floating> evaluateFloatingArray(const StateBase* state) const;

    [[nodiscard]] inline std::vector<PLAJA::floating> evaluate_floating_array(const StateBase& state) const { return evaluateFloatingArray(&state); }

    [[nodiscard]] inline std::vector<PLAJA::floating> evaluate_floating_array_const() const { return evaluateFloatingArray(nullptr); }

    /**
     * Evaluate expression and return value as ConstValueExpression.
     * Note, may only be called after determine_type has been called.
     * @param state
     * @return
    */
    [[nodiscard]] std::unique_ptr<ConstantValueExpression> evaluate(const StateBase* state) const;
    [[nodiscard]] std::unique_ptr<ConstantValueExpression> evaluate(const StateBase& state) const;
    [[nodiscard]] std::unique_ptr<ConstantValueExpression> evaluate_const() const;

    /**
     * Does expression evaluate to an integer value?
     * Note, may only be called after determine_type has been called.
     * @param state : expression must be constant if null.
     * @param value : any integer if not specified.
     */
    [[nodiscard]] bool evaluates_integer(const StateBase* state, const PLAJA::integer* value = nullptr) const;

    [[nodiscard]] inline bool evaluates_integer(const StateBase* state, PLAJA::integer value) const { return evaluates_integer(state, &value); }

    [[nodiscard]] inline bool evaluates_integer_const(const PLAJA::integer* value = nullptr) const { return evaluates_integer(nullptr, value); }

    [[nodiscard]] inline bool evaluates_integer_const(PLAJA::integer value) const { return evaluates_integer_const(&value); }

    /**
     * Does expression evaluate to a floating type?
     * Must not be called on expressions that do not evaluate to a trivial type.
     */
    [[nodiscard]] bool evaluates_floating_type() const;

    /** Is this expression constant? */
    [[nodiscard]] virtual bool is_constant() const;

    virtual bool equals(const Expression* exp) const; // for usage within the AST, but no need to be private

    inline bool operator==(const Expression& exp) const { return this->equals(&exp); }

    inline bool operator!=(const Expression& exp) const { return !this->equals(&exp); }

    [[nodiscard]] virtual std::size_t hash() const; // we orientate on the hash implementation in Prism

    /** Deep copy of expression. */
    [[nodiscard]] virtual std::unique_ptr<Expression> deepCopy_Exp() const = 0;

    [[nodiscard]] virtual std::unique_ptr<Expression> move_exp();

    /* Override. */

    [[nodiscard]] bool is_proposition() const override; // in JANI expression as subclass of property expression is typically a proposition (exceptions are specified explicitly)

    [[nodiscard]] std::unique_ptr<PropertyExpression> deepCopy_PropExp() const final;

    [[nodiscard]] std::unique_ptr<PropertyExpression> move_prop_exp() final;

};

#endif //PLAJA_EXPRESSION_H
