//
// This file is part of the PlaJA code base.
// Copyright (c) (2019 - 2022) Marcel Vinzent.
// See README.md in the top-level directory for licensing information.
//

#include "expansion_pa_base.h"
#include "../../../option_parser/enum_option_values_set.h"
#include "../../../parser/ast/expression/non_standard/predicate_abstraction_expression.h"
#include "../../../globals.h"
#include "../../factories/predicate_abstraction/search_engine_config_pa.h"
#include "../../information/jani2nnet/jani_2_nnet.h"
#ifdef USE_VERITAS
#include "../../information/jani2ensemble/jani_2_ensemble.h"
#include "../ensemble/ensemble_sat_checker/ensemble_sat_checker.h"
#endif
#include "../sat_checker.h"
#include "../../fd_adaptions/search_statistics.h"
#include "../../smt/solver/smt_solver_z3.h"
#include "../../smt/successor_generation/op_applicability.h"
#include "../../successor_generation/action_op.h"
#include "../../successor_generation/successor_generator_c.h"
#include "../match_tree/predicate_traverser.h"
#include "../nn/nn_sat_checker/nn_sat_checker.h"
#include "../inc_state_space/incremental_state_space.h"
#include "../optimization/pred_entailment_cache_interface.h"
#include "../optimization/predicate_relations.h"
#include "../pa_states/predicate_state.h"
#include "../search_space/search_space_pa_base.h"
#include "../smt/model_z3_pa.h"
#include "../successor_generation/action_op/action_op_pa.h"
#include "../successor_generation/update/update_pa.h"
#include "../successor_generation/successor_generator_pa.h"
#include "../solution_checker_instance_pa.h"
#include "expansion_cache_pa.h"


namespace PLAJA_OPTION {

    std::unique_ptr<PLAJA_OPTION::EnumOptionValuesSet> construct_pa_state_reachability_enum() {

#ifdef PA_ONLY_REACHABILITY
        PLAJA_ABORT
#else

        return std::make_unique<PLAJA_OPTION::EnumOptionValuesSet>(
            std::list<PLAJA_OPTION::EnumOptionValue> {
                PLAJA_OPTION::EnumOptionValue::PaReachabilityGlobal,
                PLAJA_OPTION::EnumOptionValue::PaOnlyReachabilityPerSrc,
                PLAJA_OPTION::EnumOptionValue::PaOnlyReachabilityPerLabel,
                PLAJA_OPTION::EnumOptionValue::PaOnlyReachabilityPerOp,
                PLAJA_OPTION::EnumOptionValue::None,
            },
            PLAJA_OPTION::EnumOptionValue::PaReachabilityGlobal
        );

#endif

    }

}

/**********************************************************************************************************************/
ExpansionPABase::ExpansionPABase(const SearchEngineConfigPA& config, bool goal_path_search, bool optimal_search, SearchSpacePABase& search_space, const ModelZ3PA& model_z3, bool witness_interval):
    sharedStats(config.get_sharable_ptr<PLAJA::StatsBase>(PLAJA::SharableKey::STATS))
    , searchSpace(&search_space)
    , solutionChecker(nullptr)
    , modelZ3(&model_z3)
    , solverOwn(new Z3_IN_PLAJA::SMTSolver(modelZ3->share_context(), sharedStats))
    , solver(solverOwn.get()) // solver((z3::tactic(modelZ3->get_context(), "solve-eqs") & z3::tactic(modelZ3->get_context(), "smt")).mk_solver(), *modelZ3),
    , reachChecker(new StateConditionCheckerPa(config, *modelZ3, not PLAJA_GLOBAL::enableTerminalStateSupport or PLAJA_GLOBAL::reachMayBeTerminal or not modelZ3->get_terminal() ? modelZ3->get_reach() : modelZ3->get_reach_non_terminal()))
    , terminalChecker(PLAJA_GLOBAL::enableTerminalStateSupport and config.get_bool_option(PLAJA_OPTION::check_for_pa_terminal_states) and modelZ3->get_terminal() ? new StateConditionCheckerPa(config, *modelZ3, modelZ3->get_terminal()) : nullptr)
    , predicateRelations(nullptr)
    , successorGenerator(nullptr)
    , opApp(PLAJA_GLOBAL::enableApplicabilityCache ? OpApplicability::construct(config) : nullptr)
    , paStateReachability(PLAJA_GLOBAL::paOnlyReachability or goal_path_search ? nullptr : config.get_enum_option(PLAJA_OPTION::pa_state_reachability).copy_value())
    , goalPathSearch(goal_path_search)
    , optimalSearch(optimal_search)
    , incOpAppTest(config.get_bool_option(PLAJA_OPTION::inc_op_app_test) and not(PLAJA_GLOBAL::enableApplicabilityFiltering and opApp))
    , lazyNonTerminalPush(PLAJA_GLOBAL::enableTerminalStateSupport and not PLAJA_GLOBAL::doSelectionTest and config.get_bool_option(PLAJA_OPTION::lazyNonTerminalPush))
    , lazyInterfacePush(PLAJA_GLOBAL::enableApplicabilityFiltering and not PLAJA_GLOBAL::doSelectionTest and config.get_bool_option(PLAJA_OPTION::lazyPolicyInterfacePush))
    , mayBeTerminal(true)
    , intervalRefinement(witness_interval)
    CONSTRUCT_IF_DEBUG(expectExactTerminalCheck(PLAJA_GLOBAL::enableTerminalStateSupport and config.has_bool_option(PLAJA_OPTION::terminal_as_preds) and config.get_bool_option(PLAJA_OPTION::terminal_as_preds)))
    , cache(nullptr) {
    PLAJA_ASSERT(sharedStats)
    const bool do_app_filtering = modelZ3->get_interface() and modelZ3->get_interface()->_do_applicability_filtering();
    PLAJA_LOG_IF(not PLAJA_GLOBAL::enableApplicabilityFiltering and do_app_filtering, "Error: applicability filtering currently not supported!")
    PLAJA_ABORT_IF(do_app_filtering and not PLAJA_GLOBAL::enableApplicabilityFiltering)

    PLAJA_LOG_IF(not do_app_filtering and opApp, PLAJA_UTILS::to_red_string(PLAJA_UTILS::string_f("Using operator applicability cache for policy verification without applicability-filtering.")))

    // Add src variable (bound)s in predicates.
    const auto start = modelZ3->get_start();
    if (start) { modelZ3->register_src_bounds(*solver, *start); }
    const auto reach = modelZ3->get_reach();
    if (reach) { modelZ3->register_src_bounds(*solver, *reach); }
    for (PredicateIndex_type index = 0; index < modelZ3->get_number_predicates(); ++index) { modelZ3->register_src_bounds(*solver, *modelZ3->get_predicate(index)); }
    // solver->add_and_register_variables(modelZ3->get_src_vars());

    // Construct successor generation (predicate dependent).
    successorGenerator = SuccessorGeneratorPA::construct(*modelZ3, config);
    PLAJA_ASSERT(config.has_sharable(PLAJA::SharableKey::MODEL_Z3))
    PLAJA_ASSERT(config.get_sharable<ModelZ3>(PLAJA::SharableKey::MODEL_Z3).get() == modelZ3)
    config.get_sharable<ModelZ3>(PLAJA::SharableKey::MODEL_Z3)->share_action_ops(*successorGenerator); // TODO quick fix: access instance as shared object

    // Share op applicability cache:
    if constexpr (PLAJA_GLOBAL::enableApplicabilityCache) { if (opApp) { modelZ3->share_op_applicability(opApp); } }

    // Optimizations:
    if (config.get_bool_option(PLAJA_OPTION::predicate_relations)) {

        if (config.has_sharable(PLAJA::SharableKey::PREDICATE_RELATIONS)) {
            predicateRelations = config.get_sharable<PredicateRelations>(PLAJA::SharableKey::PREDICATE_RELATIONS);
        } else {
            predicateRelations = config.set_sharable(PLAJA::SharableKey::PREDICATE_RELATIONS, std::make_shared<PredicateRelations>(config, *modelZ3, nullptr)); // predicates relation // OLD_INC: solver
        }

        PLAJA_ASSERT(predicateRelations->has_empty_ground_truth())

    }

    if (modelZ3->has_nn()) {
        // Makes sense to have only nn interface here
        auto interface = modelZ3->get_interface();
        auto nnInterface = dynamic_cast<const Jani2NNet*>(interface);
        satChecker = NN_SAT_CHECKER::construct_checker(*nnInterface, config); // load nn-sat checker
        if constexpr (PLAJA_GLOBAL::enableApplicabilityCache) { if (opApp) { satChecker->set_op_applicability_cache(opApp); } }
    } else {
        #ifdef USE_VERITAS
        auto interface = modelZ3->get_interface();
        auto ensembleInterface = dynamic_cast<const Jani2Ensemble*>(interface);
        satChecker = ENSEMBLE_SAT_CHECKER::construct_checker(*ensembleInterface, config);
        if constexpr (PLAJA_GLOBAL::enableApplicabilityCache) { if (opApp) { satChecker->set_op_applicability_cache(opApp); } }
        #endif
    }

    // Solution checker.
    solutionChecker = std::make_unique<SolutionCheckerPa>(config, modelZ3->get_model(), modelZ3->get_interface());
    PLAJA_ASSERT(not modelZ3->get_interface() or solutionChecker->has_policy())
    if (satChecker) { satChecker->set_solution_checker(solutionChecker.get()); }

    reset();
}

