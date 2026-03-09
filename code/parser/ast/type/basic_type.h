//
// This file is part of the PlaJA code base.
// Copyright (c) (2019 - 2019) Marcel Vinzent.
// See README.md in the top-level directory for licensing information.
//

#ifndef PLAJA_BASIC_TYPE_H
#define PLAJA_BASIC_TYPE_H

#include "declaration_type.h"

class BasicType: public DeclarationType {
public:
    BasicType();
    ~BasicType() override = 0;
    DELETE_CONSTRUCTOR(BasicType)

    /** Deep copy of a basic type. */
    [[nodiscard]] virtual std::unique_ptr<BasicType> deep_copy_basic_type() const = 0;

    /* Override. */
    [[nodiscard]] bool is_basic_type() const override;
    [[nodiscard]] bool is_trivial_type() const override;
    [[nodiscard]] std::unique_ptr<DeclarationType> deep_copy_decl_type() const override;
};

#endif //PLAJA_BASIC_TYPE_H
