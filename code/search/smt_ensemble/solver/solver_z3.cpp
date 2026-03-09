//
// This file is part of the PlaJA code base.
// Copyright (c) (2023 - 2024) Chaahat Jain.
//

#include "solver_z3.h"
#include "../../fd_adaptions/timer.h"
#include "solution_veritas.h"
#include "statistics_smt_solver_veritas.h"
#include "../../predicate_abstraction/pa_states/abstract_state.h"
#include "../veritas_query.h"
#include "../../../stats/stats_base.h"
#include "../../factories/configuration.h"
#include "../../smt/context_z3.h"
#include "../../predicate_abstraction/smt/model_z3_pa.h"
#include "../../smt_ensemble/to_z3/veritas_to_z3.h"
#include "../../smt/solver/solution_z3.h"
#include "../../smt/context_z3.h"
#include "../../smt/vars_in_z3.h"
#include "../../smt/solver/smt_solver_z3.h"
#include "../../../parser/ast/expression/non_standard/predicates_expression.h"


VERITAS_IN_PLAJA::SolverZ3::SolverZ3(const PLAJA::Configuration& config, std::shared_ptr<VERITAS_IN_PLAJA::Context>&& c):
        VERITAS_IN_PLAJA::Solver(config, std::move(c))
        , modelZ3(*std::static_pointer_cast<ModelZ3PA>(config.get_sharable<ModelZ3>(PLAJA::SharableKey::MODEL_Z3))) {
            PLAJA_ASSERT(config.has_sharable(PLAJA::SharableKey::MODEL_Z3))
            ensembleSatSolver = std::make_unique<Z3_IN_PLAJA::SMTSolver>(modelZ3.share_context());
        }

VERITAS_IN_PLAJA::SolverZ3::SolverZ3(const PLAJA::Configuration& config, const VERITAS_IN_PLAJA::VeritasQuery& veritas_query):
        VERITAS_IN_PLAJA::Solver(config, veritas_query)
        , modelZ3(*std::static_pointer_cast<ModelZ3PA>(config.get_sharable<ModelZ3>(PLAJA::SharableKey::MODEL_Z3))) {
            PLAJA_ASSERT(config.has_sharable(PLAJA::SharableKey::MODEL_Z3))
            ensembleSatSolver = std::make_unique<Z3_IN_PLAJA::SMTSolver>(modelZ3.share_context());
        }

VERITAS_IN_PLAJA::SolverZ3::~SolverZ3() = default;

bool VERITAS_IN_PLAJA::SolverZ3::check_internal() {
    // ensembleSatSolver->dump_solver_to_file("query.z3");
    const bool rlt = ensembleSatSolver->check();
    if (sharedStatistics) { // maintain exact stats
        if (ensembleSatSolver->retrieve_unknown()) { sharedStatistics->inc_attr_unsigned(PLAJA::StatsUnsigned::VERITAS_QUERIES_UNDECIDED); }
        else if (!rlt) {     
            ensembleSatSolver->pop();
            sharedStatistics->inc_attr_unsigned(PLAJA::StatsUnsigned::VERITAS_QUERIES_UNSAT); 
        }
    }
    return rlt;
}

bool VERITAS_IN_PLAJA::SolverZ3::load_policy(ActionLabel_type action_label) {
// Add variables for each tree and output label.
    auto trees = jani2Ensemble->load_ensemble();
    if (jani2Ensemble->_do_applicability_filtering()) {
        auto appTrees = modelVeritas->get_applicability_filter(action_label);
        trees.add_trees(appTrees);
    }
    auto eq_trees = this->generateEqTrees(query->get_input_query());
    if (eq_trees.num_leaf_values() == 0) { return false; }
    auto policy_trees = eq_trees.make_multiclass(action_label, trees.num_leaf_values());
    trees.add_trees(policy_trees);

    variables->add_tree_vars(trees);
    auto num_labels = trees.num_leaf_values();
    auto num_trees = (trees.get_type() == veritas::AddTreeType::CLF_MEAN) ? trees.size()/num_labels : trees.size();
    for (auto label = 0; label < num_labels; ++label){
        for (auto tree = 0; tree < num_trees; ++tree) {
            VERITAS_TO_Z3::add_bounds(*ensembleSatSolver, variables->to_z3_expr(label, tree), VERITAS_IN_PLAJA::negative_infinity, VERITAS_IN_PLAJA::infinity);
        }
        VERITAS_TO_Z3::add_bounds(*ensembleSatSolver, variables->output_label_to_z3_expr(label), VERITAS_IN_PLAJA::negative_infinity, VERITAS_IN_PLAJA::infinity);
    }

    // Load policy: 
    ensembleSatSolver->add(VERITAS_TO_Z3::generate_ensemble_forwarding_structures(trees, *variables));
    return true;
}

bool VERITAS_IN_PLAJA::SolverZ3::check(int action_label) {
    if (sharedStatistics) { sharedStatistics->inc_attr_unsigned(PLAJA::StatsUnsigned::VERITAS_QUERIES); }
    // ensemble variables
    ensembleSatSolver->push();
    if (!variables) {
        variables = std::make_unique<Z3_IN_PLAJA::VeritasToZ3Vars>(modelZ3.get_context(), *(query->share_context()), Z3_IN_PLAJA::VeritasToZ3Vars::reuse(modelZ3.get_src_vars(), modelVeritas->get_state_indexes(0)), query->get_input_query());
    } 
    ensembleSatSolver->add(variables->get_bounds(query->share_context()));

    // add bounds for source variables (inputs + predicates)
    for (auto it = jani2Ensemble->init_input_feature_iterator(true); !it.end(); ++it) {
        if (not it.get_input_feature()->is_location()) { modelZ3.register_src_bound(*ensembleSatSolver, it.get_input_feature()->get_variable_id()); }
    }
    for (auto pred_it = modelZ3._predicates()->predicatesIterator(); !pred_it.end(); ++pred_it) { modelZ3.register_src_bounds(*ensembleSatSolver, *pred_it); }

    // output interface
    outputInterfacePerIndex = VERITAS_TO_Z3::generate_output_interfaces(jani2Ensemble->load_ensemble(), *variables);
    if (!load_policy(action_label)) {
        ensembleSatSolver->pop();
        return false;
    }
    ensembleSatSolver->add(outputInterfacePerIndex[action_label]);
    return check_internal();
}

std::unique_ptr<SolutionCheckerInstance> VERITAS_IN_PLAJA::SolverZ3::extract_solution_via_checker() {
    PLAJA_ASSERT(solutionCheckerInstance)
    auto lastSolution = ensembleSatSolver->extract_solution_via_checker(modelZ3, std::move(solutionCheckerInstance));
    ensembleSatSolver->pop();
    ensembleSatSolver->reset();
    return lastSolution;
}