ExpansionPABase::~ExpansionPABase() = default;

/* */

void ExpansionPABase::increment() {
    /* Reset solver. */
    solver->clear();
    const auto start = modelZ3->get_start();
    if (start) { modelZ3->register_src_bounds(*solver, *start); }
    const auto reach = modelZ3->get_reach();
    if (reach) { modelZ3->register_src_bounds(*solver, *reach); }
    for (PredicateIndex_type index = 0; index < modelZ3->get_number_predicates(); ++index) { modelZ3->register_src_bounds(*solver, *modelZ3->get_predicate(index)); }

    /* Increment exploration structures. */

    if (reachChecker) { reachChecker->increment(); }

    if (terminalChecker) { terminalChecker->increment(); }

    successorGenerator->increment();

    if (predicateRelations) { predicateRelations->increment(); }

    if (satChecker) { satChecker->increment(); }
}

void ExpansionPABase::reset() {
    cache = nullptr;
    if constexpr (PLAJA_GLOBAL::enableTerminalStateSupport) { mayBeTerminal = true; }
#if 0
    solutionsStack.clear();
    push_solution(std::make_unique<StateValues>(solutionChecker->get_solution_constructor())); // dummy solution
    solutionStackSize.clear();
#endif
}

//

const SuccessorGeneratorC& ExpansionPABase::get_concrete_suc_gen() { return successorGenerator->get_concrete(); }

void ExpansionPABase::print_stats() const { if (satChecker) { satChecker->print_statistics(); } }

/** expansion cache **************************************************************************************************/

inline bool ExpansionPABase::do_global_reachability_search() const { return PLAJA_GLOBAL::paOnlyReachability or goalPathSearch or paStateReachability->get_value() == PLAJA_OPTION::EnumOptionValue::PaReachabilityGlobal; }

inline PATransitionStructure& ExpansionPABase::access_transition_struct() { return cache->transitionStruct; }

inline std::list<StateID_type>& ExpansionPABase::access_newly_reached() { return cache->newlyReached; }

bool ExpansionPABase::cache_current_successors() const { return not do_global_reachability_search() and not paStateReachability->in_none(); }

inline std::unordered_set<StateID_type>& ExpansionPABase::access_current_successors() { return cache->currentSuccessors; }

bool ExpansionPABase::is_current_successor(StateID_type id) const { return PLAJA_GLOBAL::paCacheSuccessors and cache->currentSuccessors.count(id); }

inline bool& ExpansionPABase::access_action_is_learned() { return cache->actionIsLearned; }

inline bool& ExpansionPABase::access_found_transition() { return cache->foundTransition; }

inline bool& ExpansionPABase::access_hit_return() { return cache->hitReturn; }

inline SearchDepth_type ExpansionPABase::get_current_search_depth() const { return cache->currentSearchDepth; }

inline SearchDepth_type ExpansionPABase::get_upper_bound_goal_distance() const { return cache->upperBoundGoalDistance; }

inline void ExpansionPABase::min_upper_bound_goal_distance(SearchDepth_type value) {
    auto& ub_goal_distance = cache->upperBoundGoalDistance;
    ub_goal_distance = std::min(ub_goal_distance, value);
}

/** solution cache ****************************************************************************************************/

void ExpansionPABase::compute_transition_for_label(const StateValues& solution) {
    auto& transition_struct = access_transition_struct();

    PLAJA_ASSERT(transition_struct._source())
    PLAJA_ASSERT(transition_struct.has_action_label())
    PLAJA_ASSERT(not access_action_is_learned() or solutionChecker->check_policy(solution, transition_struct._action_label(), access_transition_struct()._source()));

    for (const auto* op: successorGenerator->get_action_concrete(solution, transition_struct._action_label())) {
        transition_struct.set_action_op(successorGenerator->get_action_op_pa(op->_op_id()));
        cache->set_op_applicable(op->_op_id(), true);
        compute_transition_for_operator(solution, access_action_is_learned());
    }
    transition_struct.unset_action_op();

}

void ExpansionPABase::compute_transition_for_operator(const StateValues& solution, bool under_policy) {
    auto& transition_struct = access_transition_struct();

    PLAJA_ASSERT(transition_struct._source())
    PLAJA_ASSERT(transition_struct.has_action_op())

    /* Should be valid source at this point. */
    PLAJA_ASSERT(under_policy or solutionChecker->check_applicability(solution, transition_struct._action_op()->_concrete(), access_transition_struct()._source()));
    PLAJA_ASSERT(not under_policy or solutionChecker->check_policy_applicability(solution, transition_struct._action_op()->_concrete(), access_transition_struct()._source()));

    for (auto it = transition_struct._action_op()->updateIteratorPA(); !it.end(); ++it) {
        transition_struct.set_update(it.update_index(), it.update());
        transition_struct.set_update_op_id(searchSpace->compute_update_op(transition_struct));
        compute_transition_for_update(solution, under_policy);
        transition_struct.unset_update();
    }

}

