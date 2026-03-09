//
// This file is part of the PlaJA code base.
// Copyright (c) (2019 - 2024) Marcel Vinzent.
// See README.md in the top-level directory for licensing information.
//

#ifndef PLAJA_TO_NUXMV_VISITOR_H
#define PLAJA_TO_NUXMV_VISITOR_H

#include <memory>
#include <string>
#include "../parser/visitor/ast_element_visitor_const.h"
#include "forward_to_nuxmv.h"

class ToNuxmvVisitor final: protected AstElementVisitorConst {

private:
    std::unique_ptr<std::string> rlt; // Return value for the subroutine.
    const ToNuxmv* parent;
    bool numericEnv; // Child expression inside numeric expression?

    /* Expressions:. */
    void visit(const BoolValueExpression* exp) override;
    void visit(const BinaryOpExpression* exp) override;
    void visit(const IntegerValueExpression* exp) override;
    void visit(const RealValueExpression* exp) override;
    void visit(const UnaryOpExpression* exp) override;
    void visit(const VariableExpression* exp) override;
    /* Non-standard. */
    void visit(const LetExpression* exp) override;
    void visit(const StateConditionExpression* exp) override;
    /* Special cases. */
    void visit(const LinearExpression* exp) override;
    void visit(const NaryExpression* exp) override;

    [[nodiscard]] std::string child_to_str(const AstElement* ast_element, bool numeric_env) const;

    explicit ToNuxmvVisitor(const ToNuxmv& parent, bool numeric_env);
public:
    ~ToNuxmvVisitor() override;
    DELETE_CONSTRUCTOR(ToNuxmvVisitor)

    static std::unique_ptr<std::string> to_nuxmv(const AstElement& ast_element, bool numeric_env, const ToNuxmv& parent);

    inline static std::unique_ptr<std::string> to_nuxmv(const AstElement& ast_element, const ToNuxmv& parent) { return to_nuxmv(ast_element, false, parent); }

};

#endif //PLAJA_TO_NUXMV_VISITOR_H
