//
// This file is part of the PlaJA code base.
// Copyright (c) (2019 - 2022) Marcel Vinzent.
// See README.md in the top-level directory for licensing information.
//

#include "spuriousness_checker.h"
#include "../../../parser/visitor/extern/to_normalform.h"
#include "../../../utils/structs_utils/update_op_ref.h"
#include "../../factories/predicate_abstraction/pa_options.h"
#include "../../factories/configuration.h"
#include "../../successor_generation/action_op.h"
#include "../pa_states/abstract_path.h"
#include "../pa_states/abstract_state.h"
#include "../solution_checker_pa.h"
#include "../solution_checker_instance_pa.h"
#include "spuriousness_result.h"
#include "path_existence_checker.h"
#include "../../information/jani_2_interface.h"
#ifdef USE_VERITAS
#include "interval.hpp"
#endif

SpuriousnessChecker::SpuriousnessChecker(const PLAJA::Configuration& configuration, const PredicatesStructure& predicates_ref):
    predicates(&predicates_ref)
    , pathExistenceChecker(nullptr)
    , paStateAware(configuration.get_bool_option(PLAJA_OPTION::pa_state_aware))
    , checkPolicySpuriousness(configuration.get_bool_option(PLAJA_OPTION::check_policy_spuriousness))
    , encounteredNonDetUpdate(false) {

    PLAJA_ASSERT(configuration.has_sharable_const(PLAJA::SharableKey::MODEL));

    pathExistenceChecker = std::make_unique<PathExistenceChecker>(configuration, *predicates);

    if (pathExistenceChecker->has_nn()) {
        PLAJA_ASSERT(pathExistenceChecker->get_interface())
        selectionRefinement = std::make_unique<SelectionRefinement>(configuration, *pathExistenceChecker->get_interface());
    } else {
        #ifdef USE_VERITAS
        PLAJA_LOG("Inside spuriousness checker, ensembles refinement")
        selectionRefinement = std::make_unique<SelectionRefinement>(configuration, *pathExistenceChecker->get_interface());
        #endif
    }

}

SpuriousnessChecker::~SpuriousnessChecker() = default;

/**********************************************************************************************************************/

inline void SpuriousnessChecker::add_relevant_pred_split(SpuriousnessResult& spuriousness_result, const Expression& predicate, const StateBase& state) {
    auto split_predicates = TO_NORMALFORM::normalize_and_split(predicate, false); // to preserve linearity
    for (auto& split_pred: split_predicates) {
        if (split_pred->evaluate_integer(state)) { continue; } // Split is satisfied, no need to add.
        spuriousness_result.add_refinement_predicate(std::move(split_pred), spuriousness_result.get_spurious_prefix_length());
    }
}

void SpuriousnessChecker::add_violated_predicates(SpuriousnessResult& spuriousness_result, const AbstractState& abstract_state, const StateBase& state) {
    for (auto it = abstract_state.init_pred_state_iterator(); !it.end(); ++it) {
        const auto& predicate = it.predicate();
        PLAJA_ASSERT(PLAJA_GLOBAL::lazyPA or it.is_set())
        if constexpr (PLAJA_GLOBAL::lazyPA) { if (not it.is_set()) { continue; } }
        if (it.predicate_value() != predicate.evaluate_integer(state)) {
            PLAJA_ASSERT(not predicate.is_constant())
            PLAJA_ASSERT(TO_NORMALFORM::normalize_and_split(predicate, true).size() == 1)
            spuriousness_result.add_refinement_predicate(it.predicate_index(), spuriousness_result.get_spurious_prefix_length());
        }
    }
}