void ExpansionPABase::compute_transition_for_update(const StateValues& solution, bool under_policy) {
    auto& transition_struct = access_transition_struct();

    PLAJA_ASSERT(transition_struct._source())
    PLAJA_ASSERT(transition_struct.has_action_op())
    PLAJA_ASSERT(not transition_struct._target())

    /* Should be valid source at this point. */
    PLAJA_ASSERT(under_policy or solutionChecker->check_applicability(solution, transition_struct._action_op()->_concrete(), access_transition_struct()._source()));
    PLAJA_ASSERT(not under_policy or solutionChecker->check_policy_applicability(solution, transition_struct._action_op()->_concrete(), access_transition_struct()._source()));

    /*
     * The SMT solver may return an unreachable state as solution, such that an oob-successor is no modelling error.
     * If so, we do not have a valid solution and abort.
     */
    STMT_IF_DEBUG(StateBase::suppress_is_valid(true);)
    auto successor = solutionChecker->compute_successor_solution(solution, transition_struct._action_op()->_concrete(), transition_struct._update_index());
    if (not solutionChecker->check_state(successor)) { return; }
    STMT_IF_DEBUG(StateBase::suppress_is_valid(false);)

    transition_struct.set_target(searchSpace->compute_abstraction(successor)); // PUSH
    sharedStats->inc_attr_unsigned(PLAJA::StatsUnsigned::GENERATED_STATES);

    if (solutionChecker->check_abstraction(successor, *transition_struct._target())) { handle_transition_existence(solution, successor, under_policy); }

    transition_struct.unset_target(); // POP

}

void ExpansionPABase::handle_transition_existence(const StateValues& solution, const StateValues& successor, bool under_policy) {
    auto& transition_struct = access_transition_struct();

    cache->set_transition(transition_struct.get_update_op_id(), *transition_struct._target(), under_policy);

    /* Register transition. */
    if (under_policy) {

        // TODO enforce if condition as invariant
        if (not searchSpace->access_inc_ss().transition_exists(*transition_struct._source(), transition_struct.get_update_op_id(), *transition_struct._target())) {
            searchSpace->add_transition(transition_struct, solution, successor);
        }

        // TODO, for now keep node processing order.
        // auto target_node = searchSpace->add_node(transition_struct._target()->to_ptr());
        // if (pre_check_node(target_node)) { handle_transition_existence(target_node); }
    }

}

#if 0

void ExpansionPABase::push_solution(std::unique_ptr<StateValues>&& solution, std::unique_ptr<StateValues>&& target_solution) {
    PLAJA_ASSERT(solutionChecker->check_state(*solution))
    // PLAJA_ASSERT(not target_solution or solutionChecker->check_state(*target_solution)) // -> target may actually be invalid
    solutionsStack.emplace_back(std::move(solution), std::move(target_solution));
}

void ExpansionPABase::push_solution(std::unique_ptr<StateValues>&& solution) { push_solution(std::move(solution), nullptr); }

void ExpansionPABase::push_solution(StateValues&& solution) { push_solution(std::make_unique<StateValues>(std::move(solution))); }

void ExpansionPABase::pop_down(std::size_t stack_size) {
    PLAJA_ASSERT(solutionsStack.size() >= stack_size)
    while (solutionsStack.size() > stack_size) { solutionsStack.pop_back(); }
}

const StateValues& ExpansionPABase::get_last_solution() const {
    PLAJA_ASSERT(not solutionsStack.empty())
    PLAJA_ASSERT(solutionsStack.back().first)
    return *solutionsStack.back().first;
}

const StateValues* ExpansionPABase::get_last_target_solution() const {
    PLAJA_ASSERT(not solutionsStack.empty())
    return solutionsStack.back().second.get();
}

#endif

/** incremental *******************************************************************************************************/

void ExpansionPABase::push_source(std::unique_ptr<AbstractState>&& source) {
    PUSH_LAP(sharedStats, PLAJA::StatsDouble::PA_PUSH_POP_SRC_TIME)

    PLAJA_ASSERT(not PLAJA_GLOBAL::enableApplicabilityCache or not opApp or opApp->empty())

    auto& transition_struct = access_transition_struct();

    transition_struct.set_source(std::move(source));
    solver->push();
    modelZ3->add_to(*transition_struct._source(), *solver);
    if constexpr (PLAJA_GLOBAL::enableTerminalStateSupport) {
        PLAJA_ASSERT(not expectExactTerminalCheck or not mayBeTerminal)
        if (mayBeTerminal) { modelZ3->exclude_terminal(*solver, 0); }
    }

    if (satChecker) {
        satChecker->push();
        satChecker->add_source_state(*transition_struct._source());
        if constexpr (PLAJA_GLOBAL::enableTerminalStateSupport) { if (not lazyNonTerminalPush and mayBeTerminal) { modelZ3->exclude_terminal(*solver, 0); } } // TODO optimize?
    }

    POP_LAP(sharedStats, PLAJA::StatsDouble::PA_PUSH_POP_SRC_TIME)
}

void ExpansionPABase::pop_source() {
    PUSH_LAP(sharedStats, PLAJA::StatsDouble::PA_PUSH_POP_SRC_TIME)
    if (satChecker) {
        satChecker->unset_source_state();
        satChecker->pop();
    }

    solver->pop();
    access_transition_struct().unset_source();

    if constexpr (PLAJA_GLOBAL::enableApplicabilityCache) { if (opApp) { opApp->reset(); } }

    POP_LAP(sharedStats, PLAJA::StatsDouble::PA_PUSH_POP_SRC_TIME)
}

inline bool ExpansionPABase::push_output_interface() {
    if constexpr (PLAJA_GLOBAL::enableApplicabilityCache and not PLAJA_GLOBAL::doSelectionTest) { if (opApp) { opApp->set_self_applicability(true); } }
    const bool rlt = satChecker->add_output_interface();
    if constexpr (PLAJA_GLOBAL::enableApplicabilityCache and not PLAJA_GLOBAL::doSelectionTest) { if (opApp) { opApp->unset_self_applicability(); } }
    return rlt;
}

bool ExpansionPABase::push_action(ActionLabel_type action_label) {
    PUSH_LAP(sharedStats, PLAJA::StatsDouble::PA_PUSH_POP_ACTION_LABEL_TIME)
    access_transition_struct().set_action_label(action_label);

    bool rlt = true;

    if (satChecker) {
        actionIsLearned = satChecker->is_action_selectable(action_label);
        if (actionIsLearned) {
            push_solution(); // to properly POP selection test solution
            satChecker->push();
            satChecker->set_label(action_label);
            if (not lazyInterfacePush) { rlt = push_output_interface(); }
        }
    }

    POP_LAP(sharedStats, PLAJA::StatsDouble::PA_PUSH_POP_ACTION_LABEL_TIME)

    return rlt;
}

