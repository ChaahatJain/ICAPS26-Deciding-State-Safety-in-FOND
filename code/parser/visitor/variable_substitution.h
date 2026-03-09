//
// This file is part of the PlaJA code base.
// Copyright (c) (2019 - 2020) Marcel Vinzent.
// See README.md in the top-level directory for licensing information.
//

#ifndef PLAJA_VARIABLE_SUBSTITUTION_H
#define PLAJA_VARIABLE_SUBSTITUTION_H

#include <memory>
#include <unordered_map>
#include "ast_visitor.h"

class VariableSubstitution: public AstVisitor {
private:
    bool substituted;
    const std::unordered_map<VariableID_type, const Expression*>& idToExpression;

    /* Expressions. */
    void visit(ArrayAccessExpression* exp) override;
    void visit(DerivativeExpression* exp) override;
    void visit(VariableExpression* exp) override;

    /* Non-standard. */
    void visit(LocationValueExpression* exp) override;

    /* Special cases. */
    void visit(LinearExpression* expr) override;

public:
    explicit VariableSubstitution(const std::unordered_map<VariableID_type, const Expression*>& id_to_expression);
    ~VariableSubstitution() override;
    DELETE_CONSTRUCTOR(VariableSubstitution)

    std::unique_ptr<Expression> substitute(const Expression& exp);
    bool substitute(std::unique_ptr<Expression>& exp);
    static std::unique_ptr<Expression> substitute(const std::unordered_map<VariableID_type, const Expression*>& id_to_expression, const Expression& exp);
    static bool substitute(const std::unordered_map<VariableID_type, const Expression*>& id_to_expression, std::unique_ptr<Expression>& exp);

    std::unique_ptr<Expression> substitute(const Expression& exp, const std::unordered_map<VariableID_type, const Assignment*>& id_to_sub_non_det);

};

#endif //PLAJA_VARIABLE_SUBSTITUTION_H
