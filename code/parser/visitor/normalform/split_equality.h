//
// This file is part of the PlaJA code base.
// Copyright (c) (2019 - 2023) Marcel Vinzent.
// See README.md in the top-level directory for licensing information.
//

#ifndef PLAJA_SPLIT_EQUALITY_H
#define PLAJA_SPLIT_EQUALITY_H

#include <memory>
#include "../ast_visitor.h"

class SplitEquality: public AstVisitor {
public:
    enum Mode { EqualityOnly, InequalityOnly, Both }; // NOLINT(*-enum-size)

private:
    Mode mode;

    void visit(BinaryOpExpression* exp) override;
    void visit(LinearExpression* exp) override;

    explicit SplitEquality(Mode mode);
public:
    ~SplitEquality() override;
    DELETE_CONSTRUCTOR(SplitEquality)

    static void split_equality(std::unique_ptr<AstElement>& ast_elem, Mode mode);
    static void split_equality(std::unique_ptr<Expression>& expr, Mode mode);
};

#endif //PLAJA_SPLIT_EQUALITY_H
