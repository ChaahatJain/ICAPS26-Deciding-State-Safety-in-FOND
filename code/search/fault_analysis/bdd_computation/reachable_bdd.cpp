//
// This file is part of the PlaJA code base.
// Copyright (c) (2023 - 2024) Chaahat Jain.
//

#include "reachable_bdd.h"
#include <memory>
#include "../../factories/configuration.h"
#include "../../factories/fault_analysis/fault_analysis_options.h"
#include "../../../parser/ast/expression/expression.h"
#include "../../../parser/ast/expression/non_standard/state_condition_expression.h"
#include "../../../parser/ast/expression/special_cases/nary_expression.h"
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

ReachableBDD::ReachableBDD(const PLAJA::Configuration& config):
    SearchEngine(config)
    , query()
    , modelZ3(new ModelZ3(config))
    , solverZ3(PLAJA_UTILS::cast_unique<Z3_IN_PLAJA::SMTSolver>(modelZ3->init_solver(config, 1)))
    , reachablePath(config.has_value_option(PLAJA_OPTION::reachable_path) ? config.get_value_option_string(PLAJA_OPTION::reachable_path) : "")
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

    const auto* start_exp = propertyInfo->get_start();
    PLAJA_ASSERT(start_exp)
    auto lin_start = dynamic_cast<const StateConditionExpression*>(start_exp);
    reachable = query.state_condition_to_bdd(lin_start);
    // std::cout << Cudd_DagSize(reachable.getNode()) << std::endl;
    const auto* reach_exp = propertyInfo->get_reach();
    PLAJA_ASSERT(reach_exp)
    auto lin_reach = dynamic_cast<const StateConditionExpression*>(reach_exp);
    unsafe = query.state_condition_to_bdd(lin_reach);
    rho = query.get_false();
    // query.printSatisfyingAssignment(reachable & !unsafe, {unsafe, reachable});
}

ReachableBDD::~ReachableBDD() = default;

/**********************************************************************************************************************/

SearchEngine::SearchStatus ReachableBDD::initialize() { 
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

SearchEngine::SearchStatus ReachableBDD::finalize() { return SearchStatus::FINISHED; }

SearchEngine::SearchStatus ReachableBDD::step() {
    searchStatistics->inc_attr_unsigned(PLAJA::StatsUnsigned::ITERATIONS);
    const auto& suc_gen = modelZ3->get_successor_generator();

    rho = rho | (reachable & !unsafe);
    std::cout << Cudd_DagSize(rho.getNode()) << std::endl;
    std::vector<BDD> reachable_disjuncts;

    for (auto it_action = suc_gen.init_action_id_it(false); !it_action.end(); ++it_action) {
        const auto action_label = it_action.get_label();
        

        /* Iterate ops. */
        for (auto it_op = suc_gen.init_action_it_static(action_label); !it_op.end(); ++it_op) {
            const auto& action_op = it_op.operator*();
            auto guard = query.get_guard(action_op._op_id());

            PLAJA_ABORT_IF(query.get_true() == guard)
            auto updates = query.get_updates(action_op._op_id());
            std::vector<BDD> b_upds;
            
            for (auto u : updates) {
                PLAJA_ABORT_IF(query.get_true() == u)
                reachable_disjuncts.push_back(query.get_relational_product(reachable & guard, u, true));
            }
        }

    }

    reachable = query.computeDisjunctionTree(reachable_disjuncts);
    reachable = query.target_to_source_variables(reachable);
    if (reachable <= rho) {
        std::cout << Cudd_DagSize(rho.getNode()) << std::endl;
        if (reachablePath.size() > 0) {
            query.saveBDD(reachablePath.c_str(), rho);
        }
        return SearchEngine::FINISHED;
    }
    return SearchEngine::IN_PROGRESS;
}

/* Override stats. */

void ReachableBDD::print_statistics() const { searchStatistics->print_statistics(); }

void ReachableBDD::stats_to_csv(std::ofstream& file) const { searchStatistics->stats_to_csv(file); }

void ReachableBDD::stat_names_to_csv(std::ofstream& file) const { searchStatistics->stat_names_to_csv(file); }