void ExpansionPABase::pop_action() {
    PUSH_LAP(sharedStats, PLAJA::StatsDouble::PA_PUSH_POP_ACTION_LABEL_TIME)
    if (satChecker && actionIsLearned) {
        satChecker->unset_label();
        satChecker->pop();
        pop_solution(); // POP selection test solution.
    }
    actionIsLearned = false;
    access_transition_struct().unset_action_label();
    POP_LAP(sharedStats, PLAJA::StatsDouble::PA_PUSH_POP_ACTION_LABEL_TIME)
}

void ExpansionPABase::set_action_op(const ActionOpPA& action_op) {
    PUSH_LAP(sharedStats, PLAJA::StatsDouble::PA_PUSH_POP_ACTION_OP_TIME)
    access_transition_struct().set_action_op(action_op);
    push_solution(); // to properly POP applicability test solution
    POP_LAP(sharedStats, PLAJA::StatsDouble::PA_PUSH_POP_ACTION_OP_TIME)
}

void ExpansionPABase::unset_action_op() {
    PUSH_LAP(sharedStats, PLAJA::StatsDouble::PA_PUSH_POP_ACTION_OP_TIME)
    pop_solution();
    access_transition_struct().unset_action_op();
    POP_LAP(sharedStats, PLAJA::StatsDouble::PA_PUSH_POP_ACTION_OP_TIME)
}

void ExpansionPABase::push_update(UpdateIndex_type update_index, const UpdatePA& update_pa) {
    PUSH_LAP(sharedStats, PLAJA::StatsDouble::PA_PUSH_POP_UPD_TIME)

    auto& transition_struct = access_transition_struct();
    transition_struct.set_update(update_index, update_pa);
    PLAJA_ASSERT(transition_struct.has_action_op())
    PLAJA_ASSERT(transition_struct.has_action_label())
    transition_struct.set_update_op_id(searchSpace->compute_update_op(transition_struct));

    solver->push();
    update_pa.add_to_solver(*solver);

    if (satChecker and actionIsLearned) {
        satChecker->push();
        satChecker->set_update(update_index);
        satChecker->add_update();
    }

    if constexpr (PLAJA_GLOBAL::reuseSolutions) {
        push_solution();
#if 0
        // Sample target solution for update.
        /*
         * TODO
         *  Since we do not cache the solution of the app test in non-inc exploration,
         *  get_last_solution() returns a default state.
         *  The operator guard may not be applicable and hence out-of-bounds-updates are no modelling error.
         *  Moreover, the SMT solver may return an unreachable state as solution, where, again, an oob-successor is no modelling error.
         *  We just suppress the oob check here as a quick fix.
         *  This is dirty. There really is no point in trying to reuse the cached "non-solution".
         *  However, the long term long is direct solution-to-transition usage, which will make the solution stake obsolete anyway.
         */
        STMT_IF_DEBUG(StateBase::suppress_is_valid(true);)
        push_solution(std::make_unique<StateValues>(get_last_solution()), std::make_unique<StateValues>(solutionChecker->compute_successor_solution(get_last_solution(), solutionChecker->get_action_op(transition_struct._action_op_id()), transition_struct._update_index())));
        STMT_IF_DEBUG(StateBase::suppress_is_valid(false);)
#endif

    }

    POP_LAP(sharedStats, PLAJA::StatsDouble::PA_PUSH_POP_UPD_TIME)
}

void ExpansionPABase::pop_update() {
    PUSH_LAP(sharedStats, PLAJA::StatsDouble::PA_PUSH_POP_UPD_TIME)

    pop_solution();

    if (satChecker and actionIsLearned) {
        satChecker->unset_update();
        satChecker->pop();
    }

    solver->pop();
    access_transition_struct().unset_update();

    POP_LAP(sharedStats, PLAJA::StatsDouble::PA_PUSH_POP_UPD_TIME)
}

void ExpansionPABase::nn_sat_push_action_op() {
    PUSH_LAP(sharedStats, PLAJA::StatsDouble::PA_PUSH_POP_ACTION_OP_TIME)
    if (satChecker and actionIsLearned) {
        push_solution(); // To properly POP applicability test solution.
        satChecker->push();
        satChecker->set_operator(access_transition_struct()._action_op_id());
        satChecker->add_guard();
        if (lazyNonTerminalPush) { satChecker->add_non_terminal_condition(); }
        if (lazyInterfacePush) { push_output_interface(); }
    }
    POP_LAP(sharedStats, PLAJA::StatsDouble::PA_PUSH_POP_ACTION_OP_TIME)
}

void ExpansionPABase::nn_sat_pop_action_op() {
    PUSH_LAP(sharedStats, PLAJA::StatsDouble::PA_PUSH_POP_ACTION_OP_TIME)
    if (satChecker and actionIsLearned) {
        satChecker->unset_operator();
        satChecker->pop();
        pop_solution(); // POP applicability test solution.
    }
    POP_LAP(sharedStats, PLAJA::StatsDouble::PA_PUSH_POP_ACTION_OP_TIME)
}

/**********************************************************************************************************************/

bool ExpansionPABase::applicability_test() {
    sharedStats->inc_attr_unsigned(PLAJA::StatsUnsigned::APP_TESTS);

    PUSH_LAP(sharedStats, PLAJA::StatsDouble::TIME_APP_TEST)

    const auto& transition_struct = access_transition_struct();

    if constexpr (PLAJA_GLOBAL::reuseSolutions and PLAJA_GLOBAL::doSelectionTest) {

        // if (solutionChecker->check_applicability(get_last_solution(), solutionChecker->get_action_op(transition_struct._action_op_id()), transition_struct._source())) {
        if (cache->op_is_applicable(transition_struct._action_op_id(), false)) {
            sharedStats->inc_attr_unsigned(PLAJA::StatsUnsigned::REUSED_SOLUTIONS);
            POP_LAP(sharedStats, PLAJA::StatsDouble::TIME_APP_TEST)
            return true; // TODO may be beneficial to still (incrementally) check solver.
        }
    }

    const bool rlt = solver->check();
    if (rlt) {

        if constexpr (PLAJA_GLOBAL::reuseSolutions) {
            auto instance = solver->extract_solution_via_checker(*modelZ3, std::make_unique<SolutionCheckerInstancePa>(*solutionChecker, transition_struct));

            PLAJA_ASSERT(instance->get_solution_size() == 1) // No target expected.
            PLAJA_ASSERT(solutionChecker->check_applicability(*instance->get_solution(0), solutionChecker->get_action_op(transition_struct._action_op_id()), transition_struct._source()))

            // push_solution(instance->retrieve_solution(0));
            compute_transition_for_operator(*instance->get_solution(0), access_action_is_learned() and solutionChecker->check_policy(*instance->get_solution(0), transition_struct._action_label(), nullptr));
        }

    } else {
        searchSpace->access_inc_ss().set_excluded_op(*transition_struct._source(), transition_struct._action_op_id()); // Mark excluded.
        sharedStats->inc_attr_unsigned(PLAJA::StatsUnsigned::APP_TESTS_UNSAT);
    }

    if constexpr (PLAJA_GLOBAL::enableApplicabilityCache) {
        if (opApp) { opApp->set_applicability(transition_struct._action_op_id(), rlt); }
    }

    POP_LAP(sharedStats, PLAJA::StatsDouble::TIME_APP_TEST)
    return rlt;
}

