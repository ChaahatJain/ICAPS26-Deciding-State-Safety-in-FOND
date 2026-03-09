//
// This file is part of the PlaJA code base.
// Copyright (c) (2023 - 2024) Chaahat Jain.
//

#ifndef PLAJA_VERITAS_TO_GUROBI_H
#define PLAJA_VERITAS_TO_GUROBI_H

#include "../../../include/marabou_include/forward_marabou.h"
#include "../../information/jani2nnet/forward_jani_2_nnet.h"
#include "../../information/jani2nnet/using_jani2nnet.h"
#include "../../smt/context_z3.h"
#include "../forward_smt_veritas.h"
#include "../veritas_query.h"
#include "encoding_information.h"
#include "tree_vars_in_gurobi.h"

namespace VERITAS_TO_GUROBI {
    // linear constraints:

    extern z3::expr generate_equation(const VERITAS_IN_PLAJA::Equation& equation, const GUROBI_IN_PLAJA::VeritasToGurobiVars& variables);

    extern z3::expr generate_case_split(const PiecewiseLinearCaseSplit& case_split, const GUROBI_IN_PLAJA::VeritasToGurobiVars& variables);

    extern z3::expr generate_disjunction(const DisjunctionConstraint& disjunction, const GUROBI_IN_PLAJA::VeritasToGurobiVars& variables);

    // Ensemble (sub-) structures:
    extern z3::expr generate_output_interface(OutputIndex_type output_label, const veritas::AddTree& ensemble, const GUROBI_IN_PLAJA::VeritasToGurobiVars& vars);
    extern std::vector<z3::expr> generate_output_interfaces(const veritas::AddTree& ensemble, const GUROBI_IN_PLAJA::VeritasToGurobiVars& vars);

    extern z3::expr_vector generate_conjunction_of_equations(const VERITAS_IN_PLAJA::VeritasQuery& query, const GUROBI_IN_PLAJA::VeritasToGurobiVars& vars);
    extern z3::expr_vector generate_ensemble_forwarding_structures(const VERITAS_IN_PLAJA::VeritasQuery& query, const GUROBI_IN_PLAJA::VeritasToGurobiVars& vars);

}

#endif //PLAJA_VERITAS_TO_GUROBI_H
