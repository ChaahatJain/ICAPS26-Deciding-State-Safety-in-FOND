//
// This file is part of the PlaJA code base.
// Copyright (c) (2019 - 2024) Marcel Vinzent.
// See README.md in the top-level directory for licensing information.
//

#ifndef PLAJA_SMT_VARS_OF_H
#define PLAJA_SMT_VARS_OF_H

#include <unordered_set>
#include "../forward_smt_z3.h"
#include "../using_smt.h"

class AstElement;

namespace Z3_IN_PLAJA {

    [[nodiscard]] extern std::unordered_set<Z3_IN_PLAJA::VarId_type> collect_smt_vars(const AstElement& ast_element, const Z3_IN_PLAJA::StateVarsInZ3& state_vars);

}

#endif //PLAJA_SMT_VARS_OF_H