bool ExpansionPABase::transition_test() {
    sharedStats->inc_attr_unsigned(PLAJA::StatsUnsigned::TRANSITION_TESTS);
    PUSH_LAP(sharedStats, PLAJA::StatsDouble::TIME_TRANSITION_TEST)

    const auto& transition_struct = access_transition_struct();

    if constexpr (PLAJA_GLOBAL::reuseSolutions) {

        if (cache->transition_computed_already(transition_struct.get_update_op_id(), *transition_struct._target(), false)) {
            sharedStats->inc_attr_unsigned(PLAJA::StatsUnsigned::REUSED_SOLUTIONS);
            POP_LAP(sharedStats, PLAJA::StatsDouble::TIME_TRANSITION_TEST)
            return true;
        }

#if 0
        // Note: not reusing solutions for non-NN SMT-tests seems to outperform,
        // i.e., reusing should/may be disabled when computing standard PA.
        // When computing PPA: Reusing significantly improves performance.
        // Presumably in this case reusing solutions for non-NN SMT-tests is ok
        // -- (more expensive) solution caching is done anyway, we only have to check the solution (which is cheap).
        // Moreover, if *not* reusing solutions for non-NN tests, we could end up pushing a non-policy solution to the stack, hiding a valid policy solution.

        PLAJA_ASSERT(get_last_target_solution())

        STMT_IF_DEBUG(StateBase::suppress_is_valid(true);)
        if (solutionChecker->check_transition(get_last_solution(), *transition_struct._source(), solutionChecker->get_action_op(transition_struct._action_op_id()), transition_struct._update_index(), *transition_struct._target(), get_last_target_solution())) {
            sharedStats->inc_attr_unsigned(PLAJA::StatsUnsigned::REUSED_SOLUTIONS);
            POP_LAP(sharedStats, PLAJA::StatsDouble::TIME_TRANSITION_TEST)
            return true;
        }
        STMT_IF_DEBUG(StateBase::suppress_is_valid(false);)
#endif

    }

    solver->push();

    access_transition_struct()._update()->add_predicates_to_solver(*solver, *access_transition_struct()._target());
    const bool rlt = solver->check();
    // solver->dump_solver_to_file("test.z3");
    if (rlt) {

        if constexpr (PLAJA_GLOBAL::reuseSolutions) {

            auto instance = solver->extract_solution_via_checker(*modelZ3, std::make_unique<SolutionCheckerInstancePa>(*solutionChecker, transition_struct));
            PLAJA_ASSERT(instance->get_solution_size() == 2)
            // std::cout << "Source meow meow: "; instance->get_solution(0)->dump(true);
            // std::cout << "Target meow meow: "; instance->get_solution(1)->dump(true);
            PLAJA_ASSERT(solutionChecker->check_transition(*instance->get_solution(0), *access_transition_struct()._source(), solutionChecker->get_action_op(transition_struct._action_op_id()), transition_struct._update_index(), *transition_struct._target(), instance->get_solution(1)))

            // push_solution(instance->retrieve_solution(0), instance->retrieve_solution(1));

            if (actionIsLearned and solutionChecker->check_policy(*instance->get_solution(0), access_transition_struct()._action_label(), nullptr)) {
                handle_transition_existence(*instance->get_solution(0), *instance->get_solution(1), true);
            }

        }
    } else { sharedStats->inc_attr_unsigned(PLAJA::StatsUnsigned::TRANSITION_TESTS_UNSAT); }

    solver->pop();

    POP_LAP(sharedStats, PLAJA::StatsDouble::TIME_TRANSITION_TEST)
    return rlt;
}

bool ExpansionPABase::nn_sat_selection_test() {
    PLAJA_ASSERT(PLAJA_GLOBAL::doSelectionTest)

    if (not(satChecker and satChecker->is_action_selectable(access_transition_struct()._action_label()))) { return true; }

    PUSH_LAP(sharedStats, PLAJA::StatsDouble::TIME_NN_SELECTION_TEST)
    sharedStats->inc_attr_unsigned(PLAJA::StatsUnsigned::NN_SELECTION_TESTS);

    const bool rlt = PLAJA_GLOBAL::doRelaxedSelectionTestOnly ? satChecker->check_relaxed() : satChecker->check();

    if (rlt) {

        if constexpr (PLAJA_GLOBAL::debug and not PLAJA_GLOBAL::doRelaxedSelectionTestOnly) {
            PLAJA_ASSERT(not satChecker->has_solution() or solutionChecker->check_policy(satChecker->get_last_solution(), access_transition_struct()._action_label(), access_transition_struct()._source()))
        }

        if constexpr (PLAJA_GLOBAL::reuseSolutions) {
            if (satChecker->has_solution()) { compute_transition_for_label(*satChecker->release_solution()); }
        }

    } else {
        searchSpace->access_inc_ss().set_excluded_label(*access_transition_struct()._source(), access_transition_struct()._action_label()); // mark excluded
        sharedStats->inc_attr_unsigned(PLAJA::StatsUnsigned::NN_SELECTION_TESTS_UNSAT);
    }

    POP_LAP(sharedStats, PLAJA::StatsDouble::TIME_NN_SELECTION_TEST)
    return rlt;
}

bool ExpansionPABase::nn_sat_applicability_test() {
    PLAJA_ASSERT(PLAJA_GLOBAL::doNNApplicabilityTest)

    if (not(satChecker and satChecker->is_action_selectable(access_transition_struct()._action_label()))) { return true; }

    PUSH_LAP(sharedStats, PLAJA::StatsDouble::TIME_NN_APP_TEST)
    sharedStats->inc_attr_unsigned(PLAJA::StatsUnsigned::NN_APP_TESTS);

    if constexpr (PLAJA_GLOBAL::reuseSolutions) {
        // if (solutionChecker->check_policy_applicability(get_last_solution(), solutionChecker->get_action_op(transition_struct._action_op_id()), transition_struct._source())) {
        if (cache->op_is_applicable(access_transition_struct()._action_op_id(), true)) {
            sharedStats->inc_attr_unsigned(PLAJA::StatsUnsigned::REUSED_SOLUTIONS);
            POP_LAP(sharedStats, PLAJA::StatsDouble::TIME_NN_APP_TEST)
            return true;
        }
    }

    const bool rlt = PLAJA_GLOBAL::doRelaxedNNApplicabilityTestOnly ? satChecker->check_relaxed() : satChecker->check();

    if (rlt) {

        if constexpr (PLAJA_GLOBAL::debug and not PLAJA_GLOBAL::doRelaxedNNApplicabilityTestOnly) {
            PLAJA_ASSERT(not satChecker->has_solution() or solutionChecker->check_policy_applicability(satChecker->get_last_solution(), successorGenerator->get_action_op_concrete(access_transition_struct()._action_op_id()), access_transition_struct()._source()))
        }

        if constexpr (PLAJA_GLOBAL::reuseSolutions) {
            if (satChecker->has_solution()) {
                // push_solution(satChecker->release_solution());
                compute_transition_for_operator(*satChecker->release_solution(), true);
            }
        }

    } else {
        sharedStats->inc_attr_unsigned(PLAJA::StatsUnsigned::NN_APP_TESTS_UNSAT);
    }

    POP_LAP(sharedStats, PLAJA::StatsDouble::TIME_NN_APP_TEST)
    return rlt;
}

