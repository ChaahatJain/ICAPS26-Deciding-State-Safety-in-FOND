//
// This file is part of the PlaJA code base.
// Copyright (c) (2019 - 2021) Marcel Vinzent.
// See README.md in the top-level directory for licensing information.
//

#include <memory>
#include "z3++.h"
#include "marabou_in_z3_checker.h"
#include "../../../../include/marabou_include/disjunction_constraint.h"
#include "../../../../include/marabou_include/input_query.h"
#include "../../../../stats/stats_base.h"
#include "../../../smt/solver/smt_solver_z3.h"
#include "../../../smt_nn/solver/smt_solver_marabou.h"
#include "../../../smt_nn/to_z3/marabou_to_z3.h"
#include "../../smt/model_z3_pa.h"

MarabouInZ3Checker::MarabouInZ3Checker(const Jani2NNet& jani_2_nnet, const PLAJA::Configuration& config):
    NNSatInZ3Base(jani_2_nnet, config)
    , variables(nullptr) {

    // sanity check
    switch (encodingInformation->_bound_encoding()) {
        case MARABOU_TO_Z3::EncodingInformation::ALL:
        case MARABOU_TO_Z3::EncodingInformation::NONE: { break; }
        case MARABOU_TO_Z3::EncodingInformation::SKIP_FIXED_RELU_INPUTS:
        case MARABOU_TO_Z3::EncodingInformation::ONLY_RELU_OUTPUTS: {
            std::cout << "Warning: unsupported bound encoding using \"marabou_base_encoding\"." << std::endl;
            std::cout << "Falling back to \"none\"." << std::endl;
        }
    }
}

MarabouInZ3Checker::~MarabouInZ3Checker() = default;

// variables:

void MarabouInZ3Checker::generate_and_add_variables(const MARABOU_IN_PLAJA::MarabouQuery& query, Z3_IN_PLAJA::SMTSolver& solver) {
    // reset adapt for query:
    variables = std::make_unique<Z3_IN_PLAJA::MarabouToZ3Vars>(modelZ3.get_context(), query._context());

    // add bounds (currently simply over-approximated to integer bounds)
    if (encodingInformation->_bound_encoding() == MARABOU_TO_Z3::EncodingInformation::BoundEncoding::ALL) {
        for (auto it = query.variableIterator(); !it.end(); ++it) {
            MARABOU_TO_Z3::add_bounds(solver, variables->to_z3_expr(it.var()), query.get_lower_bound(it.var()), query.get_upper_bound(it.var()));
        }
    } else {
        for (auto var: query.generate_input_list()) { // must still add input bounds
            MARABOU_TO_Z3::add_bounds(solver, variables->to_z3_expr(var), query.get_lower_bound(var), query.get_upper_bound(var));
        }
    }

}

// structures:

void MarabouInZ3Checker::add_relu(Z3_IN_PLAJA::SMTSolver& solver, const ReluConstraint& relu_constraint, const MARABOU_IN_PLAJA::MarabouQuery& query) {
    if (encodingInformation->_mip_encoding()) {
        solver.add(MARABOU_TO_Z3::generate_relu_as_mip_encoding(relu_constraint, *variables, query));
    } else {
        solver.add(MARABOU_TO_Z3::generate_relu_as_ite(relu_constraint, *variables));
    }
}

//

bool MarabouInZ3Checker::check_marabou(const MARABOU_IN_PLAJA::MarabouQuery& query) {
    Z3_IN_PLAJA::SMTSolver solver(modelZ3.share_context());
    solver.push(); // to suppress scoped_timer thread in z3, which is started when using tactic2solver

    // generate variables and add bounds:
    generate_and_add_variables(query, solver);

    // add equations:
    for (const auto& equation: query._query().getEquations()) { solver.add(MARABOU_TO_Z3::generate_equation(equation, *variables)); }

    // add piecewise-linear constraints:
    for (const auto& pl_constraint: query._query().getPiecewiseLinearConstraints()) {
        switch (pl_constraint->getType()) {
            case PiecewiseLinearFunctionType::RELU: {
                const auto& relu_constraint = *static_cast<const ReluConstraint*>(pl_constraint); // NOLINT(cppcoreguidelines-pro-type-static-cast-downcast)
                PLAJA_ASSERT(FloatUtils::lt(query.get_lower_bound(relu_constraint.getB()), 0) && FloatUtils::gt(query.get_upper_bound(relu_constraint.getB()), 0)) // preprocessing should have fixed such cases
                add_relu(solver, relu_constraint, query);
                break;
            }
            case PiecewiseLinearFunctionType::DISJUNCTION: {
                solver.add(MARABOU_TO_Z3::generate_disjunction(*static_cast<const DisjunctionConstraint*>(pl_constraint), *variables)); // NOLINT(cppcoreguidelines-pro-type-static-cast-downcast)
                break;
            }
            default: PLAJA_ABORT
        }

    }

    bool rlt = solver.check();
    if (sharedStatistics) { // maintain exact nn-sat stats
        sharedStatistics->inc_attr_unsigned(PLAJA::StatsUnsigned::NN_SAT_QUERIES_Z3); // usually we set this prior to query; however ...
        if (solver.retrieve_unknown()) { sharedStatistics->inc_attr_unsigned(PLAJA::StatsUnsigned::NN_SAT_QUERIES_Z3_UNDECIDED); }
        else if (not rlt) { sharedStatistics->inc_attr_unsigned(PLAJA::StatsUnsigned::NN_SAT_QUERIES_Z3_UNSAT); }
    }
    return rlt;
}

bool MarabouInZ3Checker::check_marabou() {
    // MARABOU IN Z3
    if (preprocessQuery) {
        auto preprocessed_query = solverMarabou->preprocess();
        if (not preprocessed_query) { return false; } // preprocess & check preprocessed unsatisfiable
        return check_marabou(*preprocessed_query);
    } else {
        return check_marabou(solverMarabou->_query());
    }
}

bool MarabouInZ3Checker::check_() {

    if (relaxedForExact) {

        // first check relaxed
        auto rlt = check_relaxed_for_exact(nullptr); // TODO MarabouInZ3Checker does not support extracted bounds

        if (rlt == 0) { return false; } // relaxed is unsat
        else if (rlt == 1) { return true; } // relaxed is sat and found integer solution
        else { return check_marabou(); } // relaxed is sat, but found not integer solution so far

    } else { return check_marabou(); }

}