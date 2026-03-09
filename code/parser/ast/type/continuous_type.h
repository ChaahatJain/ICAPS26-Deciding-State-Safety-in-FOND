//
// This file is part of the PlaJA code base.
// Copyright (c) (2019 - 2019) Marcel Vinzent.
// See README.md in the top-level directory for licensing information.
//

#ifndef PLAJA_CONTINUOUS_TYPE_H
#define PLAJA_CONTINUOUS_TYPE_H

#include <memory>
#include "declaration_type.h"

class ContinuousType final: public DeclarationType {

public:
    ContinuousType();
    ~ContinuousType() override;
    DELETE_CONSTRUCTOR(ContinuousType)

    [[nodiscard]] inline std::unique_ptr<ContinuousType> deep_copy() const { return std::make_unique<ContinuousType>(); } // NOLINT(*-convert-member-functions-to-static)

    /* Override. */
    [[nodiscard]] bool is_numeric_type() const override;
    [[nodiscard]] bool is_floating_type() const override;
    [[nodiscard]] bool is_trivial_type() const override;

    [[nodiscard]] TypeKind get_kind() const override;
    [[nodiscard]] std::unique_ptr<DeclarationType> deep_copy_decl_type() const override;

    void accept(AstVisitor* ast_visitor) override;
    void accept(AstVisitorConst* ast_visitor) const override;

};

#endif //PLAJA_CONTINUOUS_TYPE_H
