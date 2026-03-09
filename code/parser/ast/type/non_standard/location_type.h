//
// This file is part of the PlaJA code base.
// Copyright (c) (2019 - 2023) Marcel Vinzent.
// See README.md in the top-level directory for licensing information.
//

#ifndef PLAJA_LOCATION_TYPE_H
#define PLAJA_LOCATION_TYPE_H

#include "../declaration_type.h"

/**
 * Type of expression that evaluates to a location value (internally, i.e., an integer).
 * So far unused, as we only consider location value constraints of the form "automaton is in (==) location".
 */
class LocationType final: public DeclarationType {

public:
    LocationType();
    ~LocationType() override;
    DELETE_CONSTRUCTOR(LocationType)

    [[nodiscard]] std::unique_ptr<LocationType> deep_copy() const { return std::make_unique<LocationType>(); }

    /* Override. */

    [[nodiscard]] TypeKind get_kind() const override;

    bool is_assignable_from(const DeclarationType& assigned_type) const override;

    std::unique_ptr<DeclarationType> infer_common(const DeclarationType& ref_type) const override;

    [[nodiscard]] std::unique_ptr<DeclarationType> deep_copy_decl_type() const override;

    void accept(AstVisitor* ast_visitor) override;

    void accept(AstVisitorConst* ast_visitor) const override;

};

#endif //PLAJA_LOCATION_TYPE_H
