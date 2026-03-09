//
// This file is part of the PlaJA code base.
// Copyright (c) (2019 - 2019) Marcel Vinzent.
// See README.md in the top-level directory for licensing information.
//

#ifndef PLAJA_LVALUE_EXPRESSION_H
#define PLAJA_LVALUE_EXPRESSION_H

#include "expression.h"

/**
 * May be adapted when including arrays and further types into evaluation
 */
class LValueExpression: public Expression {
public:
    LValueExpression();
    ~LValueExpression() override = 0;
    DELETE_CONSTRUCTOR(LValueExpression)

    /** Assigns the target state the value the value expression evaluates to over the current state. */
    virtual void assign(StateBase& target, const StateBase* source, const Expression* value) const;

    /**
     * Access state index of lvalue in target state.
     * @param source : for array index the state to determine the array index.
     */
    [[nodiscard]] virtual PLAJA::integer access_integer(const StateBase& target, const StateBase* source) const;
    [[nodiscard]] virtual PLAJA::floating access_floating(const StateBase& target, const StateBase* source) const;

    /** Get the variable id of the (assigned) variable. */
    [[nodiscard]] virtual VariableID_type get_variable_id() const = 0;

    /** In case of an array access return the index, nullptr in case of non-array access. */
    [[nodiscard]] virtual const Expression* get_array_index() const;

    /**
     * Get the variable index of the (assigned) variable.
     * @param state : state in which the lvalue is evaluated
     * -- for array access the variable index may be state-dependent.
     * Note: nullptr is valid for scalar variables or constant array access.
     */
    [[nodiscard]] virtual VariableIndex_type get_variable_index(const StateBase* state) const = 0;

    [[nodiscard]] inline VariableIndex_type get_variable_index() const { return get_variable_index(nullptr); }

    /** Deep copy of lvalue expression. */
    [[nodiscard]] virtual std::unique_ptr<LValueExpression> deep_copy_lval_exp() const = 0;

    // short-cuts:
    [[nodiscard]] bool is_boolean() const;
    [[nodiscard]] bool is_integer() const;
    [[nodiscard]] bool is_floating() const;

    // override:
    [[nodiscard]] bool is_constant() const override;
    [[nodiscard]] std::unique_ptr<Expression> deepCopy_Exp() const override;
};

#endif //PLAJA_LVALUE_EXPRESSION_H
