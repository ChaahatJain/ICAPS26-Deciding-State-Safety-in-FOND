//
// This file is part of the PlaJA code base.
// Copyright (c) (2019 - 2019) Marcel Vinzent.
// See README.md in the top-level directory for licensing information.
//

#ifndef PLAJA_TO_STR_VISITOR_H
#define PLAJA_TO_STR_VISITOR_H

#include <memory>
#include <string>
#include "../ast_visitor_const.h"

class Commentable;

/** Class for more readable output during debugging. Not all ast-elements supported. */
class ToStrVisitor: public AstVisitorConst {
private:
    std::unique_ptr<std::string> rlt; // return value for the subroutine

    explicit ToStrVisitor();
public:
    ~ToStrVisitor() override;
    DELETE_CONSTRUCTOR(ToStrVisitor)

    static std::unique_ptr<std::string> to_str(const AstElement& ast_element);
    static void dump(const AstElement& ast_element);

    void visit(const Action* action) override;
    void visit(const Assignment* assignment) override;
    void visit(const ConstantDeclaration* const_decl) override;
    void visit(const Destination* destination) override;
    void visit(const Edge* edge) override;

    // expressions:
    void visit(const ArrayAccessExpression* exp) override;
    void visit(const ArrayValueExpression* exp) override;
    void visit(const BinaryOpExpression* exp) override;
    void visit(const BoolValueExpression* exp) override;
    void visit(const ConstantExpression* exp) override;
    void visit(const IntegerValueExpression* exp) override;
    void visit(const ITE_Expression* exp) override;
    void visit(const RealValueExpression* exp) override;
    void visit(const UnaryOpExpression* exp) override;
    void visit(const VariableExpression* exp) override;
    // special cases:
    void visit(const LinearExpression* exp) override;
    void visit(const NaryExpression* exp) override;
    // non-standard:
    void visit(const CommentExpression* exp) override;
    void visit(const ConstantArrayAccessExpression* op) override;
    void visit(const LocationValueExpression* exp) override;
    void visit(const VariableValueExpression* exp) override;

    // types:
    void visit(const ArrayType* type) override;
    void visit(const BoolType* type) override;
    void visit(const ClockType* type) override;
    void visit(const ContinuousType* type) override;
    void visit(const IntType* type) override;
    void visit(const RealType* type) override;
    // non-standard:
    void visit(const LocationType* type) override;

};

#endif //PLAJA_TO_STR_VISITOR_H
