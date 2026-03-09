//
// This file is part of the PlaJA code base.
// Copyright (c) (2023 - 2024) Chaahat Jain.
//

#pragma GCC diagnostic push
#pragma ide diagnostic ignored "Simplify"

#include <memory>
#include "ensemble_sat_checker.h"
#include "../../../../include/compiler_config_const.h"
#include "../../../../globals.h"
#include "../../../factories/configuration.h"
#include "../../../information/jani2ensemble/jani_2_ensemble.h"
#include "../../../smt_ensemble/model/model_veritas.h"
#include "../../../smt_ensemble/solver/solver_veritas.h"
#include "../../../smt_ensemble/solver/solution_veritas.h"
#include "../../../smt_ensemble/solver/solution_check_wrapper_veritas.h"
#include "../../../states/state_values.h"
#include "../../pa_states/abstract_state.h"
#include "../../solution_checker_pa.h"
#include "../../solution_checker_instance_pa.h"
#include "../smt/action_op_veritas_pa.h"
#include "../smt/model_veritas_pa.h"

#ifdef USE_ADVERSARIAL_ATTACK
#include "../../../fd_adaptions/timer.h"
#include "../../../../adversarial_attack/adversarial_attack.h"
#endif

#ifdef ENABLE_QUERY_CACHING
#include "query_to_json.h"
#endif

/**********************************************************************************************************************/

EnsembleSatChecker::EnsembleSatChecker(const Jani2Ensemble& jani_2_ensemble, const PLAJA::Configuration& config)
    : 
    SatChecker(jani_2_ensemble, config), 
    modelVeritas(nullptr)
    , jani2Ensemble(&jani_2_ensemble)
    , solver(nullptr)
#ifdef USE_ADVERSARIAL_ATTACK
    , attackSolutionGuess(config.is_flag_set(PLAJA_OPTION::attack_solution_guess))
#endif
    , set_action_op(nullptr)
#ifdef ENABLE_QUERY_CACHING
, queryToJson(config.has_value_option(PLAJA_OPTION::query_cache) ? new QueryToJson(jani_2_ensemble) : nullptr)
#endif
{
    //

    const bool use_shared = config.has_sharable(PLAJA::SharableKey::MODEL_VERITAS);
    if (not use_shared) { config.set_sharable<ModelVeritas>(PLAJA::SharableKey::MODEL_VERITAS, std::make_shared<ModelVeritasPA>(config)); }
    modelVeritas = std::static_pointer_cast<ModelVeritasPA>(config.get_sharable<ModelVeritas>(PLAJA::SharableKey::MODEL_VERITAS));
    modelVeritas->increment(); // in case predicates have been added after construction
    modelVeritas->add_applicability_trees();
    solver = VERITAS_IN_PLAJA::ENSEMBLE_SOLVER::construct(config, *modelVeritas->make_query(true, true, 0));
    solver->set_model_veritas(modelVeritas);
    solver->set_jani_ensemble(jani2Ensemble);
    PLAJA_LOG_DEBUG_IF(not sharedStatistics, PLAJA_UTILS::to_red_string("Ensemble-sat-checker without stats."))
}

EnsembleSatChecker::~EnsembleSatChecker() = default;

void EnsembleSatChecker::set_op_applicability_cache(std::shared_ptr<OpApplicability>& op_app) {
    PLAJA_ASSERT(not modelVeritas->get_op_applicability())
    modelVeritas->share_op_applicability(op_app);
}

void EnsembleSatChecker::increment() { modelVeritas->increment(); }

void EnsembleSatChecker::push() { solver->push(); }

void EnsembleSatChecker::pop() { solver->pop(); }


void EnsembleSatChecker::set_operator(ActionOpID_type action_op_id) {
    set_action_op_id = action_op_id;
    set_action_op = &modelVeritas->get_action_op_pa(action_op_id);
    PLAJA_ASSERT(set_output_index == jani2Ensemble->get_output_index(set_action_op->_action_label()))
}

