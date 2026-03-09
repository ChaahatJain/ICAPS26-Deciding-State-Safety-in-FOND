//
// This file is part of the PlaJA code base.
// Copyright (c) (2019 - 2023) Marcel Vinzent.
// See README.md in the top-level directory for licensing information.
//

#ifndef PLAJA_RETRIEVE_VARIABLE_BOUND_H
#define PLAJA_RETRIEVE_VARIABLE_BOUND_H

#include "ast_visitor_const.h"
#include "../ast/expression/binary_op_expression.h"

class RetrieveVariableBound final: public AstVisitorConst {

private:
    const LValueExpression* stateIndex;
    BinaryOpExpression::BinaryOp op;
    std::unique_ptr<ConstantValueExpression> value;

    const Expression* lastExpression; // to detect bounds;

    void visit(const BinaryOpExpression* expr) override;
    void visit(const UnaryOpExpression* expr) override;
    void visit(const VariableExpression* expr) override;
    void visit(const LinearExpression* expr) override;

public:
    RetrieveVariableBound();
    ~RetrieveVariableBound() override;
    DELETE_CONSTRUCTOR(RetrieveVariableBound)

    /**
     * If this expression constitutes a variable bound it is retrieved.
     * Currently: x {<=,<,==,>,>=} c, b or !b.
     * Returns nullptr if not.
     */
    [[nodiscard]] static std::unique_ptr<RetrieveVariableBound> retrieve_bound(const Expression& expression);

    [[nodiscard]] inline bool has_bound() const { return stateIndex; }

    [[nodiscard]] VariableIndex_type get_state_index() const;

    [[nodiscard]] inline BinaryOpExpression::BinaryOp get_op() const { return op; }

    [[nodiscard]] inline const ConstantValueExpression& get_value() const { return *value; }

    void negate();

};

#endif //PLAJA_RETRIEVE_VARIABLE_BOUND_H
