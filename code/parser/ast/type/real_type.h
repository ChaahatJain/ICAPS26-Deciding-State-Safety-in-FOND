//
// This file is part of the PlaJA code base.
// Copyright (c) (2019 - 2019) Marcel Vinzent.
// See README.md in the top-level directory for licensing information.
//

#ifndef PLAJA_REAL_TYPE_H
#define PLAJA_REAL_TYPE_H

#include <memory>
#include "basic_type.h"

class RealType final: public BasicType {

public:
    RealType();
    ~RealType() override;
    DELETE_CONSTRUCTOR(RealType)

    static bool assignable_from(const DeclarationType& assigned_type);

    [[nodiscard]] inline std::unique_ptr<RealType> deep_copy() const { return std::make_unique<RealType>(); }

    /* Override. */

    [[nodiscard]] bool is_numeric_type() const override;

    [[nodiscard]] bool is_floating_type() const override;

    [[nodiscard]] bool is_assignable_from(const DeclarationType& assigned_type) const override;

    [[nodiscard]] std::unique_ptr<DeclarationType> infer_common(const DeclarationType& ref_type) const override;

    [[nodiscard]] TypeKind get_kind() const override;

    void accept(AstVisitor* ast_visitor) override;

    void accept(AstVisitorConst* ast_visitor) const override;

    [[nodiscard]] std::unique_ptr<BasicType> deep_copy_basic_type() const override;

};

#endif //PLAJA_REAL_TYPE_H