void EnsembleSatChecker::add_source_state(const AbstractState& source) {
    set_source_state = source.to_ptr();
    modelVeritas->add_to(source, *solver);
    // solver->_context().dump(); 
}

bool EnsembleSatChecker::add_guard() {
    PLAJA_ASSERT(set_action_op)
    // solver->_context().dump();
    return set_action_op->add_guard(*solver, false, 0);
}

bool EnsembleSatChecker::add_guard_negation() {
    PLAJA_ASSERT(set_action_op)
    // solver->_context().dump();
    PLAJA_ASSERT(false)
    return set_action_op->add_guard_negation(*solver, false, 0);
}

void EnsembleSatChecker::add_update() {
    PLAJA_ASSERT(set_action_op)
    PLAJA_ASSERT(set_update_index != ACTION::noneUpdate)
    // solver->_context().dump();
    set_action_op->add_update(*solver, set_update_index, false, false);
}

void EnsembleSatChecker::add_target_state(const AbstractState& target) {
    set_target_state = target.to_ptr();
    PLAJA_ASSERT(set_target_state->has_entailment_information() == target.has_entailment_information())

    PLAJA_ASSERT(set_action_op)
    set_action_op->add_target_predicate_state(target, *solver, set_update_index);
    // solver->_context().dump();

}


bool EnsembleSatChecker::add_output_interface() {
    // modelVeritas->add_output_interface(*solver, set_action_label);
    return true;
}

/***check version*******************************************************************************************************/

void EnsembleSatChecker::save_solution_if(bool rlt) {
    /* Unset solution if unsat. */
    if (not rlt) {
        lastSolution = nullptr;
        box.clear();
        return;
    }

    const bool has_instance = solver->get_solution_checker();

    if (not has_instance) { // Always extract solution via checker instance.
        // const AbstractState& pa_source = *(set_source_state.get());
        auto test = std::make_unique<SolutionCheckerInstancePa>(*solutionChecker, set_source_state.get(), set_action_label, set_action_op_id, set_update_index, set_target_state.get());
        solver->set_solution_checker(*modelVeritas, std::unique_ptr<SolutionCheckerInstance>(std::move(test)));
    }

    lastSolution = solver->extract_solution_via_checker();
    box = extract_box();
    PLAJA_ASSERT(not lastSolution or lastSolution->get_solution(0))
    PLAJA_ASSERT(not lastSolution or not set_target_state or lastSolution->get_solution(1))
   
    if (not has_instance) { solver->unset_solution_checker(); }

}

bool EnsembleSatChecker::check_() {

    if constexpr (PLAJA_GLOBAL::roundRelaxedSolutions) {
        solver->set_solution_checker(*modelVeritas, std::make_unique<SolutionCheckerInstancePa>(*solutionChecker, set_source_state.get(), set_action_label, set_action_op_id, set_update_index, set_target_state.get()));
    }

    #ifdef ENABLE_APPLICABILITY_FILTERING
        PLAJA_ASSERT(modelVeritas->get_interface())
        if (modelVeritas->get_interface()->_do_applicability_filtering()) {
            auto trees = modelVeritas->get_applicability_filter(set_action_label);
            PLAJA_ASSERT(trees.num_leaf_values() > 0);
            solver->set_app_filter_trees(trees);
            solver->set_use_filter(true);
        }
    #endif

    const bool rlt = solver->check(set_action_label);

    if constexpr (PLAJA_GLOBAL::reuseSolutions) {

        save_solution_if(rlt);

        // We expect floating errors in the NN but not the transition structure.
        STMT_IF_DEBUG(PLAJA_GLOBAL::push_additional_outputs_frame(true);)
        PLAJA_ASSERT(not rlt or not lastSolution or solutionChecker->check(*lastSolution->get_solution(0), set_source_state.get(), set_action_label, set_action_op_id, set_update_index, set_target_state.get(), lastSolution->get_solution(1)) or not solutionChecker->check_policy(*lastSolution->get_solution(0), set_action_label, nullptr))
        STMT_IF_DEBUG(PLAJA_GLOBAL::pop_additional_outputs_frame();)
    }

    if constexpr (PLAJA_GLOBAL::roundRelaxedSolutions) { solver->unset_solution_checker(); }

    solver->reset();

    return rlt;

}