bool ExpansionPABase::nn_sat_transition_test() {

    if (not(satChecker and satChecker->is_action_selectable(access_transition_struct()._action_label()))) { return true; } // No nn transition test necessary.

    PUSH_LAP(sharedStats, PLAJA::StatsDouble::TIME_NN_TRANSITION_TEST)
    sharedStats->inc_attr_unsigned(PLAJA::StatsUnsigned::NN_TRANSITION_TESTS);

    if constexpr (PLAJA_GLOBAL::reuseSolutions) {

        if (cache->transition_computed_already(access_transition_struct().get_update_op_id(), *access_transition_struct()._target(), actionIsLearned)) {
            sharedStats->inc_attr_unsigned(PLAJA::StatsUnsigned::REUSED_SOLUTIONS);
            POP_LAP(sharedStats, PLAJA::StatsDouble::TIME_NN_TRANSITION_TEST)
            return true;
        }

#if 0
        const auto& last_solution = get_last_solution();
        const auto& last_target_solution = get_last_target_solution();
        PLAJA_ASSERT(last_target_solution)

        if (solutionChecker->check_policy_transition(last_solution, *transition_struct._source(), solutionChecker->get_action_op(transition_struct._action_op_id()), transition_struct._update_index(), *transition_struct._target(), last_target_solution)) {
            sharedStats->inc_attr_unsigned(PLAJA::StatsUnsigned::REUSED_SOLUTIONS);
            POP_LAP(sharedStats, PLAJA::StatsDouble::TIME_NN_TRANSITION_TEST)
            searchSpace->add_transition(transition_struct, last_solution, *last_target_solution); // TODO For now we only add transitions possible under policy-restriction (see also below).
            return true;
        }
#endif
    }

    satChecker->push();
    satChecker->add_target_state(*access_transition_struct()._target());
    const bool rlt = satChecker->check();
    satChecker->unset_target_state();
    satChecker->pop();

    if (rlt) {

        if constexpr (PLAJA_GLOBAL::debug) {
            STMT_IF_DEBUG(PLAJA_GLOBAL::push_additional_outputs_frame(true);)
            PLAJA_ASSERT(not satChecker->has_solution()
                         or (satChecker->get_target_solution() and
                             solutionChecker->check_policy_transition(satChecker->get_last_solution(), *access_transition_struct()._source(), solutionChecker->get_action_op(access_transition_struct()._action_op_id()), access_transition_struct()._update_index(), *access_transition_struct()._target(), satChecker->get_target_solution())
                         ))
            STMT_IF_DEBUG(PLAJA_GLOBAL::pop_additional_outputs_frame();)
        }

        if (satChecker->has_solution()) { // E.g., due to handling unknown as sat. TODO solution extraction not supported by MarabouInZ3Checker.
            PLAJA_ASSERT(satChecker->get_target_solution()) // Redundant, see above. Though, why not ...
            #ifdef USE_VERITAS
            if (intervalRefinement && satChecker->has_solution_box()) {
                auto ensembleChecker = dynamic_cast<const EnsembleSatChecker*>(satChecker.get());
                auto box = ensembleChecker->release_box();
                searchSpace->add_transition(access_transition_struct(), *satChecker->release_solution(), *satChecker->release_target_solution(), box);
            } else {
                searchSpace->add_transition(access_transition_struct(), *satChecker->release_solution(), *satChecker->release_target_solution());
            }
            #else
                searchSpace->add_transition(access_transition_struct(), *satChecker->release_solution(), *satChecker->release_target_solution());
            #endif
            // Nothing to reuse it for ... // if constexpr (PLAJA_GLOBAL::reuseSolutions) { push_solution(satChecker->release_solution(), satChecker->release_target_solution()); }
        }

    } else {
        sharedStats->inc_attr_unsigned(PLAJA::StatsUnsigned::NN_TRANSITION_TESTS_UNSAT);
        searchSpace->access_inc_ss().set_excluded(*access_transition_struct()._source(), access_transition_struct().get_update_op_id(), *access_transition_struct()._target()); // TODO currently we only cache transition excluded under policy-restriction.
    }

    POP_LAP(sharedStats, PLAJA::StatsDouble::TIME_NN_TRANSITION_TEST)
    return rlt;
}

/**********************************************************************************************************************/

