//
// This file is part of the PlaJA code base.
// Copyright (c) (2019 - 2022) Marcel Vinzent.
// See README.md in the top-level directory for licensing information.
//

#include "path_existence_checker.h"
#include "../../../parser/ast/expression/non_standard/predicates_expression.h"
#include "../../../parser/visitor/extern/to_normalform.h"
#include "../../factories/configuration.h"
#include "../../factories/predicate_abstraction/pa_options.h"
#include "../../information/jani2nnet/jani_2_nnet.h"
#include "../../information/model_information.h"
#include "../../states/state_values.h"
#include "../../successor_generation/action_op.h"
#include "../../smt/solver/smt_solver_z3.h"
#include "../../smt/solver/solution_z3.h"
#include "../../smt/successor_generation/action_op_to_z3.h"
#include "../../smt/utils/to_z3_expr_splits.h"
#include "../../smt/visitor/extern/to_z3_visitor.h"
#include "../../smt/vars_in_z3.h"
#include "../../smt/constraint_z3.h"
#include "../../smt_nn/solver/smt_solver_marabou.h"
#include "../pa_states/abstract_path.h"
#include "../pa_states/abstract_state.h"
#include "../nn/smt/model_marabou_pa.h"
#include "../solution_checker_instance_pa.h"
#include "../smt/model_z3_pa.h"
#include "spuriousness_result.h"
#include "selection_refinement.h"


#ifdef USE_VERITAS
    #include "../ensemble/smt/model_veritas_pa.h"
    #include "../../smt_ensemble/solver/solver_veritas.h"
    #include "../../information/jani2ensemble/jani_2_ensemble.h"
#endif

PathExistenceChecker::PathExistenceChecker(const PLAJA::Configuration& configuration, const PredicatesStructure& predicates):
    predicates(&predicates)
    , solutionChecker(nullptr)
    , modelZ3(nullptr)
    , solver(nullptr)
    , modelMarabou(nullptr)
    , solverMarabou(nullptr)
CONSTRUCT_IF_TERMINAL_STATE_SUPPORT(paStateIsTerminalExact(configuration.get_bool_option(PLAJA_OPTION::terminal_as_preds))) {

    if (configuration.has_sharable(PLAJA::SharableKey::MODEL_Z3)) {

        modelZ3 = configuration.get_sharable_cast<ModelZ3, ModelZ3PA>(PLAJA::SharableKey::MODEL_Z3);

    } else {

        modelZ3 = std::make_shared<ModelZ3PA>(configuration);

        configuration.set_sharable<ModelZ3>(PLAJA::SharableKey::MODEL_Z3, modelZ3);
    }

    solver = std::make_unique<Z3_IN_PLAJA::SMTSolver>(modelZ3->share_context(), configuration.get_sharable_ptr<PLAJA::StatsBase>(PLAJA::SharableKey::STATS));

    if (modelZ3->has_nn()) {

        if (configuration.has_sharable(PLAJA::SharableKey::MODEL_MARABOU)) {

            modelMarabou = configuration.get_sharable_cast<ModelMarabou, ModelMarabouPA>(PLAJA::SharableKey::MODEL_MARABOU);
            modelSmt = configuration.get_sharable_cast<ModelMarabou, ModelMarabouPA>(PLAJA::SharableKey::MODEL_MARABOU);

        } else {

            modelMarabou = std::make_shared<ModelMarabouPA>(configuration);
            auto marabouModel = std::make_shared<ModelMarabouPA>(configuration);
            configuration.set_sharable<ModelMarabou>(PLAJA::SharableKey::MODEL_MARABOU, marabouModel); // cannot use modelSmt for this just yet. Annoying
            modelSmt = configuration.get_sharable_cast<ModelMarabou, ModelMarabouPA>(PLAJA::SharableKey::MODEL_MARABOU);

        }

        solverMarabou = MARABOU_IN_PLAJA::SMT_SOLVER::construct(configuration, modelMarabou->make_query(false, false, 0));

        solutionChecker = std::make_unique<SolutionCheckerPa>(configuration, modelMarabou->get_model(), modelMarabou->get_interface());

    } else {
        #ifdef USE_VERITAS
        PLAJA_ASSERT(modelZ3->has_ensemble())
        if (not configuration.has_sharable(PLAJA::SharableKey::MODEL_VERITAS)) {
            auto veritasModel = std::make_shared<ModelVeritasPA>(configuration);
            configuration.set_sharable<ModelVeritas>(PLAJA::SharableKey::MODEL_VERITAS, veritasModel);
        }
        
        modelSmt = configuration.get_sharable_cast<ModelVeritas, ModelVeritasPA>(PLAJA::SharableKey::MODEL_VERITAS);
        auto modelVeritas = configuration.get_sharable_cast<ModelVeritas, ModelVeritasPA>(PLAJA::SharableKey::MODEL_VERITAS);
        solverSmt = VERITAS_IN_PLAJA::ENSEMBLE_SOLVER::construct(configuration, *modelVeritas->make_query(false, false, 0));

        solutionChecker = std::make_unique<SolutionCheckerPa>(configuration, modelVeritas->get_model(), modelVeritas->get_interface());
        #endif
    }

}