std::unique_ptr<SpuriousnessResult> SpuriousnessChecker::check_path(std::unique_ptr<SolutionCheckerInstancePaPath>&& path_instance) const {
    STMT_IF_DEBUG(constexpr bool debug_log = false)
    PLAJA_LOG_DEBUG_IF(debug_log, "Checking path concretization ...")

    const auto& solution_checker = path_instance->get_checker_pa();
    const auto& path = path_instance->get_path();

    PLAJA_ASSERT(path_instance->get_solution(0))
    PLAJA_ASSERT(path_instance->get_solution_size() == path.get_state_path_size())
    PLAJA_ASSERT(path_instance->get_target_step() == path.get_state_path_size() - 1)
    PLAJA_ASSERT(path_instance->get_policy_target_step() == path.get_start_step())
    PLAJA_ASSERT(path_instance->is_pa_state_aware() == paStateAware) // For convenience.

    const auto* current = path_instance->get_solution(0);
    const StateValues* successor; // NOLINT(*-init-variables)

    /* Check abstract state. */
    PLAJA_ASSERT(solution_checker.check_state_and_abstraction(*current, path.get_pa_source_state()))

    /* Check path. */
    auto* path_instance_ptr = path_instance.get();
    std::unique_ptr<SpuriousnessResult> result(new SpuriousnessResult(*predicates, std::move(path_instance)));

    /* Actual path check. */
    for (auto path_it = path.init_path_iterator(); !path_it.end(); ++path_it) {
        PLAJA_LOG_DEBUG_IF(debug_log, "Path Step.")

        /* Set prefix. */
        result->set_prefix_length(path_it.get_step());

        const auto& action_op = solution_checker.get_action_op(path_it.get_op_id());
        const UpdateIndex_type update_index = path_it.get_update_index();

        /* Check concretization in abstract state (optional). */
        const auto& abstract_state = path_it.get_pa_state();
        if (paStateAware and not abstract_state.is_abstraction(*current)) {
            result->set_spurious(SpuriousnessResult::Concretization);
            add_violated_predicates(*result, abstract_state, *current);
            return result;
        } else if (result->do_add_non_entailed_predicates_externally()) { add_violated_predicates(*result, abstract_state, *current); }
        PLAJA_LOG_DEBUG_IF(debug_log and paStateAware, "Abstract state good.")

        /* Check terminal. */
        if (PLAJA_GLOBAL::enableTerminalStateSupport and solution_checker.check_terminal(*current)) {
            result->set_spurious(SpuriousnessResult::Terminal);
            PLAJA_ASSERT(pathExistenceChecker->get_non_terminal())
            add_relevant_pred_split(*result, *pathExistenceChecker->get_non_terminal(), *current);
        }
        PLAJA_LOG_DEBUG_IF(debug_log and solution_checker.get_terminal(), "Non-terminal state good.")

        /* Check guard. */
        if (not action_op.is_enabled(*current)) { // Op is not enabled.
            result->set_spurious(SpuriousnessResult::GuardApp);
            for (auto it_guard = action_op.guardIterator(); !it_guard.end(); ++it_guard) {
                SpuriousnessChecker::add_relevant_pred_split(*result, *it_guard, *current);
            }
            return result;
        }
        PLAJA_LOG_DEBUG_IF(debug_log, "Guard applicability good.")

        /* Check selection (done in check_path_nn). */

        /* Compute successor. */
        const auto successor_step = path_it.get_successor_step();
        successor = path_instance_ptr->get_solution(successor_step);
        encounteredNonDetUpdate = encounteredNonDetUpdate or action_op.get_update(update_index).is_non_deterministic(0);
        if (successor) {
            PLAJA_ASSERT_EXPECT(false)
            PLAJA_ASSERT(solution_checker.agrees(*current, action_op, update_index, *successor))
        } else {
            path_instance_ptr->set_solution(std::make_unique<StateValues>(solution_checker.compute_successor_solution(*current, action_op, update_index)), successor_step);
            successor = path_instance_ptr->get_solution(successor_step);
        }
        PLAJA_ASSERT(successor)

        /* Set prefix. */
        result->set_prefix_length(path_it.get_successor_step());

        /* Successor to current. */
        current = successor;
    }

    /* Check concretization in abstract state (optional). */
    const auto& abstract_target_state = path.get_pa_target_state();
    if (paStateAware and not abstract_target_state.is_abstraction(*current)) {

        result->set_spurious(SpuriousnessResult::Concretization);
        add_violated_predicates(*result, path.get_pa_target_state(), *current);
        return result;

    } else if (result->do_add_non_entailed_predicates_externally()) { add_violated_predicates(*result, abstract_target_state, *current); }
    PLAJA_LOG_DEBUG_IF(debug_log and paStateAware, "Abstract target state good.")

    /* Check terminal. */
    if (PLAJA_GLOBAL::enableTerminalStateSupport and not PLAJA_GLOBAL::reachMayBeTerminal and solution_checker.check_terminal(*current)) {
        result->set_spurious(SpuriousnessResult::Terminal);
        PLAJA_ASSERT(pathExistenceChecker->get_non_terminal())
        add_relevant_pred_split(*result, *pathExistenceChecker->get_non_terminal(), *current);
    } else if constexpr (PLAJA_GLOBAL::reachMayBeTerminal) { PLAJA_ASSERT_EXPECT(not solution_checker.check_terminal(*current)) }
    PLAJA_LOG_DEBUG_IF(debug_log and solution_checker.get_terminal(), "Non-terminal state good.")


    /* Check target condition. */
    const auto* reach = pathExistenceChecker->get_reach();
    PLAJA_ASSERT(reach)
    if (not reach->evaluate_integer(*current)) {

        PLAJA_ASSERT(not solution_checker.check_state_and_abstraction(*current, path.get_pa_target_state())) // Since we always add target to predicate set.

        /* Spurious since final concrete state is not unsafe -> (add) & backwards propagate reach condition. */
        result->set_spurious(SpuriousnessResult::Target);
        SpuriousnessChecker::add_relevant_pred_split(*result, *reach, *current);
        return result;

    }

    PLAJA_ASSERT(not result->is_spurious())
    return result;

}

