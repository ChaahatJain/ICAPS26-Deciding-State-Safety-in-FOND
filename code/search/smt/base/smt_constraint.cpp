//
// This file is part of the PlaJA code base.
// Copyright (c) (2019 - 2024) Marcel Vinzent.
// See README.md in the top-level directory for licensing information.
//

#include "smt_constraint.h"

PLAJA::SmtConstraint::SmtConstraint() = default;

PLAJA::SmtConstraint::~SmtConstraint() = default;

bool PLAJA::SmtConstraint::is_trivially_true() const { return false; }

bool PLAJA::SmtConstraint::is_trivially_false() const { return false; }
