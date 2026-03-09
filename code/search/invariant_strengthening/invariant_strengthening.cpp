//
// This file is part of the PlaJA code base.
// Copyright (c) (2019 - 2021) Marcel Vinzent.
// See README.md in the top-level directory for licensing information.
//

#include "invariant_strengthening.h"
#include <memory>
#include "../../parser/ast/expression/expression.h"
#include "../../parser/visitor/to_normalform.h"
#include "../successor_generation/action_op.h"
#include "../successor_generation/successor_generator_c.h"
#include "../information/jani2nnet/jani_2_nnet.h"
#include "../information/jani_2_interface.h"
#include "../information/property_information.h"
#include "../smt/model/model_z3.h"
#include "../smt/solver/smt_solver_z3.h"
#include "../smt/solver/solution_z3.h"
#include "../smt/solver/statistics_smt_solver_z3.h"
#include "../smt/vars_in_z3.h"
#include "../smt_nn/model/model_marabou.h"
#include "../smt_nn/solver/smt_solver_marabou.h"
#include "../smt_nn/solver/solution_marabou.h"
#include "../smt_nn/solver/statistics_smt_solver_marabou.h"
#include "../smt_nn/vars_in_marabou.h"
#include "../smt_ensemble/model/model_veritas.h"
#include "../smt_ensemble/solver/solver_veritas.h"
#include "../smt_ensemble/solver/solution_veritas.h"
#include "../smt_ensemble/solver/statistics_smt_solver_veritas.h"
#include "../smt_ensemble/vars_in_veritas.h"
#include "../states/state_values.h"

InvariantStrengthening::InvariantStrengthening(const PLAJA::Configuration& config):
    SearchEngine(config)
    , modelZ3(new ModelZ3(config))
    , solverZ3(PLAJA_UTILS::cast_unique<Z3_IN_PLAJA::SMTSolver>(modelZ3->init_solver(config, 1)))
    , modelMarabou(nullptr)
    , solverMarabou(nullptr) {

    // Non-Reach as initial invariant.
    const auto* reach_exp = propertyInfo->get_reach();
    PLAJA_ASSERT(reach_exp)
    nonInvariant = reach_exp->deepCopy_Exp();
    // Start as initial invariant
    // const auto* start_exp = propertyInfo->get_start();
    // invariant = start_exp->deepCopy_Exp();
    // invariant->dump(true);
    invariant = nonInvariant->deepCopy_Exp();
    TO_NORMALFORM::negate(invariant);
    TO_NORMALFORM::normalize(invariant);
    TO_NORMALFORM::specialize(invariant);

    /* Encode invariant. */
    Z3_IN_PLAJA::add_solver_stats(*searchStatistics);
    modelZ3->add_to_solver(*solverZ3, *invariant, 0);

    if (modelZ3->has_nn()) {
        MARABOU_IN_PLAJA::add_solver_stats(*searchStatistics);
        modelMarabou = std::make_unique<ModelMarabou>(config);
        solverMarabou = PLAJA_UTILS::cast_unique<MARABOU_IN_PLAJA::SMTSolver>(modelMarabou->init_solver(config, 1));
        modelMarabou->add_nn_to_query(solverMarabou->_query(), 0); // Encode policy.
        modelMarabou->add_to_solver(*solverMarabou, *invariant, 0);
    } else if (modelZ3->has_ensemble()) {
        VERITAS_IN_PLAJA::add_solver_stats(*searchStatistics);
        modelVeritas = std::make_unique<ModelVeritas>(config);
        solver = VERITAS_IN_PLAJA::ENSEMBLE_SOLVER::construct(config, *modelVeritas->make_query(true, true, 1));
        solver->set_invariant_strengthening();
        modelVeritas->add_to_solver(*solver, *invariant, 0); invariant->dump(true);
    }

    searchStatistics->add_unsigned_stats(PLAJA::StatsUnsigned::ITERATIONS, "Iterations", 0);
}

InvariantStrengthening::~InvariantStrengthening() = default;

/**********************************************************************************************************************/

SearchEngine::SearchStatus InvariantStrengthening::initialize() { return SearchEngine::IN_PROGRESS; }

SearchEngine::SearchStatus InvariantStrengthening::finalize() { return SearchStatus::FINISHED; }

/**/