PathExistenceChecker::~PathExistenceChecker() = default;

//

void PathExistenceChecker::set_interface(const class Jani2Interface* interface) { modelZ3->set_interface(interface); }

const Expression* PathExistenceChecker::get_start() const { return modelZ3->get_start(); }

const Expression* PathExistenceChecker::get_reach() const { return modelZ3->get_reach(); }

const class Jani2Interface* PathExistenceChecker::get_interface() const { return modelZ3->get_interface(); }

bool PathExistenceChecker::has_interface() const { return modelZ3->has_interface(); }

bool PathExistenceChecker::has_nn() const { return modelZ3->has_nn(); }

#ifdef USE_VERITAS
bool PathExistenceChecker::has_ensemble() const { return modelZ3->has_ensemble(); }
#endif

const Expression* PathExistenceChecker::get_non_terminal() const { return modelZ3->get_non_terminal(); }

Z3_IN_PLAJA::Context& PathExistenceChecker::get_context() const { return modelZ3->get_context(); }

inline bool PathExistenceChecker::do_locs() const { return not modelZ3->ignore_locs(); }

/**********************************************************************************************************************/

bool PathExistenceChecker::check_entailed_non_terminal(Z3_IN_PLAJA::SMTSolver& solver_ref, StepIndex_type step) const {
    PLAJA_ABORT_IF(not PLAJA_GLOBAL::debug)
    if constexpr (not PLAJA_GLOBAL::enableTerminalStateSupport) { return true; }
    if (not modelZ3->get_terminal()) { return true; }
    solver_ref.push();
    modelZ3->add_terminal(solver_ref, step);
    return not solver_ref.check_pop();
}

inline void PathExistenceChecker::encode_start(Z3_IN_PLAJA::SMTSolver& query, const SolutionCheckerInstancePaPath& problem) const {

    if (problem.get_start()) {
        PLAJA_ASSERT(modelZ3->get_start() == problem.get_start())
        modelZ3->add_start(query, problem.includes_init());
    } else if (problem.includes_init()) { modelZ3->add_init(query, 0); }

}

void PathExistenceChecker::encode_path(Z3_IN_PLAJA::SMTSolver& query, const SolutionCheckerInstancePaPath& problem) const {

    const auto& path = problem.get_path();

    PLAJA_ASSERT(path.get_state_path_size() >= 2)

    modelZ3->generate_steps(path.get_size());

    modelZ3->add_bounds(query, path.get_start_step(), not do_locs());
    encode_start(query, problem);

    for (auto path_it = path.init_path_iterator(); !path_it.end(); ++path_it) {

        /* Pa state. */
        if (problem.is_pa_state_aware() or path_it.get_step() == 0) { modelZ3->add_to(path_it.get_pa_state(), query, path_it.get_step()); }

        /* Terminal. */
        if (check_terminal(problem.is_pa_state_aware())) { modelZ3->exclude_terminal(query, path_it.get_step()); }
        else { PLAJA_ASSERT(check_entailed_non_terminal(query, path_it.get_step())) }

        /* Vars. */
        modelZ3->add_bounds(query, path_it.get_step() + 1, not do_locs());

        /* Op. */
        modelZ3->add_action_op(query, path_it.get_op_id(), path_it.get_update_index(), do_locs(), true, path_it.get_step());

    }

    /* Pa state. */
    if (problem.is_pa_state_aware()) { modelZ3->add_to(path.get_pa_target_state(), query, path.get_target_step()); }

    /* Terminal. */
    if (check_terminal(problem.is_pa_state_aware()) and not PLAJA_GLOBAL::reachMayBeTerminal) { modelZ3->exclude_terminal(query, path.get_target_step()); }
    else { PLAJA_ASSERT(check_terminal(problem.is_pa_state_aware()) or check_entailed_non_terminal(query, path.get_target_step())) }

    /* Reach */
    if (problem.get_reach()) {
        PLAJA_ASSERT(modelZ3->get_reach() == problem.get_reach())
        modelZ3->add_reach(query, path.get_target_step());
    }

    /* Terminal sanity. */
    if (PLAJA_GLOBAL::debug and PLAJA_GLOBAL::enableTerminalStateSupport and PLAJA_GLOBAL::reachMayBeTerminal and modelZ3->get_terminal()) {
        query.push();
        modelZ3->add_terminal(query, path.get_target_step());
        PLAJA_ASSERT_EXPECT(not query.check_pop())
    }
}