std::unique_ptr<SpuriousnessResult> SpuriousnessChecker::check_path_nn(std::unique_ptr<SolutionCheckerInstancePaPath>&& path_instance) const {
    STMT_IF_DEBUG(constexpr bool debug_log = true)
    PLAJA_LOG_DEBUG_IF(debug_log, "Checking path concretization (nn aware) ...")
    // PLAJA_ASSERT(pathExistenceChecker->has_nn())

    const auto& solution_checker = path_instance->get_checker_pa();
    const auto& path = path_instance->get_path();

    PLAJA_ASSERT(path_instance->get_solution(0))
    PLAJA_ASSERT(path_instance->get_solution_size() == path.get_state_path_size())
    PLAJA_ASSERT(path_instance->get_target_step() == path.get_state_path_size() - 1)
    PLAJA_ASSERT(path_instance->get_policy_target_step() == path_instance->get_target_step())
    PLAJA_ASSERT(path_instance->is_pa_state_aware() == paStateAware) // For convenience.

    const auto* current = path_instance->get_solution(0);
    const StateValues* successor; // NOLINT(*-init-variables)

    /* Check abstract state. */
    PLAJA_ASSERT(solution_checker.check_state_and_abstraction(*current, path.get_pa_source_state()));

    /* Check path. */
    auto* path_instance_ptr = path_instance.get();
    std::unique_ptr<SpuriousnessResult> result(new SpuriousnessResult(*predicates, std::move(path_instance)));

    // Actual path check. */
    for (auto path_it = path.init_path_iterator(); !path_it.end(); ++path_it) {
        PLAJA_LOG_DEBUG_IF(debug_log, "Path Step.")

        /* Set prefix. */
        result->set_prefix_length(path_it.get_step());

        const auto& action_op = solution_checker.get_action_op(path_it.get_op_id());

        /* Check concretization in abstract state (optional). */
        PLAJA_ASSERT(not paStateAware or solution_checker.check_state_and_abstraction(*current, path_it.get_pa_state()))

        /* Check terminal. */
        PLAJA_ASSERT(not solution_checker.check_terminal(*current));

        /* Check guard. */
        PLAJA_ASSERT(solution_checker.check_applicability(*current, action_op, nullptr));

        /* Check selection. */
        PLAJA_ASSERT(not path_it.has_witness() or solution_checker.check_policy(*path_it.get_witness(), action_op._action_label(), nullptr)) // Witness should be selected.
        if (not solution_checker.check_policy(*current, action_op._action_label(), nullptr)) {
            /* TODO alternatives: concretization not in (over-approximation) of NN input space for action label / not in predicate state. */
            /* Action is not selected -> for now add (over-approximating) selection condition. */
            result->set_spurious(SpuriousnessResult::ActionSel);

            #ifdef USE_VERITAS
            if (selectionRefinement->needs_interval_witness()) {
                auto box = path_it.get_box_witness();
                if (box.empty()) {
                    box = path_it.get_witness()->to_state_values().get_box();
                }
                // PLACEHOLDER FOR WITNESS REFINEMENT
                selectionRefinement->add_refinement_preds(*result, action_op._action_label(), box, current);
            } else {
                selectionRefinement->add_refinement_preds(*result, action_op._action_label(), path_it.get_witness().get(), current);
            }
            #else
                selectionRefinement->add_refinement_preds(*result, action_op._action_label(), path_it.get_witness().get(), current);
            #endif
            return result;
        }
        PLAJA_LOG_DEBUG_IF(debug_log, "Policy selection good.")

        /* Retrieve successor. */
        PLAJA_ASSERT(path_instance_ptr->get_solution(path_it.get_successor_step()))
        successor = path_instance_ptr->get_solution(path_it.get_successor_step());
        PLAJA_ASSERT(solution_checker.agrees(*current, action_op, path_it.get_update_index(), *successor))

        /* Set prefix. */
        result->set_prefix_length(path_it.get_step() + 1);

        current = successor;
    }

    /* Check concretization in abstract state (optional). */
    PLAJA_ASSERT(not paStateAware or solution_checker.check_state_and_abstraction(*current, path.get_pa_target_state()))

    /* Check terminal. */
    PLAJA_ASSERT(not solution_checker.check_terminal(*current));

    /* Check target condition. */
    STMT_IF_DEBUG(const auto* reach = pathExistenceChecker->get_reach())
    PLAJA_ASSERT(reach)
    PLAJA_ASSERT(reach->evaluate_integer(*current))

    PLAJA_ASSERT(not result->is_spurious())
    return result;

}

