//
// This file is part of the PlaJA code base.
// Copyright (c) (2023 - 2024) Chaahat Jain.
//

#include <z3++.h>
#include "ensemble_sat_checker_z3.h"
#include "../../../../parser/ast/expression/non_standard/predicates_expression.h"
#include "../../../../stats/stats_base.h"
#include "../../../information/jani2ensemble/jani_2_ensemble.h"
#include "../../../information/input_feature_to_jani.h"
#include "../../../smt/solver/smt_solver_z3.h"
#include "../../../smt/solver/solution_check_wrapper_z3.h"
#include "../../../smt/solver/solution_z3.h"
#include "../../../smt/context_z3.h"
#include "../../../smt/vars_in_z3.h"
#include "../../../smt_ensemble/solver/solver_veritas.h"
#include "../../../smt_ensemble/to_z3/veritas_to_z3.h"
#include "../../pa_states/abstract_state.h"
#include "../../successor_generation/action_op/action_op_pa.h"
#include "../../smt/model_z3_pa.h"
#include "../../solution_checker_pa.h"
#include "../../solution_checker_instance_pa.h"
#include "../smt/action_op_veritas_pa.h"
#include "../smt/model_veritas_pa.h"

EnsembleSatCheckerZ3::EnsembleSatCheckerZ3(const Jani2Ensemble& jani_2_ensemble, const PLAJA::Configuration& config):
    EnsembleSatInZ3Base(jani_2_ensemble, config)
    , ensembleSatSolver(new Z3_IN_PLAJA::SMTSolver(modelZ3.share_context())) {

    ensemble_as_query = modelVeritas->make_query(true, false, 0);

    // ensemble variables
    variables = std::make_unique<Z3_IN_PLAJA::VeritasToZ3Vars>(modelZ3.get_context(), modelVeritas->get_context(), Z3_IN_PLAJA::VeritasToZ3Vars::reuse(modelZ3.get_src_vars(), modelVeritas->get_state_indexes(0)), ensemble_as_query->get_input_query());

    // add bounds for source variables (inputs + predicates)
    for (auto it = jani2Ensemble->init_input_feature_iterator(true); !it.end(); ++it) {
        if (not it.get_input_feature()->is_location()) { modelZ3.register_src_bound(*ensembleSatSolver, it.get_input_feature()->get_variable_id()); }
    }
    for (auto pred_it = modelZ3._predicates()->predicatesIterator(); !pred_it.end(); ++pred_it) { modelZ3.register_src_bounds(*ensembleSatSolver, *pred_it); }

    // output interface
    outputInterfacePerIndex = VERITAS_TO_Z3::generate_output_interfaces(jani2Ensemble->load_ensemble(), *variables);

    // ensembleSatSolver->dump_solver_to_file("z3_query.z3");
}

EnsembleSatCheckerZ3::~EnsembleSatCheckerZ3() = default;
//

void EnsembleSatCheckerZ3::load_policy(ActionLabel_type action_label) {
// Add variables for each tree and output label.
    auto trees = jani2Ensemble->load_ensemble();
    if (jani2Ensemble->_do_applicability_filtering()) {
        auto appTrees = modelVeritas->get_applicability_filter(action_label);
        trees.add_trees(appTrees);
    }
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
}

void EnsembleSatCheckerZ3::add_source_state_z3() {
    PLAJA_ASSERT(set_source_state)
    const auto& source = *set_source_state; // quick fix to make code more readable and more flexible towards adaptions of set_source_state

    // locations
    auto& c = modelZ3.get_context();
    const auto& veritas_vars = modelVeritas->get_state_indexes();
    for (const VariableIndex_type state_index: modelVeritas->get_input_locations()) {
        ensembleSatSolver->add(variables->to_z3_expr(veritas_vars.to_veritas(state_index)) == c().int_val(source.location_value(state_index)));
    }

    // add predicate constraints
    for (auto pred_it = source.init_pred_state_iterator(); !pred_it.end(); ++pred_it) {
        if (pred_it.predicate_value()) { ensembleSatSolver->add(modelZ3.get_src_predicate(pred_it.predicate_index())); }
        else { ensembleSatSolver->add(!modelZ3.get_src_predicate(pred_it.predicate_index())); }
    }

}