void PathExistenceChecker::encode_path(MARABOU_IN_PLAJA::SMTSolver& query, const SolutionCheckerInstancePaPath& problem) const {
    const auto& path = problem.get_path();

    PLAJA_ASSERT(path.get_state_path_size() >= 1)

    // modelMarabou->generate_steps(path.size()); // responsibility of callee as query must have been initialized apropriately

    PLAJA_ASSERT(not(problem.get_start() and problem.includes_init()))

    if (problem.get_start()) {

        // optimize start
        // if (optimize_start) {
        Z3_IN_PLAJA::SMTSolver solver_tmp(modelZ3->share_context(), solver->share_statistics());
        encode_path(solver_tmp, problem);
        modelMarabou->add_to_query(query, *modelZ3->optimize_expression(*modelMarabou->get_start(), solver_tmp, false, false, false, false), path.get_start_step());
        // }
        // else { modelMarabou->add_start(query); }

    } else if (problem.includes_init()) { modelMarabou->add_init(query, path.get_start_step()); }

    for (auto path_it = path.init_path_iterator(); !path_it.end(); ++path_it) {

        if (problem.is_pa_state_aware() or path_it.get_step() == path.get_start_step()) { modelMarabou->add_to(path_it.get_pa_state(), query, path_it.get_step()); }

        modelMarabou->add_action_op(query, path_it.get_op_id(), path_it.get_update_index(), do_locs(), true, path_it.get_step());

        /* Terminal. */ // TODO optimize
        if (check_terminal(problem.is_pa_state_aware())) { modelMarabou->exclude_terminal(query, path_it.get_step()); }

    }

    /* Pa state. */
    if (problem.is_pa_state_aware()) { modelMarabou->add_to(path.get_pa_target_state(), query, path.get_target_step()); }

    /* Terminal. */
    if (check_terminal(problem.is_pa_state_aware()) and not PLAJA_GLOBAL::reachMayBeTerminal) { modelMarabou->exclude_terminal(query, path.get_target_step()); }

    /* Reach */
    if (problem.get_reach()) {
        PLAJA_ASSERT(modelZ3->get_reach() == problem.get_reach()) // modelMarabou does not work, since here start is rewritten.
        modelMarabou->add_reach(query, path.get_target_step());
    }

}

/**********************************************************************************************************************/

bool PathExistenceChecker::check_start(const AbstractState& abstract_state, bool include_model_init) {
    modelZ3->generate_steps(1);
    auto& solver_ = *solver;
    solver_.clear();
    PLAJA_ASSERT(solver_.get_stack_size() == 1)
    solver_.push(); // to suppress scoped_timer thread in z3, which is started when using tactic2solver
    modelZ3->add_bounds(solver_, 0, not do_locs());
    modelZ3->add_start(solver_, include_model_init);
    modelZ3->add_to(abstract_state, solver_, 0);
    return solver_.check();
}

