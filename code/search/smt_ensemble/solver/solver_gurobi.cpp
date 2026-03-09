//
// This file is part of the PlaJA code base.
// Copyright (c) (2023 - 2024) Chaahat Jain.
//

#include "solver_gurobi.h"
#include "../../fd_adaptions/timer.h"
#include "solution_veritas.h"
#include "statistics_smt_solver_veritas.h"
#include "../../predicate_abstraction/pa_states/abstract_state.h"
#include "../veritas_query.h"
#include "../../../stats/stats_base.h"
#include "../../factories/configuration.h"

VERITAS_IN_PLAJA::SolverGurobi::SolverGurobi(const PLAJA::Configuration& config, std::shared_ptr<VERITAS_IN_PLAJA::Context>&& c):
        VERITAS_IN_PLAJA::Solver(config, std::move(c)), gurobi_model(nullptr), env(GRBEnv(true)) {
            env.set(GRB_IntParam_OutputFlag, 0);
            env.set(GRB_IntParam_LogToConsole, 0);
            env.start();
        }

VERITAS_IN_PLAJA::SolverGurobi::SolverGurobi(const PLAJA::Configuration& config, const VERITAS_IN_PLAJA::VeritasQuery& veritas_query):
        VERITAS_IN_PLAJA::Solver(config, veritas_query), gurobi_model(nullptr), env(GRBEnv(true)) {
            env.set(GRB_IntParam_OutputFlag, 0);
            env.set(GRB_IntParam_LogToConsole, 0);
            env.start();
        }

VERITAS_IN_PLAJA::SolverGurobi::~SolverGurobi() = default;

bool VERITAS_IN_PLAJA::SolverGurobi::check_internal() {
    try {
        gurobi_model->optimize();
    }
    catch (GRBException e)
    {
        std::cout << "Error code = " << e.getErrorCode() << std::endl;
        std::cout << e.getMessage() << std::endl;
    }
    if (gurobi_model->get(GRB_IntAttr_Status) == GRB_INFEASIBLE) {
        if (sharedStatistics) {
            sharedStatistics->inc_attr_unsigned(PLAJA::StatsUnsigned::VERITAS_QUERIES_UNSAT);
        }
        return false;
    } 

    return true;
}

bool VERITAS_IN_PLAJA::SolverGurobi::check(int action_label) {
    if (sharedStatistics) { sharedStatistics->inc_attr_unsigned(PLAJA::StatsUnsigned::VERITAS_QUERIES); }
    gurobi_model = std::make_shared<GRBModel>(env);
    gurobi_model->set(GRB_IntParam_OutputFlag, 0);
    gurobi_model->set(GRB_IntParam_LogToConsole, 0);
    auto veritas_tree = query->get_input_query();

    if constexpr (PLAJA_GLOBAL::enableApplicabilityFiltering) {
        if (VERITAS_IN_PLAJA::Solver::is_filtering_enabled()) {
            auto appTrees = VERITAS_IN_PLAJA::Solver::appFilterTrees;
            veritas_tree.add_trees(appTrees);
        }
    }
    auto eq_trees = this->generateEqTrees(query->get_input_query());
    if (eq_trees.num_leaf_values() == 0) { return false; }
    auto policy_trees = eq_trees.make_multiclass(action_label, veritas_tree.num_leaf_values());
    veritas_tree.add_trees(policy_trees);

    // ensemble variables
    if (veritas_tree.get_type() == veritas::AddTreeType::CLF_SOFTMAX) {
        variables = std::make_unique<GUROBI_IN_PLAJA::VeritasToGurobiVars>(gurobi_model, query->generate_input_list(), veritas_tree, action_label);
        variables->add_equations(this->getEquations());
    } else {
        rf_variables = std::make_unique<GUROBI_IN_PLAJA::RFVeritasToGurobiVars>(gurobi_model, query->generate_input_list(), veritas_tree, action_label);
        rf_variables->add_equations(this->getEquations());
    }
    gurobi_model->write("test.lp");
    return check_internal();
}

VERITAS_IN_PLAJA::Solution VERITAS_IN_PLAJA::SolverGurobi::extract_solution() {
    PLAJA_ASSERT(gurobi_model->get(GRB_IntAttr_Status) != GRB_INFEASIBLE)
    GRBVar* vars = NULL;
    vars = gurobi_model->getVars();
    VERITAS_IN_PLAJA::Solution solution(*query.get());
    // Print the values of all variables
    
    int num_features = 0;
    // CLF_SOFTMAX veritas::AddTree encodes one ensemble per class. Hence distinction has to be made from rest of implementation.
    if (query->get_input_query().get_type() == veritas::AddTreeType::CLF_SOFTMAX) {
        num_features = variables->num_features();
    } else {
        num_features = rf_variables->num_features();
    }
    
    for (int i = 0; i < num_features; i++) {
        solution.set_value(i, vars[i].get(GRB_DoubleAttr_X));
    }
    return solution;
}

std::unique_ptr<SolutionCheckerInstance> VERITAS_IN_PLAJA::SolverGurobi::extract_solution_via_checker() {
    auto solutionCheckerInstanceWrapper = std::make_unique<VERITAS_IN_PLAJA::SolutionCheckWrapper>(std::move(solutionCheckerInstance), *modelVeritas);

    PLAJA_ASSERT(solutionCheckerInstanceWrapper)

    if (solutionCheckerInstanceWrapper->is_invalidated()) {
        solutionCheckerInstanceWrapper = nullptr; // Unset wrapper.
        return nullptr;
    }

    if (not solutionCheckerInstanceWrapper->has_solution()) { // May have been set already.

        PLAJA_ASSERT(gurobi_model->get(GRB_IntAttr_Status) != GRB_INFEASIBLE)

        const auto solution = extract_solution();

        if (solution.clipped_out_of_bounds()) {
            solutionCheckerInstanceWrapper = nullptr; // Unset wrapper.
            return nullptr;
        }

        solutionCheckerInstanceWrapper->set(solution);
    }

    PLAJA_ASSERT(not solutionCheckerInstanceWrapper->is_invalidated())
    auto solution_checker_instance = solutionCheckerInstanceWrapper->release_instance();
    solutionCheckerInstanceWrapper = nullptr; // Unset wrapper.
    return solution_checker_instance;
}