void InvariantStrengthening::restrict(const StateValues& state) {
    auto state_condition = state.to_condition(not modelZ3->ignore_locs(), modelZ3->get_model());

    /* Invariant. */
    {
        std::list<std::unique_ptr<Expression>> conjuncts = TO_NORMALFORM::split_conjunction(std::move(invariant), false);
        conjuncts.push_back(state_condition->deepCopy_Exp());
        invariant = TO_NORMALFORM::construct_conjunction(std::move(conjuncts));
        TO_NORMALFORM::normalize(invariant);
        TO_NORMALFORM::specialize(invariant);
    }

    /* Non-Invariant. */
    {
        std::list<std::unique_ptr<Expression>> disjuncts = TO_NORMALFORM::split_disjunction(std::move(nonInvariant), false);
        TO_NORMALFORM::negate(state_condition);
        disjuncts.push_back(std::move(state_condition));
        nonInvariant = TO_NORMALFORM::construct_disjunction(std::move(disjuncts));
        TO_NORMALFORM::normalize(nonInvariant);
        TO_NORMALFORM::specialize(nonInvariant);
    }

}

void InvariantStrengthening::restrict(const std::vector<std::vector<veritas::Interval>>& boxes) {
    for (auto box : boxes) {
        restrict(box);
    }
}

void InvariantStrengthening::restrict(const std::vector<veritas::Interval>& box) {
    PLAJA_ASSERT(modelZ3->has_ensemble())
    solver->set_prune_boxes(true);
    auto num_actions = modelZ3->get_model().get_number_actions();
    solver->add_box_trees(boxTree(box, num_actions)); // remove box from being searched over ever again.
    // TODO: Restrict non invariant to nonInv OR box1
    debug_solutions.push_back(box);
    // dump(box);
    count += 1;

}

/**/
SearchEngine::SearchStatus InvariantStrengthening::step() {
    searchStatistics->inc_attr_unsigned(PLAJA::StatsUnsigned::ITERATIONS);
    if (modelZ3->has_ensemble()) {
        return step_ensemble();
    }
    return step_nn();
}

SearchEngine::SearchStatus InvariantStrengthening::step_ensemble() {
    const bool has_ensemble = modelZ3->has_ensemble();
    PLAJA_ASSERT(not has_ensemble or (modelVeritas and solver))
    const bool do_locs = not modelZ3->ignore_locs();
    const auto& suc_gen = modelZ3->get_successor_generator();
    /*
     * Find a s not in S_inv
     * -- for each action op check whether there exists a transition leaving the invariant.
     */

    // bool violation_found = false;
    solverZ3->push();
    if (has_ensemble) { solver->push(); }

    /* Add non-invariant. */
    modelZ3->add_to_solver(*solverZ3, *nonInvariant, 1);
    if (modelVeritas) { modelVeritas->add_to_solver(*solver, *nonInvariant, 1); }

    /* Iterate labels. */
    for (auto it_action = suc_gen.init_action_id_it(true); !it_action.end(); ++it_action) {
        const auto action_label = it_action.get_label();

        /* Iterate ops. */
        for (auto it_op = suc_gen.init_action_it_static(action_label); !it_op.end(); ++it_op) {
            const auto& action_op = it_op.operator*();

            for (auto it_upd = action_op.updateIterator(); !it_upd.end(); ++it_upd) {

                /* Check non-policy transition. */
                {
                    solverZ3->push();
                    modelZ3->add_action_op(*solverZ3, action_op._op_id(), it_upd.update_index(), do_locs, true, 0);
                    const bool rlt = solverZ3->check_pop();

                    if (not rlt) { continue; }

                    /* Restrict. */
                    if (not has_ensemble) {
                        // violation_found = true;
                        auto solution_state = modelZ3->get_model_info().get_initial_values(); // TODO solution checker
                        modelZ3->get_state_vars(0).extract_solution(*solverZ3->get_solution(), solution_state, do_locs);
                        restrict(solution_state);
                        continue;
                    }
                }

                if (has_ensemble) {
                    solver->push();
                    modelVeritas->add_action_op(*solver, action_op._op_id(), it_upd.update_index(), do_locs, false, 0);
                 #ifdef ENABLE_APPLICABILITY_FILTERING
                    PLAJA_ASSERT(modelVeritas->get_interface())
                    if (modelVeritas->get_interface()->_do_applicability_filtering()) {
                        auto trees = modelVeritas->get_applicability_filter(action_label);
                        PLAJA_ASSERT(trees.num_leaf_values() > 0);
                        solver->set_app_filter_trees(trees);
                        solver->set_use_filter(true);
                    }
                 #endif
                    const bool rlt = solver->check(action_label);
                    solver->pop();

                    if (not rlt) { continue; }

                    /* Collect all solution boxes. */
                    // violation_found = true;
                    auto solution_boxes = solver->extract_boxes();
                    for (auto b : solution_boxes) {
                        auto num_inputs = modelZ3->get_model().get_number_variables();
                        std::vector<veritas::Interval> box1(b.begin(), b.begin() + num_inputs);
                        step_solutions.push_back(box1);
                        for (auto box: debug_solutions) {
                            PLAJA_ASSERT(not areBoxesEqual(box1, box))
                        }
                    }
                    
                }

            }

        }
    }
    
    if (has_ensemble) { solver->pop(); }
    solverZ3->pop();

    if (step_solutions.empty()) {
        std::cout << "Total boxes added are:" << count << std::endl;
        return SearchEngine::SOLVED;
    } else {
        // std::cout << "Found " << (step_solutions.size()) << " solutions for each action label" << std::endl;
        step_solutions = merge_recursive(step_solutions);
        // std::cout << "Merged to " << step_solutions.size() << " solutions in total" << std::endl;
        auto box = step_solutions.back();
        restrict(box);
        auto expr = box_to_condition(modelZ3->get_model(), box);
        nonInvariant = expr->deepCopy_Exp();
        step_solutions.pop_back();
        TO_NORMALFORM::normalize(nonInvariant);
        TO_NORMALFORM::specialize(nonInvariant);
        // nonInvariant->dump(true);
        return SearchEngine::IN_PROGRESS;
    }
}

