//
// This file is part of the PlaJA code base.
// Copyright (c) (2019 - 2020) Marcel Vinzent.
// See README.md in the top-level directory for licensing information.
//

#pragma GCC diagnostic push
#pragma ide diagnostic ignored "Simplify"

#include <memory>
#include "nn_sat_checker.h"
#include "../../../../include/compiler_config_const.h"
#include "../../../../globals.h"
#include "../../../factories/configuration.h"
#include "../../../information/jani2nnet/jani_2_nnet.h"
#include "../../../smt_nn/model/model_marabou.h"
#include "../../../smt_nn/solver/smt_solver_marabou.h"
#include "../../../smt_nn/solver/solution_marabou.h"
#include "../../../smt_nn/solver/solution_check_wrapper_marabou.h"
#include "../../../states/state_values.h"
#include "../../pa_states/abstract_state.h"
#include "../../solution_checker_pa.h"
#include "../../solution_checker_instance_pa.h"
#include "../smt/action_op_marabou_pa.h"
#include "../smt/model_marabou_pa.h"

#ifdef USE_ADVERSARIAL_ATTACK
#include "../../../fd_adaptions/timer.h"
#include "../../../../adversarial_attack/adversarial_attack.h"
#endif

#ifdef ENABLE_QUERY_CACHING
#include "query_to_json.h"
#endif

// extern
namespace PLAJA_OPTION::SMT_SOLVER_MARABOU { extern const std::unordered_map<std::string, MILPSolverBoundTighteningType> stringToBoundTightening; }
namespace PLAJA_OPTION {
    extern const std::string per_src_tightening;
    extern const std::string per_src_label_tightening;
}

/**********************************************************************************************************************/

NNSatChecker::NNSatChecker(const Jani2NNet& jani_2_nnet, const PLAJA::Configuration& config)
    : 
    SatChecker(jani_2_nnet, config), 
    modelMarabou(nullptr)
    , jani2NNet(&jani_2_nnet)
    , solverMarabou(nullptr)
    , perSrcBounds(config.get_option(PLAJA_OPTION::SMT_SOLVER_MARABOU::stringToBoundTightening, PLAJA_OPTION::per_src_tightening))
    , perSrcLabelBounds(config.get_option(PLAJA_OPTION::SMT_SOLVER_MARABOU::stringToBoundTightening, PLAJA_OPTION::per_src_label_tightening))
#ifdef USE_ADVERSARIAL_ATTACK
    , attackSolutionGuess(config.is_flag_set(PLAJA_OPTION::attack_solution_guess))
#endif
    , set_action_op(nullptr)
#ifdef ENABLE_QUERY_CACHING
, queryToJson(config.has_value_option(PLAJA_OPTION::query_cache) ? new QueryToJson(jani_2_nnet) : nullptr)
#endif
{
    //

    const bool use_shared = config.has_sharable(PLAJA::SharableKey::MODEL_MARABOU);
    if (not use_shared) { config.set_sharable<ModelMarabou>(PLAJA::SharableKey::MODEL_MARABOU, std::make_shared<ModelMarabouPA>(config)); }
    modelMarabou = PLAJA_UTILS::cast_shared<ModelMarabouPA>(config.get_sharable<ModelMarabou>(PLAJA::SharableKey::MODEL_MARABOU));
    modelMarabou->increment(); // in case predicates have been added after construction
    solverMarabou = MARABOU_IN_PLAJA::SMT_SOLVER::construct(config, modelMarabou->make_query(true, true, 0));

#ifdef USE_ADVERSARIAL_ATTACK
    if (PLAJA_GLOBAL::optionParser->is_flag_set(PLAJA_OPTION::adversarial_attack)) {
        adversarialAttack = std::make_unique<AdversarialAttack>(jani2NNet, baseInputQuery.getNumberOfVariables() - jani2NNet.count_neurons() - jani2NNet.count_hidden_neurons());
        adversarialAttack->set_statistics(shared_statistics);
    }
#endif

    PLAJA_LOG_DEBUG_IF(not sharedStatistics, PLAJA_UTILS::to_red_string("NN-sat-checker without stats."))
}

NNSatChecker::~NNSatChecker() = default;

void NNSatChecker::set_op_applicability_cache(std::shared_ptr<OpApplicability>& op_app) {
    PLAJA_ASSERT(not modelMarabou->get_op_applicability())
    modelMarabou->share_op_applicability(op_app);
}

void NNSatChecker::increment() { modelMarabou->increment(); }

void NNSatChecker::push() { solverMarabou->push(); }

void NNSatChecker::pop() { solverMarabou->pop(); }


void NNSatChecker::set_operator(ActionOpID_type action_op_id) {
    set_action_op_id = action_op_id;
    set_action_op = &modelMarabou->get_action_op_pa(action_op_id);
    PLAJA_ASSERT(set_output_index == jani2NNet->get_output_index(set_action_op->_action_label()))
}

void NNSatChecker::add_source_state(const AbstractState& source) {
    set_source_state = source.to_ptr();
    modelMarabou->add_to(source, *solverMarabou);
    if (perSrcBounds != MILPSolverBoundTighteningType::NONE) { modelMarabou->lp_tightening(*solverMarabou, perSrcBounds); }
}

#ifdef ENABLE_TERMINAL_STATE_SUPPORT

void NNSatChecker::add_non_terminal_condition() { modelMarabou->exclude_terminal(*solverMarabou, 0); }

#endif