bool PathExistenceChecker::check(const SolutionCheckerInstancePaPath& problem) {
    PLAJA_ASSERT(not do_locs()) // not properly supported

    const auto& path = problem.get_path();
    PLAJA_ASSERT(path.get_state_path_size() >= 2) // Expect actual path.
    /* TODO Might enable checking of path prefix only, but for now we assert: */
    PLAJA_ASSERT(problem.get_target_step() == path.get_size()) // StepIndex is state path length - 1.

    modelZ3->generate_steps(path.get_size());

    auto& solver_ = *solver;
    solver_.clear();
    PLAJA_ASSERT(solver_.get_stack_size() == 1)
    solver_.push(); // To suppress scoped_timer thread in z3, which is started when using tactic2solver.

    /* Start vars. */
    modelZ3->add_bounds(solver_, path.get_start_step(), not do_locs());

    /* Start. */
    encode_start(solver_, problem);

    for (auto path_it = path.init_path_iterator(); !path_it.end(); ++path_it) {

        /* Predicates (optional) */
        if (problem.is_pa_state_aware() or path_it.get_step() == 0) { // Must always assert abstract start state.
            PLAJA_ASSERT(path_it.get_step() > 0 or solver_.check())
            modelZ3->add_to(path_it.get_pa_state(), solver_, path_it.get_step());
        }

        /* Terminal. */
        if (check_terminal(problem.is_pa_state_aware())) { modelZ3->exclude_terminal(solver_, path_it.get_step()); }
        else { PLAJA_ASSERT(check_entailed_non_terminal(solver_, path_it.get_step())) }

        /* NN select. */
        if (modelZ3->has_nn() and path_it.get_step() < problem.get_policy_target_step()) {
            modelZ3->add_nn(solver_, path_it.get_step());
            modelZ3->add_output_interface_for_op(solver_, path_it.get_op_id(), path_it.get_step());
        }

        #ifdef USE_VERITAS
        if (modelZ3->has_ensemble() and path_it.get_step() < problem.get_policy_target_step()) {
            PLAJA_LOG("add ensemble to z3?")
            PLAJA_ABORT
        }
        #endif

        /* Action op structure. */
        modelZ3->add_action_op(solver_, path_it.get_op_id(), path_it.get_update_index(), do_locs(), true, path_it.get_step());

        /* Target vars. */
        modelZ3->add_bounds(solver_, path_it.get_successor_step(), not do_locs());
    }

    /* Predicates. */
    if (problem.is_pa_state_aware()) {
        PLAJA_ASSERT(path.get_target_step() > 0)
        modelZ3->add_to(path.get_pa_target_state(), solver_, path.get_target_step());
    }

    /* Terminal. */
    if (check_terminal(problem.is_pa_state_aware()) and not PLAJA_GLOBAL::reachMayBeTerminal) { modelZ3->exclude_terminal(solver_, path.get_target_step()); }
    else { PLAJA_ASSERT(check_terminal(problem.is_pa_state_aware()) or check_entailed_non_terminal(solver_, path.get_target_step())) }

    /* Reach. */
    if (problem.get_reach()) {
        PLAJA_ASSERT(modelZ3->get_reach() == problem.get_reach())
        modelZ3->add_reach(solver_, path.get_target_step());
    }
    /* Terminal sanity. */
    if (PLAJA_GLOBAL::debug and PLAJA_GLOBAL::enableTerminalStateSupport and PLAJA_GLOBAL::reachMayBeTerminal and modelZ3->get_terminal()) {
        solver_.push();
        modelZ3->add_terminal(solver_, path.get_target_step());
        PLAJA_ASSERT_EXPECT(not solver_.check_pop())
    }

    return solver_.check();

}

/**********************************************************************************************************************/

void PathExistenceChecker::add_predicates_aux(SpuriousnessResult& result, StepIndex_type step, const AbstractState& abstract_state, Z3_IN_PLAJA::SMTSolver& solver_) const {

    if (result.do_exclude_entailment()) { // Add non-entailed predicate (values) only.

        for (auto pred_it = abstract_state.init_pred_state_iterator(); !pred_it.end(); ++pred_it) {
            PLAJA_ASSERT(PLAJA_GLOBAL::lazyPA or pred_it.is_set())
            if constexpr (PLAJA_GLOBAL::lazyPA) { if (not pred_it.is_set()) { continue; } }

            if (pred_it.predicate_value()) {

                if (solver_.check_push_pop(!modelZ3->get_predicate_z3(pred_it.predicate_index(), step))) {
                    result.add_refinement_predicate(pred_it.predicate_index(), step);
                }

            } else {

                if (solver_.check_push_pop(modelZ3->get_predicate_z3(pred_it.predicate_index(), step))) {
                    result.add_refinement_predicate(pred_it.predicate_index(), step);
                }

            }

        }

    } else {

        for (auto pred_it = abstract_state.init_pred_state_iterator(); !pred_it.end(); ++pred_it) {
            PLAJA_ASSERT(PLAJA_GLOBAL::lazyPA or pred_it.is_set())
            if constexpr (PLAJA_GLOBAL::lazyPA) { if (not pred_it.is_set()) { continue; } }
            result.add_refinement_predicate(pred_it.predicate_index(), step);
        }

    }
}