std::unique_ptr<SpuriousnessResult> SpuriousnessChecker::check_path(const AbstractPath& path) const {
    encounteredNonDetUpdate = false;

    PLAJA_ASSERT(pathExistenceChecker->check_start(path.get_pa_source_state(), true))

    const auto& initial_state = pathExistenceChecker->get_solution_checker().get_models_initial_values();
    std::unique_ptr<SpuriousnessResult> result_initial_state;
    if (path.get_pa_source_state().is_abstraction(initial_state)) {
        PLAJA_LOG("Checking spuriousness with respect to model's initial state ...")

        /* Check standard spuriousness. */
        auto instance = std::make_unique<SolutionCheckerInstancePaPath>(pathExistenceChecker->get_solution_checker(), path, path.get_target_step(), 0, paStateAware, nullptr, true, pathExistenceChecker->get_reach());
        instance->set_solution(std::make_unique<StateValues>(initial_state), 0);
        result_initial_state = check_path(std::move(instance));

        const bool pa_spurious = result_initial_state->is_spurious();

        /* Check policy spuriousness. */
        if (not pa_spurious) {
            instance = result_initial_state->retrieve_path_instance();
            instance->set_policy_target(path.get_target_step());
            result_initial_state = check_path_nn(std::move(instance));
        }

        if (not result_initial_state->is_spurious()) { return result_initial_state; } // Found concretization.

        /* Further case distinction necessary ... */

        const auto* start = pathExistenceChecker->get_start();

        if (encounteredNonDetUpdate) { // Must use SMT-check, but only if initial state is not subsumed by start state anyway.

            if (not start or not start->evaluate_integer(initial_state)) {

                if (pa_spurious) {
                    result_initial_state = pathExistenceChecker->check_incrementally(std::make_unique<SolutionCheckerInstancePaPath>(pathExistenceChecker->get_solution_checker(), path, path.get_target_step(), 0, paStateAware, nullptr, true, pathExistenceChecker->get_reach()));
                }

                if (not result_initial_state->is_spurious() and pathExistenceChecker->has_nn()) {

                    PLAJA_ASSERT(result_initial_state->get_path_instance().check())

                    instance = result_initial_state->retrieve_path_instance();
                    instance->set_policy_target(path.get_target_step());
                    result_initial_state = pathExistenceChecker->check_policy_incrementally(std::move(instance), *selectionRefinement);

                }

            }

        }

        if (not start or not pathExistenceChecker->check_start(path.get_pa_source_state(), false)) { return result_initial_state; }

    }

    PLAJA_LOG("Checking spuriousness with respect to start condition ...")

    PLAJA_ASSERT(pathExistenceChecker->get_start())
    PLAJA_ASSERT(pathExistenceChecker->check_start(path.get_pa_source_state(), false))

    auto result = pathExistenceChecker->check_incrementally(std::make_unique<SolutionCheckerInstancePaPath>(pathExistenceChecker->get_solution_checker(), path, path.get_target_step(), 0, paStateAware, pathExistenceChecker->get_start(), false, pathExistenceChecker->get_reach()));
    #ifdef USE_VERITAS
    if (result->is_spurious() or not (pathExistenceChecker->has_nn() or pathExistenceChecker->has_ensemble())) { return result; }
    #else
    if (result->is_spurious() or not (pathExistenceChecker->has_nn())) { return result; }
    #endif
    else {

        // TODO alternatives

        /* (option): (Incrementally) check NN-aware path existence. */
        if (checkPolicySpuriousness or encounteredNonDetUpdate) {

            /* When encountering non-det. updates, we need SMT_check for completeness. */
            STMT_IF_DEBUG(bool print(true);)
            PLAJA_LOG_DEBUG_IF(not checkPolicySpuriousness and print, PLAJA_UTILS::to_red_string("Warning: Checking policy spuriousness due to non-deterministic update."))
            STMT_IF_DEBUG(print = false;)

            PLAJA_LOG("Checking policy spuriousness ...")

            /* (option) Refine with respect to longest (policy-aware) concretization possible. */
            auto instance = result->retrieve_path_instance();
            instance->set_policy_target(path.get_target_step());
            auto result_policy = pathExistenceChecker->check_policy_incrementally(std::move(instance), *selectionRefinement);
            PLAJA_LOG_IF(result_policy->is_spurious(), "Spurious under policy.")

            return result_policy;

        } else {

            /* (option): Sample source state (from standard PA paths) and refine with respect to its violation (option: including utilization of over-app selection condition). */

            auto instance = result->retrieve_path_instance();
            PLAJA_ASSERT(instance)
            PLAJA_ASSERT(not instance->is_invalidated())
            instance->set_policy_target(path.get_target_step());
            return check_path_nn(std::move(instance));

        }

        /* (option) Compute WP of (over-approximating) selection condition on path (prefix). */

        /* (naive option) Enumerate all standard PA path start states (and refine). */

    }
}
