//
// This file is part of the PlaJA code base.
// Copyright (c) (2019 - 2022) Marcel Vinzent.
// See README.md in the top-level directory for licensing information.
//

#ifndef PLAJA_TO_Z3_VISITOR_H
#define PLAJA_TO_Z3_VISITOR_H

#include <vector>
#include "../../utils/forward_z3.h"

// forward declaration:
class Expression;
struct ToZ3Expr;
struct ToZ3ExprSplits;
namespace Z3_IN_PLAJA { class StateVarsInZ3; }

namespace Z3_IN_PLAJA {

    extern ToZ3Expr to_z3(const Expression& exp, const Z3_IN_PLAJA::StateVarsInZ3& var_to_z3);
    extern z3::expr to_z3_condition(const Expression& exp, const Z3_IN_PLAJA::StateVarsInZ3& var_to_z3);
    extern std::unique_ptr<ToZ3ExprSplits> to_z3_condition_splits(const Expression& exp, const Z3_IN_PLAJA::StateVarsInZ3& var_to_z3);

    extern z3::expr to_conjunction(const z3::expr_vector& vec);
    extern z3::expr to_conjunction(const std::vector<z3::expr>& vec);
    extern z3::expr to_disjunction(const z3::expr_vector& vec);
    extern z3::expr to_disjunction(const std::vector<z3::expr>& vec);

}

#endif //PLAJA_TO_Z3_VISITOR_H