bool PathExistenceChecker::add_predicates(SpuriousnessResult& result, StepIndex_type step, const AbstractState& abstract_state, Z3_IN_PLAJA::SMTSolver& solver_, bool pa_state_aware) {
    PLAJA_ASSERT(not pa_state_aware or modelZ3->_predicates()->equals(abstract_state.get_predicates()))

    if (result.do_add_non_entailed_predicates_externally()) { add_predicates_aux(result, step, abstract_state, solver_); }

    if (pa_state_aware or step == 0) { // Must always assert abstract start state.

        const bool need_to_push = result.do_exclude_entailment() and not result.do_add_non_entailed_guards_externally();
        if (need_to_push) { solver_.push(); } // Must still check predicate entailment.

        modelZ3->add_to(abstract_state, solver_, step);

        if (not solver_.check()) {

            if (need_to_push) { solver_.pop(); } // Must still check predicate entailment.

            PLAJA_ASSERT(step > 0)
            result.set_spurious(SpuriousnessResult::Concretization);

            /* If not already done, we still need to add the (non-entailed) predicate (values). */
            if (not result.do_add_non_entailed_predicates_externally()) { add_predicates_aux(result, step, abstract_state, solver_); }

            return true;
        }

    }

    return false;
}

bool PathExistenceChecker::add_terminal(SpuriousnessResult& result, StepIndex_type step, Z3_IN_PLAJA::SMTSolver& solver_) {

    if (not PLAJA_GLOBAL::enableTerminalStateSupport or not solutionChecker->get_terminal()) { return true; }

    const auto& vars_smt = modelZ3->get_state_vars(step);

    // TODO: for now use guard config for terminal.
    if (result.do_add_non_entailed_guards_externally()) {
        auto terminal_splits = Z3_IN_PLAJA::to_z3_condition_splits(*modelZ3->get_non_terminal(), vars_smt);
        add_guards(result, step, *terminal_splits, solver_);
    }

    const bool need_to_push = result.do_exclude_entailment() and not result.do_add_non_entailed_guards_externally();
    if (need_to_push) { solver_.push(); } // Must still check guard entailment.

    modelZ3->exclude_terminal(solver_, step);

    if (not solver_.check()) { // Now check path existence.

        if (need_to_push) { solver_.pop(); } // Must still check guard entailment.

        result.set_spurious(SpuriousnessResult::Terminal);

        /* If not already done, we still need to add the (non-entailed) guards. */
        if (not result.do_add_non_entailed_guards_externally()) {
            auto terminal_splits = Z3_IN_PLAJA::to_z3_condition_splits(*modelZ3->get_non_terminal(), vars_smt);
            add_guards(result, step, *terminal_splits, solver_);
        }

        return false;

    }

    return true;
}

void PathExistenceChecker::add_guards(SpuriousnessResult& result, StepIndex_type step, ToZ3ExprSplits& splits, Z3_IN_PLAJA::SMTSolver& solver) {

    if (result.do_exclude_entailment()) {
        for (auto it = splits.iterator(); !it.end(); ++it) {
            if (solver.check_push_pop(!it.split_z3())) { result.add_refinement_predicate(it.move_split(), step); }
        }
    } else { for (auto it = splits.iterator(); !it.end(); ++it) { result.add_refinement_predicate(it.move_split(), step); } }

}

/**********************************************************************************************************************/

