//
// This file is part of the PlaJA code base.
// Copyright (c) (2019 - 2021) Marcel Vinzent.
// See README.md in the top-level directory for licensing information.
//

#include <z3++.h>
#include "nn_sat_checker_z3.h"
#include "../../../../exception/not_implemented_exception.h"
#include "../../../../include/marabou_include/relu_constraint.h"
#include "../../../../parser/ast/expression/non_standard/predicates_expression.h"
#include "../../../../stats/stats_base.h"
#include "../../../information/jani2nnet/jani_2_nnet.h"
#include "../../../information/input_feature_to_jani.h"
#include "../../../smt/solver/smt_solver_z3.h"
#include "../../../smt/solver/solution_check_wrapper_z3.h"
#include "../../../smt/solver/solution_z3.h"
#include "../../../smt/context_z3.h"
#include "../../../smt/vars_in_z3.h"
#include "../../../smt_nn/solver/smt_solver_marabou.h"
#include "../../../smt_nn/to_z3/marabou_to_z3.h"
#include "../../pa_states/abstract_state.h"
#include "../../successor_generation/action_op/action_op_pa.h"
#include "../../smt/model_z3_pa.h"
#include "../../solution_checker_pa.h"
#include "../../solution_checker_instance_pa.h"
#include "../smt/action_op_marabou_pa.h"
#include "../smt/model_marabou_pa.h"

NNSatCheckerZ3::NNSatCheckerZ3(const Jani2NNet& jani_2_nnet, const PLAJA::Configuration& config):
    NNSatInZ3Base(jani_2_nnet, config)
    , nnSatSolver(new Z3_IN_PLAJA::SMTSolver(modelZ3.share_context())) {

    auto nn_as_query = modelMarabou->make_query(true, false, 0);

    // nn variables
    variables = std::make_unique<Z3_IN_PLAJA::MarabouToZ3Vars>(modelZ3.get_context(), modelMarabou->get_context(), Z3_IN_PLAJA::MarabouToZ3Vars::reuse(modelZ3.get_src_vars(), modelMarabou->get_state_indexes(0)));

    // add bounds for source variables (inputs + predicates)
    for (auto it = jani2NNet->init_input_feature_iterator(true); !it.end(); ++it) {
        if (not it.get_input_feature()->is_location()) { modelZ3.register_src_bound(*nnSatSolver, it.get_input_feature()->get_variable_id()); }
    }
    for (auto pred_it = modelZ3._predicates()->predicatesIterator(); !pred_it.end(); ++pred_it) { modelZ3.register_src_bounds(*nnSatSolver, *pred_it); }

    // compute equations:
    for (const auto& equation: nn_as_query->_query().getEquations()) { nnSatSolver->add(MARABOU_TO_Z3::generate_equation(equation, *variables)); }

    // cache ReLU constraints:
    reluConstraints.reserve(nn_as_query->_query().getPiecewiseLinearConstraints().size());
    for (const auto* pl_constraint: nn_as_query->_query().getPiecewiseLinearConstraints()) {
        reluConstraints.push_back(PLAJA_UTILS::cast_ptr<ReluConstraint>(pl_constraint->duplicateConstraint()));
    }

    // output interface
    if (jani_2_nnet._do_applicability_filtering()) { throw NotImplementedException(__PRETTY_FUNCTION__); }
    outputInterfacePerIndex = MARABOU_TO_Z3::generate_output_interfaces(modelMarabou->get_nn_in_marabou(), *variables);

}

NNSatCheckerZ3::~NNSatCheckerZ3() = default;

/**********************************************************************************************************************/

void NNSatCheckerZ3::add_relu_constraints_z3(const MARABOU_IN_PLAJA::PreprocessedBounds* preprocessed_bounds) {
    if (preprocessed_bounds) {
        encodingInformation->set_preprocessed_bounds(preprocessed_bounds); // set preprocessed bounds
        //
        if (encodingInformation->_mip_encoding()) {
            for (const auto* relu_constraint: reluConstraints) {
                nnSatSolver->add(MARABOU_TO_Z3::fix_or_generate_relu_as_mip_encoding(*relu_constraint, *variables, *encodingInformation));
            }
        } else {
            for (const auto* relu_constraint: reluConstraints) {
                nnSatSolver->add(MARABOU_TO_Z3::fix_or_generate_relu_as_ite(*relu_constraint, *variables, *encodingInformation));
            }
        }
        //
        encodingInformation->set_preprocessed_bounds(nullptr); // unset preprocessed bounds !!!
    } else {
        PLAJA_ASSERT(!encodingInformation->_mip_encoding()) // for mip we need bounds
        for (const auto* relu_constraint: reluConstraints) {
            nnSatSolver->add(MARABOU_TO_Z3::generate_relu_as_ite(*relu_constraint, *variables));
        }
    }

}

/**********************************************************************************************************************/

void NNSatCheckerZ3::add_source_state_z3() {
    PLAJA_ASSERT(set_source_state)
    const auto& source = *set_source_state; // quick fix to make code more readable and more flexible towards adaptions of set_source_state

    // locations
    auto& c = modelZ3.get_context();
    const auto& marabou_vars = modelMarabou->get_state_indexes();
    for (const VariableIndex_type state_index: modelMarabou->get_input_locations()) {
        nnSatSolver->add(variables->to_z3_expr(marabou_vars.to_marabou(state_index)) == c().int_val(source.location_value(state_index)));
    }

    // add predicate constraints
    for (auto pred_it = source.init_pred_state_iterator(); !pred_it.end(); ++pred_it) {
        if (pred_it.predicate_value()) { nnSatSolver->add(modelZ3.get_src_predicate(pred_it.predicate_index())); }
        else { nnSatSolver->add(!modelZ3.get_src_predicate(pred_it.predicate_index())); }
    }

}

