//
// This file is part of the PlaJA code base.
// Copyright (c) (2023 - 2024) Chaahat Jain.
//

#ifndef PLAJA_VERITAS_TO_Z3_H
#define PLAJA_VERITAS_TO_Z3_H

#include "../../../include/marabou_include/forward_marabou.h"
#include "../../information/jani2nnet/forward_jani_2_nnet.h"
#include "../../information/jani2nnet/using_jani2nnet.h"
#include "../../smt/context_z3.h"
#include "../forward_smt_veritas.h"
#include "../veritas_query.h"
#include "encoding_information.h"
#include "tree_vars_in_z3.h"

namespace VERITAS_TO_Z3 {

    // bounds:

    inline z3::expr generate_lower_bound(const z3::expr& var, VeritasFloating_type lower_bound) {
        PLAJA_ASSERT(VERITAS_IN_PLAJA::negative_infinity <= lower_bound and lower_bound <= VERITAS_IN_PLAJA::infinity)
        if (var.is_int()) { return Z3_IN_PLAJA::z3_to_int(var.ctx(), PLAJA_UTILS::cast_numeric<PLAJA::integer>(std::floor(lower_bound))) <= var; }
        else {
            PLAJA_ASSERT(var.is_real())
            return Z3_IN_PLAJA::z3_to_real(var.ctx(), std::floor(lower_bound), Z3_IN_PLAJA::floatingPrecision) <= var;
        }
    }

    inline z3::expr generate_upper_bound(const z3::expr& var, VeritasFloating_type upper_bound) {
        PLAJA_ASSERT(VERITAS_IN_PLAJA::negative_infinity <= upper_bound and upper_bound <= VERITAS_IN_PLAJA::infinity)
        if (var.is_int()) { return var <= Z3_IN_PLAJA::z3_to_int(var.ctx(), PLAJA_UTILS::cast_numeric<PLAJA::integer>(std::ceil(upper_bound))); }
        else {
            PLAJA_ASSERT(var.is_real())
            return var <= Z3_IN_PLAJA::z3_to_real(var.ctx(), std::ceil(upper_bound), Z3_IN_PLAJA::floatingPrecision);
        }
    }

    inline z3::expr generate_bounds(const z3::expr& var, VeritasFloating_type lower_bound, VeritasFloating_type upper_bound) { return generate_lower_bound(var, lower_bound) && generate_upper_bound(var, upper_bound); } // NOLINT(bugprone-easily-swappable-parameters)

    template<typename Solver_t>
    inline void add_bounds(Solver_t& solver, const z3::expr& var, VeritasFloating_type lower_bound, VeritasFloating_type upper_bound) {
        if (VERITAS_IN_PLAJA::negative_infinity <= lower_bound and lower_bound <= VERITAS_IN_PLAJA::infinity) { solver.add(generate_lower_bound(var, lower_bound)); }
        if (VERITAS_IN_PLAJA::negative_infinity <= upper_bound and upper_bound <= VERITAS_IN_PLAJA::infinity) { solver.add(generate_upper_bound(var, upper_bound)); }
    }

    // linear constraints:

    extern z3::expr generate_equation(const VERITAS_IN_PLAJA::Equation& equation, const Z3_IN_PLAJA::VeritasToZ3Vars& variables);

    inline z3::expr generate_bound(const VERITAS_IN_PLAJA::Tightening& bound, const Z3_IN_PLAJA::VeritasToZ3Vars& variables) {
        switch (bound._type) {
            case VERITAS_IN_PLAJA::Tightening::LB: return variables.to_z3_expr(bound._variable) <= Z3_IN_PLAJA::z3_to_real(variables.get_context_z3(), bound._value, Z3_IN_PLAJA::floatingPrecision);
            case VERITAS_IN_PLAJA::Tightening::UB: return variables.to_z3_expr(bound._variable) >= Z3_IN_PLAJA::z3_to_real(variables.get_context_z3(), bound._value, Z3_IN_PLAJA::floatingPrecision);
        }
        PLAJA_ABORT
    }

    extern z3::expr generate_case_split(const PiecewiseLinearCaseSplit& case_split, const Z3_IN_PLAJA::VeritasToZ3Vars& variables);

    extern z3::expr generate_disjunction(const DisjunctionConstraint& disjunction, const Z3_IN_PLAJA::VeritasToZ3Vars& variables);

    // Ensemble (sub-) structures:
    extern z3::expr generate_output_interface(OutputIndex_type output_label, const veritas::AddTree& ensemble, const Z3_IN_PLAJA::VeritasToZ3Vars& vars);
    extern std::vector<z3::expr> generate_output_interfaces(const veritas::AddTree& ensemble, const Z3_IN_PLAJA::VeritasToZ3Vars& vars);

    extern z3::expr_vector generate_conjunction_of_equations(const VERITAS_IN_PLAJA::VeritasQuery& query, const Z3_IN_PLAJA::VeritasToZ3Vars& vars);
    extern z3::expr_vector generate_ensemble_forwarding_structures(const veritas::AddTree trees, const Z3_IN_PLAJA::VeritasToZ3Vars& vars);

}

#endif //PLAJA_VERITAS_TO_Z3_H
