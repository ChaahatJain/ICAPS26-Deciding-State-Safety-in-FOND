//
// This file is part of the PlaJA code base.
// Copyright (c) (2019 - 2021) Marcel Vinzent.
// See README.md in the top-level directory for licensing information.
//

#include "to_z3_expr.h"
#include "../context_z3.h"

ToZ3Expr::ToZ3Expr(Z3_IN_PLAJA::Context& c):
    context(&c)
    , primary(c())
    , auxiliary(c()) {}

ToZ3Expr::ToZ3Expr(Z3_IN_PLAJA::Context& c, z3::expr primary_, z3::expr auxiliary_):
    context(&c)
    , primary(std::move(primary_))
    , auxiliary(std::move(auxiliary_)) {
    PLAJA_ASSERT(static_cast<const z3::context&>(c()) == primary.ctx() and static_cast<const z3::context&>(c()) == auxiliary.ctx())
}

bool ToZ3Expr::has_primary() const { return Z3_IN_PLAJA::expr_is(primary); }

bool ToZ3Expr::has_auxiliary() const { return Z3_IN_PLAJA::expr_is(auxiliary); }

void ToZ3Expr::conjunct_primary(const z3::expr& prim) {
    if (has_primary()) { primary = primary && prim; }
    else { primary = prim; }
}

void ToZ3Expr::conjunct_auxiliary(const z3::expr& aux) {
    if (has_auxiliary()) { auxiliary = auxiliary && aux; }
    else { auxiliary = aux; }
}

void ToZ3Expr::conjunct(const ToZ3Expr& to_z3_expr) {
    conjunct_primary(to_z3_expr.primary);
    conjunct_auxiliary(to_z3_expr.auxiliary);
}

z3::expr ToZ3Expr::to_conjunction() const {
    if (has_auxiliary()) { return primary && auxiliary; }
    else { return primary; }
}