std::unique_ptr<SpuriousnessResult> PathExistenceChecker::check_incrementally(std::unique_ptr<SolutionCheckerInstancePaPath>&& problem) {
    PLAJA_ASSERT(not do_locs()) // not properly supported

    const auto& path = problem->get_path();
    PLAJA_ASSERT(path.get_state_path_size() >= 2) // Expect actual path.
    PLAJA_ASSERT(problem->get_target_step() == path.get_size()) // StepIndex is state path length - 1.
    PLAJA_ASSERT(problem->get_policy_target_step() == 0)

    modelZ3->generate_steps(path.get_size());

    auto& solver_ = *solver;
    solver_.clear();
    PLAJA_ASSERT(solver_.get_stack_size() == 1)
    solver_.push(); // To suppress scoped_timer thread in z3, which is started when using tactic2solver.

    /* Start vars. */
    modelZ3->add_bounds(solver_, path.get_start_step(), not do_locs());

    /* Start. */
    encode_start(solver_, *problem);

    const auto* problem_ptr = problem.get();
    std::unique_ptr<SpuriousnessResult> result(new SpuriousnessResult(*predicates, std::move(problem)));

    for (auto path_it = path.init_path_iterator(); !path_it.end(); ++path_it) {
        const auto& src_vars = modelZ3->get_state_vars(path_it.get_step());
        const auto& action_op = modelZ3->get_action_op_structure(path_it.get_op_id());
        const UpdateIndex_type update_index = path_it.get_update_index();

        PLAJA_ASSERT(solver_.check())

        /* Abstract state. */
        if (add_predicates(*result, path_it.get_step(), path_it.get_pa_state(), solver_, problem_ptr->is_pa_state_aware())) { return result; }

        /* Terminal. */
        if (check_terminal(problem_ptr->is_pa_state_aware()) and not add_terminal(*result, path_it.get_step(), solver_)) { return result; }
        else { PLAJA_ASSERT(check_terminal(problem_ptr->is_pa_state_aware()) or check_entailed_non_terminal(solver_, path_it.get_step())) }
        /* Guard. */

        /* TODO we split the guards down to atomic linear constraints and add these as predicates.
         * However, when excluding entailment, we might apply this on the disjunction level,
         * i.e., only adding atomic linear constraints for non entailed disjunctions.
         * In principle same holds true for reach/terminal condition. */
        if (result->do_add_non_entailed_guards_externally()) { // We add the non-entailed guards either way.
            auto guard_splits = action_op.guard_to_z3_splits(src_vars);
            add_guards(*result, path_it.get_step(), *guard_splits, solver_);
        }

        const bool need_to_push = result->do_exclude_entailment() and not result->do_add_non_entailed_guards_externally();
        if (need_to_push) { solver_.push(); } // Must still check guard entailment.

        action_op.guard_to_z3(src_vars, do_locs())->add_to_solver(solver_);

        if (not solver_.check()) { // Now check path existence.

            if (need_to_push) { solver_.pop(); } // Must still check guard entailment.

            result->set_spurious(SpuriousnessResult::GuardApp);

            /* If not already done, we still need to add the (non-entailed) guards. */
            if (not result->do_add_non_entailed_guards_externally()) {
                auto guard_splits = action_op.guard_to_z3_splits(src_vars);
                add_guards(*result, path_it.get_step(), *guard_splits, solver_);
            }

            return result;

        }

        /* */

        /* Target vars. */
        modelZ3->add_bounds(solver_, path_it.get_successor_step(), not do_locs());

        /* Update. */
        action_op.update_to_z3(update_index, src_vars, modelZ3->get_state_vars(path_it.get_successor_step()), do_locs(), true)->add_to_solver(solver_);
        PLAJA_ASSERT(solver_.check())

        /* Set prefix. */
        result->set_prefix_length(path_it.get_successor_step());
    }

    /* Abstract state. */
    if (add_predicates(*result, path.get_target_step(), path.get_pa_target_state(), solver_, problem_ptr->is_pa_state_aware())) { return result; }

    /* Terminal. */
    if (check_terminal(problem_ptr->is_pa_state_aware()) and not PLAJA_GLOBAL::reachMayBeTerminal) { if (not add_terminal(*result, path.get_target_step(), solver_)) { return result; } }
    else { PLAJA_ASSERT(check_terminal(problem_ptr->is_pa_state_aware()) or check_entailed_non_terminal(solver_, path.get_target_step())) }

    /* Finally must check target condition.*/

    if (result->do_exclude_entailment()) { solver_.push(); } // Must still check target entailment.

    // solver_.add(target_splits->_splits_in_z3()); This does not work for disjunctions.
    modelZ3->add_reach(solver_, path.get_target_step());

    /* Terminal sanity. */
    if (PLAJA_GLOBAL::debug and PLAJA_GLOBAL::enableTerminalStateSupport and PLAJA_GLOBAL::reachMayBeTerminal and modelZ3->get_terminal()) {
        solver_.push();
        modelZ3->add_terminal(solver_, path.get_target_step());
        PLAJA_ASSERT_EXPECT(not solver_.check_pop())
    }

    if (not solver_.check()) {

        if (result->do_exclude_entailment()) { solver_.pop(); } // Must still check target entailment.

        result->set_spurious(SpuriousnessResult::Target);

        const auto& target_vars = modelZ3->get_state_vars(path.get_target_step());
        auto target_splits = Z3_IN_PLAJA::to_z3_condition_splits(*modelZ3->get_reach(), target_vars);
        PathExistenceChecker::add_guards(*result, path.get_target_step(), *target_splits, solver_);

        return result;

    }

    /* Extract solution. */
    result->set_concretization(solver->extract_solution_via_checker(*modelZ3, result->get_path_instance().copy_problem())->retrieve_solution_vector());
    PLAJA_ASSERT(not result->get_path_instance().is_invalidated())

    return result;

}

