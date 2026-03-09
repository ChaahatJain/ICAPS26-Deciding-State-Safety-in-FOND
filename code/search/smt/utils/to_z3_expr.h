//
// This file is part of the PlaJA code base.
// Copyright (c) (2019 - 2021) Marcel Vinzent.
// See README.md in the top-level directory for licensing information.
//

#ifndef PLAJA_TO_Z3_EXPR_H
#define PLAJA_TO_Z3_EXPR_H

#include <z3++.h>
#include "../../../assertions.h"

// forward declaration:
class Expression;
namespace Z3_IN_PLAJA { class Context; }

struct ToZ3Expr {
    Z3_IN_PLAJA::Context* context;
    z3::expr primary;
    z3::expr auxiliary;

    explicit ToZ3Expr(Z3_IN_PLAJA::Context& c);
    ToZ3Expr(Z3_IN_PLAJA::Context& c, z3::expr primary_, z3::expr auxiliary_);
    ~ToZ3Expr() = default;
    DEFAULT_CONSTRUCTOR(ToZ3Expr)

    [[nodiscard]] bool has_primary() const;
    [[nodiscard]] bool has_auxiliary() const;
    //
    inline void set(z3::expr primary_) { primary = std::move(primary_); }
    inline z3::expr release() { PLAJA_ASSERT(has_primary()) return std::move(primary); }
    inline z3::expr release_auxiliary() { PLAJA_ASSERT(has_auxiliary()) return std::move(auxiliary); }
    void conjunct_primary(const z3::expr& prim);
    void conjunct_auxiliary(const z3::expr& aux);
    void conjunct(const ToZ3Expr& to_z3_expr);
    [[nodiscard]] z3::expr to_conjunction() const;
};

#endif //PLAJA_TO_Z3_EXPR_H