void EnsembleSatCheckerZ3::add_guard_z3() {
    PLAJA_ASSERT(set_action_op)
    modelZ3.get_action_op_pa(set_action_op_id).add_to_solver(*ensembleSatSolver);
}

void EnsembleSatCheckerZ3::add_guard_negation_z3() {
    PLAJA_ASSERT(set_action_op)
    const auto& action_op = modelZ3.get_action_op_pa(set_action_op_id);
    action_op.add_src_bounds(*ensembleSatSolver);
    ensembleSatSolver->add(!action_op._guards());
}

void EnsembleSatCheckerZ3::add_update_z3() {
    PLAJA_ASSERT(set_action_op)
    PLAJA_ASSERT(set_update_index != ACTION::noneUpdate)
    const auto& update = modelZ3.get_action_op_pa(set_action_op_id).get_update_pa(set_update_index);
    update.add_to_solver(*ensembleSatSolver);

}

void EnsembleSatCheckerZ3::add_target_predicate_state_z3() {
    PLAJA_ASSERT(set_action_op)
    PLAJA_ASSERT(set_update_index != ACTION::noneUpdate)
    PLAJA_ASSERT(set_target_state)
    const auto& update = modelZ3.get_action_op_pa(set_action_op_id).get_update_pa(set_update_index);
    update.add_predicates_to_solver(*ensembleSatSolver, *set_target_state);
}

void EnsembleSatCheckerZ3::add_output_interface_z3() {
    PLAJA_ASSERT(set_output_index != NEURON_INDEX::none)
    ensembleSatSolver->add(outputInterfacePerIndex[set_output_index]);
}

bool EnsembleSatCheckerZ3::check_z3() {
    ensembleSatSolver->push();
    
    add_source_state_z3(); // we always expect the source state
    add_output_interface_z3(); // as well as the action label --> NN-SAT(S,a)
    if (set_action_op) { // NN-SAT(S,g,a)
        PLAJA_ASSERT(set_output_index == jani2Ensemble->get_output_index(set_action_op->_action_label())) // sanity check
        add_guard_z3();
        if (set_update_index != ACTION::noneUpdate) { // NN-SAT(S,a,S')
            add_update_z3();
            add_target_predicate_state_z3(); // if update index is set, we also expect a target state
        }
        load_policy(set_output_index); // load policy with app filter trees
    }

    const bool rlt = ensembleSatSolver->check();

    // extract solution
    if constexpr (PLAJA_GLOBAL::reuseSolutions) {
        if (rlt) {
            lastSolution = ensembleSatSolver->extract_solution_via_checker(modelZ3, std::make_unique<SolutionCheckerInstancePa>(*solutionChecker, set_source_state.get(), set_action_label, set_action_op->_op_id(), set_update_index, set_target_state.get()));
            PLAJA_ASSERT(solutionChecker->check(*lastSolution->get_solution(0), set_source_state.get(), set_action_label, set_action_op->_op_id(), set_update_index, set_target_state.get(), set_target_state ? lastSolution->get_solution(1) : nullptr))
        }
    }

    if (sharedStatistics) { // maintain exact stats
        sharedStatistics->inc_attr_unsigned(PLAJA::StatsUnsigned::ENSEMBLE_SAT_QUERIES_Z3); // usually we set this prior to query; however ...
        if (ensembleSatSolver->retrieve_unknown()) { sharedStatistics->inc_attr_unsigned(PLAJA::StatsUnsigned::ENSEMBLE_SAT_QUERIES_Z3_UNDECIDED); }
        else if (!rlt) { sharedStatistics->inc_attr_unsigned(PLAJA::StatsUnsigned::ENSEMBLE_SAT_QUERIES_Z3_UNSAT); }
    }

    ensembleSatSolver->pop();
    ensembleSatSolver->reset(); // reset after each query
    return rlt;
}

bool EnsembleSatCheckerZ3::check_() {
        return check_z3();
}