bool PathExistenceChecker::check_policy(const SolutionCheckerInstancePaPath& problem) {
    PLAJA_ASSERT(not do_locs()) // Probably not properly supported.
    PLAJA_ASSERT(modelZ3->has_nn())

    const auto& path = problem.get_path();
    PLAJA_ASSERT(problem.get_target_step() == path.get_size()) // StepIndex is state path length - 1
    PLAJA_ASSERT(problem.get_policy_target_step() == problem.get_target_step())

    modelMarabou->generate_steps(path.get_state_path_size());
    auto& solver_marabou = *solverMarabou;
    solver_marabou.set_query(modelMarabou->make_query(false, true, path.get_size() - 1));
    PLAJA_ASSERT(path.get_state_path_size() >= 1)

    // TODO maybe swap order (NN, path without NN)

    /* Path without NN. */
    encode_path(solver_marabou, problem);

    /* NN. */
    for (auto path_it = path.init_path_iterator(); !path_it.end(); ++path_it) {
        modelMarabou->add_nn_to_query(solver_marabou._query(), path_it.get_step());
        modelMarabou->add_output_interface_for_op(solver_marabou._query(), path_it.get_op_id(), path_it.get_step());
    }

    solver_marabou.set_solution_checker(*modelMarabou, problem.copy_problem());
    const bool rlt = solver_marabou.check();
    solver_marabou.unset_solution_checker();
    return rlt;
}

std::unique_ptr<SpuriousnessResult> PathExistenceChecker::check_policy_incrementally(std::unique_ptr<SolutionCheckerInstancePaPath>&& problem, const SelectionRefinement& selection_refinement) {
    PLAJA_ASSERT(not do_locs()) // not properly supported
    PLAJA_ASSERT(modelZ3->has_nn())

    const auto& path = problem->get_path();
    PLAJA_ASSERT(problem->get_target_step() == path.get_size()) // StepIndex is state path length - 1
    PLAJA_ASSERT(problem->get_policy_target_step() == problem->get_target_step())

    modelMarabou->generate_steps(path.get_state_path_size());
    auto& solver_marabou = *solverMarabou;
    solver_marabou.set_query(modelMarabou->make_query(false, true, path.get_size() - 1));
    PLAJA_ASSERT(path.get_state_path_size() >= 1)

    /*
     * We expect that PathExistenceChecker's z3 solver has been successfully called on the non-policy-restricted path problem
     * and problem comes with a matching concretization.
     */
    auto solution_checker_instance = problem->copy_problem();
    solution_checker_instance->set_policy_target(path.get_start_step());
    PLAJA_ASSERT(not problem->is_invalidated())
    solution_checker_instance->set_solution(problem->retrieve_solution_vector());
    PLAJA_ASSERT(solution_checker_instance->check())
    bool current_instance_solution_is_good = true;

    encode_path(solver_marabou, *problem);

    if constexpr (PLAJA_GLOBAL::debug) {
        solver_marabou.set_solution_checker(*modelMarabou, solution_checker_instance->copy_problem());
        PLAJA_ASSERT(solver_marabou.check())
        auto sanity_instance = solver_marabou.extract_solution_via_checker();
        PLAJA_ASSERT(not sanity_instance or sanity_instance->check())
    }

    const auto* problem_ptr = problem.get();
    std::unique_ptr<SpuriousnessResult> result(new SpuriousnessResult(*predicates, std::move(problem)));

    for (auto path_it = path.init_path_iterator(); !path_it.end(); ++path_it) {

        PLAJA_LOG_DEBUG(PLAJA_UTILS::string_f("Step %i", path_it.get_step()))

        /* Add nn structures for current step. */
        modelMarabou->add_nn_to_query(solver_marabou._query(), path_it.get_step());
        modelMarabou->add_output_interface_for_op(solver_marabou._query(), path_it.get_op_id(), path_it.get_step());

        /* Try to reuse previous solution. */
        if (current_instance_solution_is_good) {
            solution_checker_instance->inc_policy_target();

            if (solution_checker_instance->check()) {
                result->set_prefix_length(path_it.get_successor_step()); // Set prefix.
                continue;
            } else {
                current_instance_solution_is_good = false; // Keep as backup for concretization.
            }

        }

        /* Check (with fresh instance). */
        solver_marabou.set_solution_checker(*modelMarabou, std::make_unique<SolutionCheckerInstancePaPath>(*solutionChecker, path, path.get_target_step(), path_it.get_successor_step(), problem_ptr->is_pa_state_aware(), problem_ptr->get_start(), problem_ptr->includes_init(), problem_ptr->get_reach()));
        const bool rlt = solver_marabou.check();

        if (rlt) {
            PLAJA_ASSERT(not current_instance_solution_is_good)

            /* If check found a valid solution, we cache the fresh solution instance. */
            auto fresh_instance = PLAJA_UTILS::cast_unique<SolutionCheckerInstancePaPath>(solver_marabou.extract_solution_via_checker());
            if (fresh_instance) {
                PLAJA_ASSERT(not fresh_instance->is_invalidated())
                PLAJA_ASSERT(fresh_instance->check())
                solution_checker_instance = std::move(fresh_instance);
                current_instance_solution_is_good = true;
            }

            result->set_prefix_length(path_it.get_successor_step()); // Set prefix.

        } else {
            PLAJA_ASSERT(not current_instance_solution_is_good)

            result->set_spurious(SpuriousnessResult::ActionSel);

            PLAJA_ASSERT(not solution_checker_instance->is_invalidated())
            PLAJA_ASSERT(not get_interface()->is_chosen(*solution_checker_instance->get_solution(path_it.get_step()), path_it.retrieve_label()))
            selection_refinement.add_refinement_preds(*result, path_it.retrieve_label(), path_it.get_witness().get(), solution_checker_instance->get_solution(path_it.get_step()));

            solver_marabou.unset_solution_checker();
            return result;

        }

    }

    /* Cache concretization. */
    if (current_instance_solution_is_good) {
        PLAJA_ASSERT(solution_checker_instance->get_solution_size() == path.get_state_path_size())
        result->set_concretization(solution_checker_instance->retrieve_solution_vector());
    }

    return result;

}