void NNSatCheckerZ3::add_guard_z3() {
    PLAJA_ASSERT(set_action_op)
    modelZ3.get_action_op_pa(set_action_op_id).add_to_solver(*nnSatSolver);
}

void NNSatCheckerZ3::add_guard_negation_z3() {
    PLAJA_ASSERT(set_action_op)
    const auto& action_op = modelZ3.get_action_op_pa(set_action_op_id);
    action_op.add_src_bounds(*nnSatSolver);
    nnSatSolver->add(!action_op._guards());
}

void NNSatCheckerZ3::add_update_z3() {
    PLAJA_ASSERT(set_action_op)
    PLAJA_ASSERT(set_update_index != ACTION::noneUpdate)
    const auto& update = modelZ3.get_action_op_pa(set_action_op_id).get_update_pa(set_update_index);
    update.add_to_solver(*nnSatSolver);

}

void NNSatCheckerZ3::add_target_predicate_state_z3() {
    PLAJA_ASSERT(set_action_op)
    PLAJA_ASSERT(set_update_index != ACTION::noneUpdate)
    PLAJA_ASSERT(set_target_state)
    const auto& update = modelZ3.get_action_op_pa(set_action_op_id).get_update_pa(set_update_index);
    update.add_predicates_to_solver(*nnSatSolver, *set_target_state);
}

void NNSatCheckerZ3::add_output_interface_z3() {
    PLAJA_ASSERT(set_output_index != NEURON_INDEX::none)
    nnSatSolver->add(outputInterfacePerIndex[set_output_index]);
}

bool NNSatCheckerZ3::check_z3(const MARABOU_IN_PLAJA::PreprocessedBounds* preprocessed_bounds) {
    nnSatSolver->push();
    add_source_state_z3(); // we always expect the source state
    add_output_interface_z3(); // as well as the action label --> NN-SAT(S,a)
    if (set_action_op) { // NN-SAT(S,g,a)
        PLAJA_ASSERT(set_output_index == jani2NNet->get_output_index(set_action_op->_action_label())) // sanity check
        add_guard_z3();
        if (set_update_index != ACTION::noneUpdate) { // NN-SAT(S,a,S')
            add_update_z3();
            add_target_predicate_state_z3(); // if update index is set, we also expect a target state
        }
    }
    add_relu_constraints_z3(preprocessed_bounds); // add relu (and bounds derived)
    const bool rlt = nnSatSolver->check();

    // extract solution
    if constexpr (PLAJA_GLOBAL::reuseSolutions) {
        if (rlt) {
            lastSolution = nnSatSolver->extract_solution_via_checker(modelZ3, std::make_unique<SolutionCheckerInstancePa>(*solutionChecker, set_source_state.get(), set_action_label, set_action_op->_op_id(), set_update_index, set_target_state.get()));
            PLAJA_ASSERT(solutionChecker->check(*lastSolution->get_solution(0), set_source_state.get(), set_action_label, set_action_op->_op_id(), set_update_index, set_target_state.get(), set_target_state ? lastSolution->get_solution(1) : nullptr))
        }
    }

    if (sharedStatistics) { // maintain exact nn-sat stats
        sharedStatistics->inc_attr_unsigned(PLAJA::StatsUnsigned::NN_SAT_QUERIES_Z3); // usually we set this prior to query; however ...
        if (nnSatSolver->retrieve_unknown()) { sharedStatistics->inc_attr_unsigned(PLAJA::StatsUnsigned::NN_SAT_QUERIES_Z3_UNDECIDED); }
        else if (!rlt) { sharedStatistics->inc_attr_unsigned(PLAJA::StatsUnsigned::NN_SAT_QUERIES_Z3_UNSAT); }
    }

    nnSatSolver->pop();
    nnSatSolver->reset(); // reset after each nn-sat query
    return rlt;
}

bool NNSatCheckerZ3::check_() {
    if (relaxedForExact) {

        // first check relaxed
        std::unique_ptr<MARABOU_IN_PLAJA::PreprocessedBounds> preprocessed_bounds(nullptr);
        auto* preprocessed_bounds_ptr = preprocessQuery ? &preprocessed_bounds : nullptr; // extract bounds if z3 query is to be preprocessed ...
        auto rlt = check_relaxed_for_exact(preprocessed_bounds_ptr);

        if (rlt == 0) { return false; } // relaxed is unsat
        else if (rlt == 1) { return true; } // relaxed is sat and found integer solution
        else { return check_z3(preprocessed_bounds.get()); } // relaxed is sat, but found not integer solution so far

    } else {

        if (preprocessQuery) {
            auto preprocessed_bounds_locally = solverMarabou->preprocess_bounds();
            // preprocessed unsatisfiable ?
            if (not preprocessed_bounds_locally) { return false; }
            return check_z3(preprocessed_bounds_locally.get());
        } else { return check_z3(nullptr); }

    }
}
