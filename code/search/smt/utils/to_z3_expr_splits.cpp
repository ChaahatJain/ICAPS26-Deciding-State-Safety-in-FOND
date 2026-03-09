//
// This file is part of the PlaJA code base.
// Copyright (c) (2019 - 2022) Marcel Vinzent.
// See README.md in the top-level directory for licensing information.
//

#include "to_z3_expr_splits.h"
#include "../../../parser/ast/expression/expression.h"
#include "../context_z3.h"

ToZ3ExprSplits::ToZ3ExprSplits(Z3_IN_PLAJA::Context& c): context(&c), splitsInZ3(c()) {}

ToZ3ExprSplits::ToZ3ExprSplits(std::list<std::unique_ptr<Expression>>&& splits_, Z3_IN_PLAJA::Context& c)
    : context(&c)
    , splits(std::move(splits_)), splitsInZ3(c()) {}

ToZ3ExprSplits::~ToZ3ExprSplits() = default;