std::list<StateID_type> ExpansionPABase::step(StateID_type src_id) {

    reset();
    const auto src_node = searchSpace->get_node(src_id);
    cache = std::make_unique<ExpansionCachePa>(src_node);
    const auto& transition_struct = access_transition_struct();

    /* Expand state. */

    sharedStats->inc_attr_unsigned(PLAJA::StatsUnsigned::EXPANDED_STATES);

    // Check terminal?
    if constexpr (PLAJA_GLOBAL::enableTerminalStateSupport) {

        if (terminalChecker) {

            mayBeTerminal = terminalChecker->check(src_node.get_state());
            STMT_IF_TERMINAL_STATE_SUPPORT(if (mayBeTerminal) { sharedStats->inc_attr_unsigned(PLAJA::StatsUnsigned::TERMINAL_TESTS_SAT); })
            PLAJA_ASSERT_EXPECT(not expectExactTerminalCheck or terminalChecker->is_exact()) // Expected for now.
            if (mayBeTerminal and terminalChecker->is_exact()) {
                sharedStats->inc_attr_unsigned(PLAJA::StatsUnsigned::TERMINAL_STATES);
                return {}; // std::move(access_newly_reached());
            }

        }

    }

    push_source(src_node.get_state().to_ptr()); // PUSH SOURCE STATE

    /* Refine incremental state space (move witness downwards). */
    PUSH_LAP(sharedStats, PLAJA::StatsDouble::PA_SS_REFINE_TIME)
    searchSpace->access_inc_ss().refine_for(*transition_struct._source());
    POP_LAP(sharedStats, PLAJA::StatsDouble::PA_SS_REFINE_TIME)

    /* Explore. */
    PUSH_LAP(sharedStats, PLAJA::StatsDouble::PA_SUC_GEN_EXPLORE_TIME)
    if (incOpAppTest) { successorGenerator->explore_for_incremental(*transition_struct._source(), *solver); }
    else { successorGenerator->explore(*transition_struct._source(), *solver); }
    POP_LAP(sharedStats, PLAJA::StatsDouble::PA_SUC_GEN_EXPLORE_TIME)

    bool terminal_state = true; // Under policy restriction.

    /* Iterate action labels. */
    PUSH_LAP(sharedStats, PLAJA::StatsDouble::PA_ACTION_LABEL_IT_TIME)
    for (auto action_it = successorGenerator->init_action_it_pa(); !action_it.end(); ++action_it) {

        /* Already excluded? */
        if (searchSpace->access_inc_ss().is_excluded_label(*transition_struct._source(), action_it.action_label())) { continue; }

        if (not push_action(action_it.action_label())) { // PUSH ACTION
            pop_action(); // POP ACTION
            continue;
        }

        if constexpr (PLAJA_GLOBAL::doSelectionTest) {
            if (not nn_sat_selection_test()) {
                pop_action(); // POP ACTION
                continue;
            }
        }

        bool any_applicable_op = false;
        bool found_transition = false; // Under policy restriction.

        /* Iterate action operators. */
        PUSH_LAP(sharedStats, PLAJA::StatsDouble::PA_ACTION_OP_IT_TIME)
        for (auto per_action_it = action_it.iterator(); !per_action_it.end(); ++per_action_it) {
            const ActionOpPA& action_op = *per_action_it;

            /* Already excluded? */
            if (searchSpace->access_inc_ss().is_excluded_op(*transition_struct._source(), action_op._op_id())) { continue; }

            set_action_op(action_op); // SET ACTION OP

            solver->push(); // PUSH GUARD
            action_op.add_to_solver(*solver);

            if (incOpAppTest) { // Incremental solving.
                if (not applicability_test()) {
                    solver->pop(); // POP GUARD
                    unset_action_op(); // UNSET ACTION OP
                    continue;
                }
            } // else: // Already checked applicability.

            any_applicable_op = true;
            found_transition = step_per_op() or found_transition;
            solver->pop(); // POP GUARD
            unset_action_op(); // UNSET ACTION OP

            if (not do_global_reachability_search() and paStateReachability->get_value() == PLAJA_OPTION::EnumOptionValue::PaOnlyReachabilityPerOp) { access_current_successors().clear(); }
            if (access_hit_return()) { break; } // PATH_SEARCH: termination condition.
        }
        POP_LAP(sharedStats, PLAJA::StatsDouble::PA_ACTION_OP_IT_TIME)

        if (not any_applicable_op) { searchSpace->access_inc_ss().set_excluded_label(*transition_struct._source(), transition_struct._action_label()); }
        if (found_transition) {
            sharedStats->inc_attr_unsigned(PLAJA::StatsUnsigned::ACTION_LABELS);
            terminal_state = false;
        }

        pop_action(); // POP ACTION

        if (not do_global_reachability_search() and paStateReachability->get_value() == PLAJA_OPTION::EnumOptionValue::PaOnlyReachabilityPerLabel) { access_current_successors().clear(); }
        if (access_hit_return()) { break; } // PATH_SEARCH: termination condition.
    }
    POP_LAP(sharedStats, PLAJA::StatsDouble::PA_ACTION_LABEL_IT_TIME)

    /* Terminal state? */
    if (terminal_state) { sharedStats->inc_attr_unsigned(PLAJA::StatsUnsigned::TERMINAL_STATES); }

    pop_source(); // POP SOURCE STATE

    return std::move(access_newly_reached());
}

bool ExpansionPABase::step_per_op() {

    nn_sat_push_action_op(); // PUSH ACTION OP
    if constexpr (PLAJA_GLOBAL::doNNApplicabilityTest) {
        if (not nn_sat_applicability_test()) {
            nn_sat_pop_action_op(); // POP ACTION OP
            return false;
        }
    }

    bool found_transition = false;
    const auto& transition_struct = access_transition_struct();

    PUSH_LAP(sharedStats, PLAJA::StatsDouble::PA_UPDATE_IT_TIME)
    for (auto update_it = transition_struct._action_op()->updateIteratorPA(); !update_it.end(); ++update_it) { // for each PA update structure

        const auto& update = update_it.update();
        push_update(update_it.update_index(), update); // PUSH UPDATE

        PredicateState predicate_state(modelZ3->get_number_predicates(), modelZ3->get_number_automata_instances(), modelZ3->_predicates());
        transition_struct._source()->to_location_state(predicate_state.get_location_state()); // copy non-updated locations.
        update.evaluate_locations(predicate_state.get_location_state());

        /* Entailment Optimizations. */
        PUSH_LAP(sharedStats, PLAJA::StatsDouble::PA_UPD_ENTAILMENT_TIME)
        {
            const PredEntailmentCacheInterface entailments_cache_interface(searchSpace->access_inc_ss(), transition_struct, predicateRelations.get());
            update.fix(predicate_state, *solver, *transition_struct._source(), entailments_cache_interface);
        }
        POP_LAP(sharedStats, PLAJA::StatsDouble::PA_UPD_ENTAILMENT_TIME)
        STMT_IF_DEBUG(if (predicateRelations) { predicateRelations->check_all_entailments(predicate_state); }) // ./PlaJA ../../benchmarks/racetrack-C6/benchmarks/tiny/tiny_09.jani --additional_properties ../../benchmarks/racetrack-C6/additional_properties/predicates/tiny/tiny_c_map_c_car.jani --engine PREDICATE_ABSTRACTION --prop 8 --predicate-relations --predicate-sat

        enumerate_pa_states(predicate_state);
        found_transition = access_found_transition();

        pop_update(); // POP UPDATE

        if (access_hit_return()) { break; } // PATH_SEARCH: termination condition.
    }
    POP_LAP(sharedStats, PLAJA::StatsDouble::PA_UPDATE_IT_TIME)

    nn_sat_pop_action_op(); // POP ACTION OP

    if (found_transition) { sharedStats->inc_attr_unsigned(PLAJA::StatsUnsigned::ACTION_OPS); }
    return found_transition;
}

void ExpansionPABase::enumerate_pa_states(PredicateState& pred_state) {
    access_found_transition() = false;
    PUSH_LAP(sharedStats, PLAJA::StatsDouble::PA_SUCCESSOR_ENUM_TIME)
    auto pred_traverser = searchSpace->init_predicate_traverser();
    enumerate_pa_states(pred_state, pred_traverser);
    POP_LAP(sharedStats, PLAJA::StatsDouble::PA_SUCCESSOR_ENUM_TIME)
}

