//
// This file is part of the PlaJA code base (2019 - 2020).
//

#pragma GCC diagnostic push
#pragma ide diagnostic ignored "Simplify"

#include <memory>
#include "sat_checker.h"
#include "../../include/compiler_config_const.h"
#include "../../globals.h"
#include "../factories/configuration.h"
#include "../information/jani_2_interface.h"
#include "../states/state_values.h"
#include "pa_states/abstract_state.h"
#include "solution_checker_pa.h"
#include "solution_checker_instance_pa.h"

#ifdef USE_ADVERSARIAL_ATTACK
#include "../../../fd_adaptions/timer.h"
#include "../../../../adversarial_attack/adversarial_attack.h"
#endif

#ifdef ENABLE_QUERY_CACHING
#include "query_to_json.h"
#endif

/**********************************************************************************************************************/

SatChecker::SatChecker(const Jani2Interface& jani_2_interface, const PLAJA::Configuration& config)
    :
    jani2Interface(&jani_2_interface)
    , solutionChecker(nullptr)
#ifdef USE_ADVERSARIAL_ATTACK
    , attackSolutionGuess(config.is_flag_set(PLAJA_OPTION::attack_solution_guess))
#endif
    , sharedStatistics(config.get_sharable_ptr<PLAJA::StatsBase>(PLAJA::SharableKey::STATS))
    , set_action_label(ACTION::noneLabel)
    , set_output_index(NEURON_INDEX::none)
    , set_action_op_id(ACTION::noneOp)
    , set_update_index(ACTION::noneUpdate)
    , set_source_state(nullptr)
    , set_target_state(nullptr)
    , lastSolution(nullptr)
#ifdef ENABLE_QUERY_CACHING
, queryToJson(config.has_value_option(PLAJA_OPTION::query_cache) ? new QueryToJson(jani_2_nnet) : nullptr)
#endif
{
    PLAJA_LOG_DEBUG_IF(not sharedStatistics, PLAJA_UTILS::to_red_string("Sat-checker without stats."))
}

SatChecker::~SatChecker() = default;


void SatChecker::set_label(ActionLabel_type action_label) {
    PLAJA_ASSERT(is_action_selectable(action_label))
    set_action_label = action_label;
    set_output_index = jani2Interface->get_output_index(action_label);
}

void SatChecker::unset_label() {
    set_action_label = ACTION::noneLabel;
    set_output_index = NEURON_INDEX::none;
}

void SatChecker::unset_operator() {
    set_action_op_id = ACTION::noneOp;
}

void SatChecker::set_update(UpdateIndex_type update_index) {
    set_update_index = update_index;
}

void SatChecker::unset_update() {
    set_update_index = ACTION::noneUpdate;
}

void SatChecker::unset_source_state() { set_source_state = nullptr; }

void SatChecker::unset_target_state() { set_target_state = nullptr; }

bool SatChecker::is_action_selectable(ActionLabel_type action_label) const { return jani2Interface->is_learned(action_label); }

const StateValues& SatChecker::get_last_solution() const {
    PLAJA_ASSERT(has_solution() and lastSolution->get_solution(0))
    return *lastSolution->get_solution(0);
}

std::unique_ptr<StateValues> SatChecker::release_solution() {
    PLAJA_ASSERT(has_solution() and lastSolution->get_solution(0))
    return lastSolution->retrieve_solution(0);
}

const StateValues* SatChecker::get_target_solution() const {
    PLAJA_ASSERT(has_solution())
    if (lastSolution->get_solution_size() > 1) {
        PLAJA_ASSERT(lastSolution->get_solution_size() == 2)
        return lastSolution->get_solution(1);
    } else {
        return nullptr;
    }
}

std::unique_ptr<StateValues> SatChecker::release_target_solution() {
    PLAJA_ASSERT(has_solution() and lastSolution->get_solution(1))
    return lastSolution->retrieve_solution(1);
}

/***check version*******************************************************************************************************/


bool SatChecker::check() {

    // unset exact solution if set:
    if constexpr (PLAJA_GLOBAL::reuseSolutions) { lastSolution = nullptr; }

#ifdef USE_ADVERSARIAL_ATTACK
    if (adversarialAttack) {
        auto time_offset = (*PLAJA_GLOBAL::timer)();
        bool rlt = adversarial_attack();
        auto attack_time = (*PLAJA_GLOBAL::timer)() - time_offset;
        if (sharedStatistics) {
            sharedStatistics->inc_attr_double(PLAJA::StatsDouble::TIME_AD_ATTACKS);
            if (rlt) { sharedStatistics->inc_attr_double(PLAJA::StatsDouble::TIME_AD_ATTACKS_SUC); }
        }
        if (rlt) { return true; }
    }
    // if (adversarialAttack && adversarial_attack()) { return true; }
#endif

    const bool rlt = check_();

#ifdef ENABLE_QUERY_CACHING
    if (queryToJson) { queryToJson->cache_query(pushedInputQueries.back(), set_output_index, set_action_op, set_update_index, &rlt); }
#endif

    return rlt;
}

#ifdef ENABLE_STALLING_DETECTION
void SatChecker::stalling_detection(const State& source) {
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

bool SatChecker::adversarial_attack() {
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

void SatChecker::print_statistics() {
#ifdef ENABLE_QUERY_CACHING
    if (queryToJson) { queryToJson->dump_cached_queries(); }
#endif
}

#pragma GCC diagnostic pop