SearchEngine::SearchStatus InvariantStrengthening::step_nn() {

    const bool has_nn = modelZ3->has_nn();
    PLAJA_ASSERT(not has_nn or (modelMarabou and solverMarabou))

    const bool do_locs = not modelZ3->ignore_locs();
    const auto& suc_gen = modelZ3->get_successor_generator();

    /*
     * Find a s not in S_inv
     * -- for each action op check whether there exists a transition leaving the invariant.
     */

    bool violation_found = false;

    solverZ3->push();
    if (has_nn) { solverMarabou->push(); }

    /* Add non-invariant. */
    modelZ3->add_to_solver(*solverZ3, *nonInvariant, 1);
    if (modelMarabou) { modelMarabou->add_to_solver(*solverMarabou, *nonInvariant, 1); }

    /* Iterate labels. */
    for (auto it_action = suc_gen.init_action_id_it(true); !it_action.end() and not violation_found; ++it_action) {
        const auto action_label = it_action.get_label();

        const bool is_learned = (has_nn and modelMarabou->get_interface()->is_learned(action_label));
        if (is_learned) {
            if (has_nn) {
                modelMarabou->add_output_interface(*solverMarabou, action_label, 0);
            }
        }

        /* Iterate ops. */
        for (auto it_op = suc_gen.init_action_it_static(action_label); !it_op.end() and not violation_found; ++it_op) {
            const auto& action_op = it_op.operator*();

            for (auto it_upd = action_op.updateIterator(); !it_upd.end() and not violation_found; ++it_upd) {

                /* Check non-policy transition. */
                {
                    solverZ3->push();
                    modelZ3->add_action_op(*solverZ3, action_op._op_id(), it_upd.update_index(), do_locs, true, 0);
                    const bool rlt = solverZ3->check_pop();

                    if (not rlt) { continue; }

                    /* Restrict. */
                    if (not has_nn) {
                        violation_found = true;
                        auto solution_state = modelZ3->get_model_info().get_initial_values(); // TODO solution checker
                        modelZ3->get_state_vars(0).extract_solution(*solverZ3->get_solution(), solution_state, do_locs);
                        restrict(solution_state);
                        continue;
                    }
                }


                if (has_nn) {
                    solverMarabou->push();
                    modelMarabou->add_action_op(*solverMarabou, action_op._op_id(), it_upd.update_index(), do_locs, true, 0);
                    const bool rlt = solverMarabou->check();
                    solverMarabou->pop();

                    if (not rlt) { continue; }

                    /* Restrict. */
                    violation_found = true;
                    auto solution_state = modelMarabou->get_model_info().get_initial_values();
                    modelMarabou->get_state_indexes(0).extract_solution(solverMarabou->extract_solution(), solution_state, do_locs);
                    restrict(solution_state);
                }
            }

        }

    }

    if (has_nn) { solverMarabou->pop(); }
    solverZ3->pop();

    if (violation_found) {
        return SearchEngine::IN_PROGRESS;
    } else {
        return SearchEngine::SOLVED;
    }
}

/* Override stats. */

void InvariantStrengthening::print_statistics() const { searchStatistics->print_statistics(); }

void InvariantStrengthening::stats_to_csv(std::ofstream& file) const { searchStatistics->stats_to_csv(file); }

void InvariantStrengthening::stat_names_to_csv(std::ofstream& file) const { searchStatistics->stat_names_to_csv(file); }
