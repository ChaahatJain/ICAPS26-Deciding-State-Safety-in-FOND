//
// This file is part of the PlaJA code base.
// Copyright (c) (2019 - 2019) Marcel Vinzent.
// See README.md in the top-level directory for licensing information.
//

#include "basic_type.h"

BasicType::BasicType() = default;

BasicType::~BasicType() = default;

/* Override. */

bool BasicType::is_basic_type() const { return true; }

bool BasicType::is_trivial_type() const { return true; }

std::unique_ptr<DeclarationType> BasicType::deep_copy_decl_type() const { return deep_copy_basic_type(); }