void ExpansionPABase::enumerate_pa_states(PredicateState& pred_state, PredicateTraverser& predicate_traverser) { //NOLINT(*-no-recursion)

    if (predicate_traverser.end()) {

        compute_transition(pred_state);
        access_transition_struct().unset_target();

    } else { // Continue recursion.

        const auto pred_index = predicate_traverser.predicate_index();

        if (pred_state.is_defined(pred_index)) {

            PLAJA_ASSERT(pred_state.is_entailed(pred_index) and not pred_state.is_set(pred_index))
            pred_state.set(pred_index, pred_state.predicate_value(pred_index));

            predicate_traverser.next(pred_state);

            enumerate_pa_states(pred_state, predicate_traverser);

            predicate_traverser.previous();

        } else {
            PLAJA_ASSERT(pred_state.is_undefined(pred_index))

            /* First branch */
            if constexpr (PLAJA_GLOBAL::queryPerBranchExpansion) {
                solver->push();
                access_transition_struct()._update()->add_predicate_to_solver(*solver, pred_index, true);
            }

            if (not PLAJA_GLOBAL::queryPerBranchExpansion or solver->check()) {

                pred_state.push_layer();
                pred_state.set(pred_index, true);
                if (predicateRelations) { predicateRelations->fix_predicate_state_asc_if_non_lazy(pred_state, pred_index); }

                predicate_traverser.next(pred_state);

                enumerate_pa_states(pred_state, predicate_traverser);

                predicate_traverser.previous();
                pred_state.pop_layer();

                if (access_hit_return()) { return; } // PATH_SEARCH: termination condition.
            }

            if constexpr (PLAJA_GLOBAL::queryPerBranchExpansion) { solver->pop(); }
            /* End first branch */

            /* Second branch */
            if constexpr (PLAJA_GLOBAL::queryPerBranchExpansion) {
                solver->push();
                access_transition_struct()._update()->add_predicate_to_solver(*solver, pred_index, false);
            }

            if (not PLAJA_GLOBAL::queryPerBranchExpansion or solver->check()) {

                pred_state.push_layer();
                pred_state.set(pred_index, false);
                if (predicateRelations) { predicateRelations->fix_predicate_state_asc_if_non_lazy(pred_state, pred_index); }

                predicate_traverser.next(pred_state);

                enumerate_pa_states(pred_state, predicate_traverser);

                predicate_traverser.previous();
                pred_state.pop_layer();
            }

            if constexpr (PLAJA_GLOBAL::queryPerBranchExpansion) { solver->pop(); }
            /* End second branch */

        }

    }

}

void ExpansionPABase::compute_transition(PredicateState& pred_state) {

    /* Generate potential target. */
    auto& transition_struct = access_transition_struct();
    transition_struct.set_target(searchSpace->set_abstract_state(pred_state));
    sharedStats->inc_attr_unsigned(PLAJA::StatsUnsigned::GENERATED_STATES);
    auto target_node = searchSpace->add_node(transition_struct._target()->to_ptr());

    if (not pre_check_node(target_node)) { return; }

    // TODO maybe once we have some public MT traverser (e.g., for lazy abstraction),
    //  we can check for exclusion and existence separately, the former prior to adding the node or even during successor enumeration.
    const auto witness = searchSpace->access_inc_ss().transition_exists(*transition_struct._source(), transition_struct.get_update_op_id(), *transition_struct._target());
    if (witness) { // Already know result.

        if (*witness != STATE::none) {
            sharedStats->inc_attr_unsigned(PLAJA::StatsUnsigned::REUSED_TRANSITION_EXISTENCE);
            handle_transition_existence(target_node);
        } else { // Excluded, i.e., transition does not exist.
            sharedStats->inc_attr_unsigned(PLAJA::StatsUnsigned::REUSED_TRANSITION_EXCLUSION);
            if (PLAJA_GLOBAL::useIncMt and not PLAJA_GLOBAL::useRefReg and not target_node.is_reached()) { searchSpace->unregister_state_if_possible(target_node.get_id()); }
        }

        return;
    }

    /* SMT checks. */

    if constexpr (not PLAJA_GLOBAL::queryPerBranchExpansion) { push_solution(); } // PUSH transition test solution frame.

    if ((PLAJA_GLOBAL::queryPerBranchExpansion or transition_test()) and nn_sat_transition_test()) {
        handle_transition_existence(target_node);
    } else if (PLAJA_GLOBAL::useIncMt and not PLAJA_GLOBAL::useRefReg and not target_node.is_reached()) { searchSpace->unregister_state_if_possible(target_node.get_id()); }

    if constexpr (not PLAJA_GLOBAL::queryPerBranchExpansion) { pop_solution(); } // POP transition test solution frame.

}

bool ExpansionPABase::pre_check_node(const SEARCH_SPACE_PA::SearchNode& target_node) const {

    if (target_node().is_dead_end()) { PLAJA_ABORT /* return false; */ } // Do not expect any dead ends for now.

    /* Only reachability? */
    if (PLAJA_GLOBAL::paOnlyReachability or goalPathSearch) {
        if (target_node.is_reached()) {
            if (not optimalSearch or target_node().get_start_distance() <= get_current_search_depth() + 1) { // For optimal search we may have to reopen.
                return false;
            }
        }
        /* Else: if only interested in step-reachability, we may still skip, if there is already an update from current source to target. */
    } else if (is_current_successor(target_node.get_id())) { return false; }

    return true;

}

void ExpansionPABase::handle_transition_existence(SEARCH_SPACE_PA::SearchNode& target_node) {

    PLAJA_ASSERT(not access_hit_return())

    const auto& transition_struct = access_transition_struct();
    const auto& target = *transition_struct._target();
    const auto target_id = transition_struct._target_id();

    access_found_transition() = true;
    sharedStats->inc_attr_unsigned(PLAJA::StatsUnsigned::TRANSITIONS);

    /* Handle newly reached. */
    if (not target_node.is_reached()) {

        target_node.set_reached(transition_struct._source_id(), transition_struct.get_update_op_id(), get_current_search_depth() + 1);
        sharedStats->inc_attr_unsigned(PLAJA::StatsUnsigned::REACHABLE_STATES);

        /* Goal check. */
        if (target_node().has_goal_path()) {

            searchSpace->add_goal_path_frontier(target_id);

            if (goalPathSearch and not optimalSearch) { access_hit_return() = true; }
            else { min_upper_bound_goal_distance(target_node().get_start_distance() + target_node().get_goal_distance()); }

        } else if (reachChecker->check(target)) {

            target_node().set_goal();
            sharedStats->inc_attr_unsigned(PLAJA::StatsUnsigned::GOAL_STATES);
            searchSpace->add_goal_path_frontier(target_id);

            if (goalPathSearch and not optimalSearch) { access_hit_return() = true; }
            else { min_upper_bound_goal_distance(target_node().get_start_distance()); }

            /* Else: cache for expansion; however if searching for goal path, we may skip if start distance already exceed goal path upper bound. */
        } else if (not goalPathSearch or target_node().get_start_distance() + 1 < get_upper_bound_goal_distance()) {
            access_newly_reached().push_back(target_id);
        }

    } else if (optimalSearch and get_current_search_depth() + 1 < target_node().get_start_distance()) { // Reopen.

        target_node.set_reached(transition_struct._source_id(), transition_struct.get_update_op_id(), get_current_search_depth() + 1);

        /* Goal check. */
        if (target_node().has_goal_path()) { min_upper_bound_goal_distance(target_node().get_start_distance() + target_node().get_goal_distance()); }
        else if (not goalPathSearch or target_node().get_start_distance() + 1 < get_upper_bound_goal_distance()) {
            /* Else: cache for expansion; however if searching for goal path, we may skip if start distance already exceed goal path upper bound. */
            access_newly_reached().push_back(target_id);
        }

    } else { // Additional parent.
        target_node().add_parent(transition_struct._source_id(), transition_struct.get_update_op_id());
    }

    if constexpr (PLAJA_GLOBAL::paCacheSuccessors) {
        PLAJA_ASSERT(not access_current_successors().count(target_id))
        access_current_successors().insert(target_id); // Cache successor in case we are only interested in step-reachability.
    }

}
