//
// This file is part of the PlaJA code base.
// Copyright (c) (2023 - 2024) Chaahat Jain.
//

#include "unsafe_bdd.h"
#include <memory>
#include "../../factories/configuration.h"
#include "../../factories/fault_analysis/fault_analysis_options.h"
#include "../../../parser/ast/expression/expression.h"
#include "../../../parser/ast/expression/non_standard/objective_expression.h"
#include "../../../parser/ast/expression/non_standard/state_condition_expression.h"
#include "../../../parser/visitor/to_normalform.h"
#include "../../successor_generation/update.h"
#include "../../successor_generation/action_op.h"
#include "../../successor_generation/successor_generator_c.h"
#include "../../information/jani_2_interface.h"
#include "../../information/property_information.h"
#include "../../information/model_information.h"
#include "../../../globals.h"
#include "../../smt/model/model_z3.h"
#include "../../smt/solver/smt_solver_z3.h"
#include "../../smt/solver/solution_z3.h"
#include "../../smt/solver/statistics_smt_solver_z3.h"
#include "../../smt/vars_in_z3.h"
#include "../../states/state_values.h"

using namespace std;

UnsafeBDD::UnsafeBDD(const PLAJA::Configuration& config):
    SearchEngine(config)
    , query()
    , modelZ3(new ModelZ3(config))
    , solverZ3(PLAJA_UTILS::cast_unique<Z3_IN_PLAJA::SMTSolver>(modelZ3->init_solver(config, 1)))
    , unsafePath(config.has_value_option(PLAJA_OPTION::unsafe_path) ? config.get_value_option_string(PLAJA_OPTION::unsafe_path) : "")
    {

    Z3_IN_PLAJA::add_solver_stats(*searchStatistics);
    searchStatistics->add_unsigned_stats(PLAJA::StatsUnsigned::ITERATIONS, "Iterations", 0);

    PLAJA_ASSERT(modelZ3->has_ensemble());

    const auto& model_info = PLAJA_GLOBAL::currentModel->get_model_information();
    int num_inputs = model_info.get_state_size();
    for (int state_index = 1; state_index < num_inputs; ++state_index) {
        // source variables
        query.add_var_to_query(state_index - 1, model_info.get_lower_bound_int(state_index), model_info.get_upper_bound_int(state_index));
    }
    for (int state_index = 1; state_index < num_inputs; ++state_index) {
        // target variables
        query.add_var_to_query(num_inputs + state_index - 2, model_info.get_lower_bound_int(state_index), model_info.get_upper_bound_int(state_index));
    }
    query.reorderVariables(false);
    // query.dump();
    const auto* reach_exp = propertyInfo->get_reach();
    PLAJA_ASSERT(reach_exp)
    auto lin_reach = dynamic_cast<const StateConditionExpression*>(reach_exp);
    tau = query.state_condition_to_bdd(lin_reach);
    rho = query.get_false();
    // reach_exp->dump(true);
    reachable = query.get_true();
    if (config.get_bool_option(PLAJA_OPTION::reachable_prop)) {
        const auto* obj_exp = propertyInfo->get_learning_objective();
        auto goal_exp = dynamic_cast<const StateConditionExpression*>(obj_exp->get_goal());
        reachable = query.state_condition_to_bdd(goal_exp);
    }
    reachable &= config.has_value_option(PLAJA_OPTION::reachable_path) ? query.loadBDD(config.get_value_option_string(PLAJA_OPTION::reachable_path).c_str()) : query.get_true();

    std::cout << "Reachable: " << Cudd_DagSize(reachable.getNode()) << std::endl;
    // std::cout << "Reachable: " << reachable << std::endl;
    // query.printSatisfyingAssignment(tau & !rho, {rho, tau});
}

UnsafeBDD::~UnsafeBDD() = default;

/**********************************************************************************************************************/

