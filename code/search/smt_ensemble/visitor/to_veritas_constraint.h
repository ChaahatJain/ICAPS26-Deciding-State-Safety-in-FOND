//
// This file is part of the PlaJA code base (2019 - 2020).
//

#ifndef PLAJA_TO_VERITAS_CONSTRAINT_H
#define PLAJA_TO_VERITAS_CONSTRAINT_H

#include <memory>
#include "../predicates/Equation.h"
#include "../../../parser/ast/expression/forward_expression.h"
#include "../forward_smt_veritas.h"
#include "../using_veritas.h"
#include "../vars_in_veritas.h"

namespace TO_VERITAS_CONSTRAINT {

    /**
     * Computes the VeritasConstraint for a linear constraint.
     */
    extern std::unique_ptr<VeritasConstraint> to_veritas_linear_constraint(const Expression& expr, const StateIndexesInVeritas& state_indexes_in_veritas);

    extern std::unique_ptr<VeritasConstraint> to_veritas_conjunction_bool(const Expression& expr, const StateIndexesInVeritas& state_indexes_in_veritas);

    extern std::unique_ptr<VeritasConstraint> to_veritas_disjunction_bool(const Expression& expr, const StateIndexesInVeritas& state_indexes_in_veritas);

    extern std::unique_ptr<VeritasConstraint> to_veritas_linear_sum(VeritasVarIndex_type target_var, VERITAS_IN_PLAJA::Equation::EquationType op, const Expression& assignment_sum, const StateIndexesInVeritas& state_indexes_in_veritas);

}

#endif //PLAJA_TO_VERITAS_CONSTRAINT_H
