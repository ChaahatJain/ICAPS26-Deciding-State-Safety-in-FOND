//
// This file is part of the PlaJA code base.
// Copyright (c) (2019 - 2019) Marcel Vinzent.
// See README.md in the top-level directory for licensing information.
//

#ifndef PLAJA_INT_TYPE_H
#define PLAJA_INT_TYPE_H

#include <memory>
#include "basic_type.h"

class IntType final: public BasicType {

public:
    IntType();
    ~IntType() override;
    DELETE_CONSTRUCTOR(IntType)

    [[nodiscard]] inline std::unique_ptr<IntType> deep_copy() const { return std::make_unique<IntType>(); }

    static bool assignable_from(const DeclarationType& assigned_type);

    /* Override. */

    [[nodiscard]] bool is_numeric_type() const override;
    [[nodiscard]] bool is_integer_type() const override;

    [[nodiscard]] TypeKind get_kind() const override;
    bool is_assignable_from(const DeclarationType& assigned_type) const override;
    std::unique_ptr<DeclarationType> infer_common(const DeclarationType& ref_type) const override;
    [[nodiscard]] std::unique_ptr<BasicType> deep_copy_basic_type() const override;

    void accept(AstVisitor* ast_visitor) override;
    void accept(AstVisitorConst* ast_visitor) const override;

};

#endif //PLAJA_INT_TYPE_H