/* */

bool PathExistenceChecker::check(unsigned int number_of_steps, bool nn_aware) {
    modelZ3->generate_steps(number_of_steps);

    auto& solver_ = *solver;
    solver_.clear();
    PLAJA_ASSERT(solver_.get_stack_size() == 1)
    solver_.push(); // To suppress scoped_timer thread in z3, which is started when using tactic2solver.

    modelZ3->add_bounds(solver_, 0, false);
    modelZ3->add_start(solver_, true);

    for (StepIndex_type step_index = 0; step_index < number_of_steps; ++step_index) {
        /* Forwarding. */
        if (nn_aware) { modelZ3->add_nn(solver_, step_index); }
        /* Terminal. */
        if (check_terminal(false)) { modelZ3->exclude_terminal(solver_, step_index); }
        /* Choice point. */
        modelZ3->add_choice_point(solver_, step_index, nn_aware);
        /* Successor bounds. */
        modelZ3->add_bounds(solver_, step_index + 1, false);
    }

    /* Terminal. */
    if (check_terminal(false) and not PLAJA_GLOBAL::reachMayBeTerminal) { modelZ3->exclude_terminal(solver_, number_of_steps); }

    /* Target condition. */
    if (modelZ3->get_reach()) { modelZ3->add_reach(solver_, number_of_steps); }

    /* Terminal sanity. */
    if (PLAJA_GLOBAL::debug and PLAJA_GLOBAL::enableTerminalStateSupport and PLAJA_GLOBAL::reachMayBeTerminal and modelZ3->get_terminal()) {
        solver_.push();
        modelZ3->add_terminal(solver_, number_of_steps);
        PLAJA_ASSERT_EXPECT(not solver_.check_pop())
    }

    return solver_.check();

}

/* */

void PathExistenceChecker::dump_solution(std::unique_ptr<SolutionCheckerInstance>&& solution_checker_instance) const {

    solution_checker_instance = solver->extract_solution_via_checker(*modelZ3, std::move(solution_checker_instance));

    for (StepIndex_type step = 0; step < solution_checker_instance->get_solution_size(); ++step) {
        PLAJA_LOG(PLAJA_UTILS::string_f("--- Step %i ---", step))
        solution_checker_instance->get_solution(step)->dump(&modelZ3->get_model());
    }

    // Deprecated:
    // const auto solution = solver->get_solution();
    // modelZ3->get_state_vars(0).extract_solution(*solution, state, do_locs());
    // solver->add(modelZ3->get_state_vars(0).encode_solution(*solution, do_locs()));

#if 0
    // nn vars:
    if (include_nn_vars) {
        PLAJA_LOG("NN output variables:")
        for (auto it = modelZ3->iterate_path(); !it.end(); ++it) {
            const auto& nn_vars = modelZ3->get_nn_vars(it.step());
            PLAJA_LOG(PLAJA_UTILS::string_f("--- Step %i ---", it.step()))
            for (const auto output_var: nn_vars._output_features()) {
                PLAJA_LOG((nn_vars[output_var] == PLAJA::SMTSolver::eval(nn_vars[output_var], m)).to_string())
            }
        }
    }
#endif

}