SearchEngine::SearchStatus UnsafeBDD::initialize() { 
    // TODO: Compute the BDD for every action operator here. Store it as <guard, update> in context which can be used in future.
    const auto& suc_gen = modelZ3->get_successor_generator();
     for (auto it_action = suc_gen.init_action_id_it(false); !it_action.end(); ++it_action) {
        const auto action_label = it_action.get_label();
        /* Iterate ops. */
        for (auto it_op = suc_gen.init_action_it_static(action_label); !it_op.end(); ++it_op) {
            const auto& action_op = it_op.operator*();
            query.operator_to_bdd(action_op);
        }
    }
    query.reorderVariables(true);
    return SearchEngine::IN_PROGRESS;
}

SearchEngine::SearchStatus UnsafeBDD::finalize() { return SearchStatus::FINISHED; }

SearchEngine::SearchStatus UnsafeBDD::step() {
    searchStatistics->inc_attr_unsigned(PLAJA::StatsUnsigned::ITERATIONS);
    const auto& suc_gen = modelZ3->get_successor_generator();

    rho = rho | tau;
    auto last_tau = query.source_to_target_variables(tau);
    std::vector<BDD> tau_conjuncts;
    tau_conjuncts.push_back(reachable);
    tau_conjuncts.push_back(!rho);
    std::vector<BDD> guard_disjunctions;
    // std::vector<BDD> debug;
    for (auto it_action = suc_gen.init_action_id_it(false); !it_action.end(); ++it_action) {
        const auto action_label = it_action.get_label();
        std::vector<BDD> b_actions;
        
        std::vector<BDD> b_guards;

        /* Iterate ops. */
        for (auto it_op = suc_gen.init_action_it_static(action_label); !it_op.end(); ++it_op) {
            const auto& action_op = it_op.operator*();
            // action_op.dump(nullptr, true);
            auto guard = query.get_guard(action_op._op_id());

            PLAJA_ABORT_IF(query.get_true() == guard)
            

            auto updates = query.get_updates(action_op._op_id());
            std::vector<BDD> b_upds;
            
            for (auto u : updates) {
                PLAJA_ABORT_IF(query.get_true() == u)
                b_upds.push_back(query.get_relational_product(last_tau, u));
            }
            b_actions.push_back(guard & query.computeDisjunctionTree(b_upds));
            b_guards.push_back(!guard);
            guard_disjunctions.push_back(guard);
        }
        // debug.push_back(query.computeConjunctionTree(b_guards));
        b_actions.push_back(query.computeConjunctionTree(b_guards));
        tau_conjuncts.push_back(query.computeDisjunctionTree(b_actions));
        // debug.push_back(query.computeDisjunctionTree(b_actions));
        // std::cout << "Size for action label: " << action_label << " " << Cudd_DagSize(query.computeDisjunctionTree(b_actions).getNode()) << std::endl;
    }
    // cout << query.computeDisjunctionTree(guard_disjunctions) << endl;
    tau_conjuncts.push_back(query.computeDisjunctionTree(guard_disjunctions));
    tau = query.computeConjunctionTree(tau_conjuncts);
    // query.generateCounterExample({0, 1, 2, 60, 74, 75, 77}, {3, 4, 5, 6, 8, 9, 10, 12, 13, 14, 16, 17, 19, 20, 22, 23, 28, 29, 31, 32, 34,35, 48, 49, 50, 51, 52, 53, 54, 55, 56, 57, 58, 59, 61, 64, 69, 70, 71}, debug);
    std::cout << "Int Tau size: " << Cudd_DagSize(tau.getNode()) << std::endl;
    // query.printSatisfyingAssignment(tau & !rho, {});
    if (tau <= rho) {
        std::cout << "Finished rho: " << Cudd_DagSize(rho.getNode()) << std::endl;
        // std::cout << "Finished rho: " << rho << std::endl;
        if (unsafePath.size() > 0) {
            query.saveBDD(unsafePath.c_str(), rho);
        }
        return SearchEngine::FINISHED;
    }
    return SearchEngine::IN_PROGRESS;
}

/* Override stats. */

void UnsafeBDD::print_statistics() const { searchStatistics->print_statistics(); }

void UnsafeBDD::stats_to_csv(std::ofstream& file) const { searchStatistics->stats_to_csv(file); }

void UnsafeBDD::stat_names_to_csv(std::ofstream& file) const { searchStatistics->stat_names_to_csv(file); }