bool NNSatChecker::add_guard() {
    PLAJA_ASSERT(set_action_op)
    return set_action_op->add_guard(*solverMarabou, false, 0);
}

bool NNSatChecker::add_guard_negation() {
    PLAJA_ASSERT(set_action_op)
    return set_action_op->add_guard_negation(*solverMarabou, false, 0);
}

void NNSatChecker::add_update() {
    PLAJA_ASSERT(set_action_op)
    PLAJA_ASSERT(set_update_index != ACTION::noneUpdate)
    set_action_op->add_update(*solverMarabou, set_update_index, false, false);
}

void NNSatChecker::add_target_state(const AbstractState& target) {
    set_target_state = target.to_ptr();
    PLAJA_ASSERT(set_target_state->has_entailment_information() == target.has_entailment_information())

    PLAJA_ASSERT(set_action_op)
    set_action_op->add_target_predicate_state(target, *solverMarabou, set_update_index);
}


bool NNSatChecker::add_output_interface() {
    modelMarabou->add_output_interface(*solverMarabou, set_action_label);
    if (perSrcLabelBounds != MILPSolverBoundTighteningType::NONE) { return modelMarabou->lp_tightening_output(*solverMarabou, perSrcLabelBounds); }
    return true;
}

/***check version*******************************************************************************************************/

void NNSatChecker::save_solution_if(bool rlt) {

    /* Unset solution if unsat. */
    if (not rlt) {
        lastSolution = nullptr;
        return;
    }

    const bool has_instance = solverMarabou->get_solution_checker();

    if (not has_instance) { // Always extract solution via checker instance.
        solverMarabou->set_solution_checker(*modelMarabou, std::make_unique<SolutionCheckerInstancePa>(*solutionChecker, set_source_state.get(), set_action_label, set_action_op_id, set_update_index, set_target_state.get()));
    }

    lastSolution = solverMarabou->extract_solution_via_checker();
    PLAJA_ASSERT(not lastSolution or lastSolution->get_solution(0))
    PLAJA_ASSERT(not lastSolution or not set_target_state or lastSolution->get_solution(1))

    /* Alt (deprecated due to extraction of target solution): */
    /* lastSolution = std::make_unique<StateValues>(solutionChecker->get_solution_constructor(*set_source_state));
    modelMarabou->get_src_in_marabou().extract_solution(solverMarabou->extract_solution(), *lastSolution); */

    if (not has_instance) { solverMarabou->unset_solution_checker(); }

}

bool NNSatChecker::check_() {

    if constexpr (PLAJA_GLOBAL::roundRelaxedSolutions) {
        solverMarabou->set_solution_checker(*modelMarabou, std::make_unique<SolutionCheckerInstancePa>(*solutionChecker, set_source_state.get(), set_action_label, set_action_op_id, set_update_index, set_target_state.get()));
    }

    const bool rlt = solverMarabou->check();

    if constexpr (PLAJA_GLOBAL::reuseSolutions) {

        save_solution_if(rlt);

        // We expect floating errors in the NN but not the transition structure.
        STMT_IF_DEBUG(PLAJA_GLOBAL::push_additional_outputs_frame(true);)
        PLAJA_ASSERT(not rlt or not solverMarabou->is_exact() or not lastSolution or solutionChecker->check(*lastSolution->get_solution(0), set_source_state.get(), set_action_label, set_action_op_id, set_update_index, set_target_state.get(), lastSolution->get_solution(1)) or not solutionChecker->check_policy(*lastSolution->get_solution(0), set_action_label, nullptr))
        STMT_IF_DEBUG(PLAJA_GLOBAL::pop_additional_outputs_frame();)
    }

    if constexpr (PLAJA_GLOBAL::roundRelaxedSolutions) { solverMarabou->unset_solution_checker(); }

    solverMarabou->reset();

    return rlt;

}

bool NNSatChecker::check_relaxed() {

    const bool rlt = solverMarabou->check_relaxed();

    if constexpr (PLAJA_GLOBAL::reuseSolutions) { save_solution_if(rlt); }

    solverMarabou->reset();

    return rlt;
}


#ifdef ENABLE_STALLING_DETECTION
void NNSatChecker::stalling_detection(const State& source) {
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

bool NNSatChecker::adversarial_attack() {
    PLAJA_ASSERT(adversarialAttack)
    PLAJA_ASSERT(!pushedInputQueries.empty())
    const auto& input_query = pushedInputQueries.back();
    if (adversarialAttack->check(input_query, set_output_index, PLAJA_GLOBAL::reuseSolutions and attackSolutionGuess ? &solutionChecker->get_last_solution() : nullptr, stateIndexesToMarabou.get())) {
        if (PLAJA_GLOBAL::optionParser->is_flag_set(PLAJA_OPTION::ignore_attack_constraints) && !solutionChecker->check(*lastSolution, set_target_state.get())) {
            lastSolution = nullptr;
            return false;
        } else { PLAJA_ASSERT(solutionChecker->check(*lastSolution, set_target_state.get())) }
        if constexpr(PLAJA_GLOBAL::reuseSolutions) {
            lastSolution = std::make_unique<StateValues>(solutionChecker->get_solution_constructor());
            adversarialAttack->extract_solution(*lastSolution, input_query, *stateIndexesToMarabou);
            lastSolutionIsExact = true;
        }
#if 0 // always maintain own solution checker
        else if (solutionEnumerator) {
            auto last_solution = std::make_unique<StateValues>(solutionChecker->get_solution_constructor());
            adversarialAttack->extract_solution(*last_solution, input_query, *stateIndexesToMarabou);
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
