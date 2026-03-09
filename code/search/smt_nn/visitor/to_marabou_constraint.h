//
// This file is part of the PlaJA code base.
// Copyright (c) (2019 - 2020) Marcel Vinzent.
// See README.md in the top-level directory for licensing information.
//

#ifndef PLAJA_TO_MARABOU_CONSTRAINT_H
#define PLAJA_TO_MARABOU_CONSTRAINT_H

#include <memory>
#include "../../../include/marabou_include/equation.h"
#include "../../../parser/ast/expression/forward_expression.h"
#include "../forward_smt_nn.h"
#include "../using_marabou.h"

namespace TO_MARABOU_CONSTRAINT {

    /**
     * Computes the MarabouConstraint for a linear constraint.
     */
    extern std::unique_ptr<MarabouConstraint> to_marabou_linear_constraint(const Expression& expr, const StateIndexesInMarabou& state_indexes_in_marabou);

    extern std::unique_ptr<MarabouConstraint> to_marabou_conjunction_bool(const Expression& expr, const StateIndexesInMarabou& state_indexes_in_marabou);

    extern std::unique_ptr<MarabouConstraint> to_marabou_disjunction_bool(const Expression& expr, const StateIndexesInMarabou& state_indexes_in_marabou);

    extern std::unique_ptr<MarabouConstraint> to_marabou_linear_sum(MarabouVarIndex_type target_var, Equation::EquationType op, const Expression& assignment_sum, const StateIndexesInMarabou& state_indexes_in_marabou);

}

#endif //PLAJA_TO_MARABOU_CONSTRAINT_H