bool EnsembleSatChecker::check_relaxed() {

    const bool rlt = solver->check(set_action_label);

    if constexpr (PLAJA_GLOBAL::reuseSolutions) { save_solution_if(rlt); }

    solver->reset();

    return rlt;
}

#ifdef ENABLE_TERMINAL_STATE_SUPPORT

void EnsembleSatChecker::add_non_terminal_condition() { modelVeritas->exclude_terminal(*solver, 0); }

#endif

#ifdef ENABLE_STALLING_DETECTION
void EnsembleSatChecker::stalling_detection(const State& source) {
    PLAJA_ASSERT(pushedInputQueries.size() == 1)
    std::size_t num_outputs = jani2NNet.get_num_output_features();
    for (OutputIndex_type output_index = 0; output_index < num_outputs; ++output_index) {
        set_output(jani2NNet.get_output_label(output_index));
        push();
        add_source_state(source);
        add_output_interface();
        if (actionOpToConstraint[output_index]->is_guard_set_enabled()) { // if *not* applicable, stalling comes down to choosing the action independent of the guard
            add_guard_negation();
            // false is returned if guard negation is trivially unsat
            // -> modulo location guard: NN cannot stall choosing the current action
        }
        // actual test
        if (sharedStatistics) sharedStatistics->inc_nn_sat_stalling_tests();
        if (check(true)) {
            sharedStatistics->inc_nn_sat_stalling_tests_sat();
        }

        pop();
    }
}
#endif

#ifdef USE_ADVERSARIAL_ATTACK

bool EnsembleSatChecker::adversarial_attack() {
    PLAJA_ASSERT(adversarialAttack)
    PLAJA_ASSERT(!pushedInputQueries.empty())
    const auto& input_query = pushedInputQueries.back();
    if (adversarialAttack->check(input_query, set_output_index, PLAJA_GLOBAL::reuseSolutions and attackSolutionGuess ? &solutionChecker->get_last_solution() : nullptr, stateIndexesToVeritas.get())) {
        if (PLAJA_GLOBAL::optionParser->is_flag_set(PLAJA_OPTION::ignore_attack_constraints) && !solutionChecker->check(*lastSolution, set_target_state.get())) {
            lastSolution = nullptr;
            return false;
        } else { PLAJA_ASSERT(solutionChecker->check(*lastSolution, set_target_state.get())) }
        if constexpr(PLAJA_GLOBAL::reuseSolutions) {
            lastSolution = std::make_unique<StateValues>(solutionChecker->get_solution_constructor());
            adversarialAttack->extract_solution(*lastSolution, input_query, *stateIndexesToVeritas);
            lastSolutionIsExact = true;
        }
#if 0 // always maintain own solution checker
        else if (solutionEnumerator) {
            auto last_solution = std::make_unique<StateValues>(solutionChecker->get_solution_constructor());
            adversarialAttack->extract_solution(*last_solution, input_query, *stateIndexesToVeritas);
            // set enumerator
            PLAJA_ASSERT(set_source_state)
            PLAJA_ASSERT(set_output_index != NEURON_INDEX::none)
            solutionEnumerator->set_abstract_source(*set_source_state);
            solutionEnumerator->set_label(jani2NNet.get_output_label(set_output_index));
            if (set_action_op) { solutionEnumerator->set_operator(set_action_op->_op_id()); }
            solutionEnumerator->set_update(set_update_index);
            // check
            auto rlt = !solutionEnumerator->check(*last_solution, set_target_state.get());
            if (!rlt) { return false; }
            solutionEnumerator->unset(); // unset enumerator
        }
#endif
        return true;
    } else { return false; }
}

#endif

/***********************************************************************************************************************/


#pragma GCC diagnostic pop
