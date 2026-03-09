//
// This file is part of the PlaJA code base.
// Copyright (c) (2019 - 2021) Marcel Vinzent.
// See README.md in the top-level directory for licensing information.
//

#pragma GCC diagnostic push
#pragma ide diagnostic ignored "Simplify"

#include "initial_states_enumerator_pa.h"
#include "../../../parser/ast/expression/non_standard/location_value_expression.h"
#include "../../../parser/ast/expression/non_standard/state_condition_expression.h"
#include "../../../parser/visitor/extern/to_normalform.h"
#include "../../../option_parser/option_parser.h"
#include "../../factories/predicate_abstraction/pa_options.h"
#include "../../factories/configuration.h"
#include "../../non_prob_search/initial_states_enumerator.h"
#include "../../states/state_values.h"
#include "../../smt/solver/smt_solver_z3.h"
#include "../match_tree/predicate_traverser.h"
#include "../optimization/predicate_relations.h"
#include "../pa_states/abstract_state.h"
#include "../pa_states/predicate_state.h"
#include "../smt/model_z3_pa.h"
#include "search_space_pa_base.h"

InitialStatesEnumeratorPa::InitialStatesEnumeratorPa(const PLAJA::Configuration& config, const SearchSpacePABase& parent, const Expression* condition_ptr):
    parent(&parent)
    , modelZ3(config.get_sharable_cast<ModelZ3, ModelZ3PA>(PLAJA::SharableKey::MODEL_Z3).get())
    , conditionPtr(condition_ptr)
    // , stateCondition(nullptr)
    , explicitEnumerator(nullptr)
    , predicateRelations(nullptr)
    , numLocations(modelZ3->get_number_automata_instances())
    , numPredicates(modelZ3->get_number_predicates()) {

    PLAJA_ASSERT(modelZ3)

    if (not conditionPtr) { conditionPtr = modelZ3->get_start(); }

    if (not conditionPtr) { return; } // Nothing to be done.

    if (TO_NORMALFORM::is_states_values(*conditionPtr) or config.is_flag_set(PLAJA_OPTION::explicit_initial_state_enum)) {
        explicitEnumerator = std::make_unique<InitialStatesEnumerator>(config, *conditionPtr);
        return;
    }

    auto stateCondition = StateConditionExpression::transform(conditionPtr->deepCopy_Exp());

    /* Prepare constructor. */
    PLAJA_ASSERT(numLocations == stateCondition->get_size_loc_values()) // Complete location value assignment.
    locationState = std::make_unique<StateValues>(numLocations);
    for (auto it = stateCondition->init_loc_value_iterator(); !it.end(); ++it) { locationState->assign_int<true>(it->get_location_index(), it->get_location_value()); }

    /* Prepare solver. */
    solver = std::make_unique<Z3_IN_PLAJA::SMTSolver>(modelZ3->share_context(), config.get_sharable_ptr<PLAJA::StatsBase>(PLAJA::SharableKey::STATS));
    modelZ3->add_src_bounds(*solver, true);
    const auto* start_constraint = stateCondition->get_constraint();
    // add only required variable bounds (not supported in incremental mode)
    // for (PredicateIndex_type pred_index = 0; pred_index < num_predicates; ++pred_index) { solver->add_unregistered_variables(modelZ3.get_predicate(pred_index)); }
    // if (state_var_constraint) { solver->add_unregistered_variables(state_var_constraint); }
    // solver->unregister_all_vars(); // here we do not need the registry infrastructure in the first place
    if (start_constraint) { modelZ3->add_expression(*solver, *start_constraint, 0); }

    /* Set predicate relations (with ground truth). */
    if constexpr (true) {
        if (config.get_bool_option(PLAJA_OPTION::predicate_relations)) { predicateRelations = std::make_unique<PredicateRelations>(*modelZ3, *solver); }
    } else {
        if (config.get_bool_option(PLAJA_OPTION::predicate_relations)) { predicateRelations = std::make_unique<PredicateRelations>(modelZ3, stateCondition); }
    }

}

InitialStatesEnumeratorPa::~InitialStatesEnumeratorPa() = default;

void InitialStatesEnumeratorPa::increment() {
    if (do_enumerate_explicitly()) { return; } // Nothing to be done.

    PLAJA_ASSERT(locationState->size() == numLocations)
    PLAJA_ASSERT((PLAJA_GLOBAL::lazyPA and numPredicates == modelZ3->get_number_predicates()) or numPredicates < modelZ3->get_number_predicates()) // Incremental usage.
    numPredicates = modelZ3->get_number_predicates();

    if (predicateRelations) { predicateRelations->increment(); }
}

void InitialStatesEnumeratorPa::reset_smt() { solver->reset(); }

// TODO in incremental mode: might skip one check if the other check returned unsat.
void InitialStatesEnumeratorPa::enumerate(PredicateState& pred_state, PredicateTraverser& predicate_traverser) { // NOLINT(misc-no-recursion)

    if (predicate_traverser.end()) {

        if (not PLAJA_GLOBAL::queryPerBranchInit) {
            solver->push();
            /* Add predicates. */
            for (PredicateIndex_type pred_index = 0; pred_index < numPredicates; ++pred_index) {
                PLAJA_ASSERT(pred_state.is_defined(pred_index))
                if (pred_state.predicate_value(pred_index)) { solver->add(modelZ3->get_src_predicate(pred_index)); }
                else { solver->add(!modelZ3->get_src_predicate(pred_index)); }
            }
            /* */
            if (not solver->check_pop()) { return; }
        }

        PLAJA_ASSERT(not PLAJA_GLOBAL::queryPerBranchInit or solver->check())

        /* Add to initial states. */
        paStates.push_back(pred_state.to_pa_state_values(true));

    } else {

        const auto tmp_pred_index = predicate_traverser.predicate_index();

        if (pred_state.is_defined(tmp_pred_index)) {

            if (not pred_state.is_set(tmp_pred_index)) { pred_state.set(tmp_pred_index, pred_state.predicate_value(tmp_pred_index)); }

            predicate_traverser.next(pred_state);

            enumerate(pred_state, predicate_traverser);

            predicate_traverser.previous();

        } else {
            PLAJA_ASSERT(pred_state.is_undefined(tmp_pred_index))

            /* First branch */
            if constexpr (PLAJA_GLOBAL::queryPerBranchInit) {
                solver->push();
                solver->add(modelZ3->get_src_predicate(tmp_pred_index));
            }

            if (not PLAJA_GLOBAL::queryPerBranchInit or solver->check()) {

                pred_state.push_layer();
                pred_state.set(tmp_pred_index, true);
                if (predicateRelations) { predicateRelations->fix_predicate_state_asc_if_non_lazy(pred_state, tmp_pred_index); }

                predicate_traverser.next(pred_state);

                enumerate(pred_state, predicate_traverser);

                predicate_traverser.previous();
                pred_state.pop_layer();

            }

            if constexpr (PLAJA_GLOBAL::queryPerBranchInit) { solver->pop(); }
            /* End first branch */

            /* Second branch */
            if constexpr (PLAJA_GLOBAL::queryPerBranchInit) {
                solver->push();
                solver->add(!modelZ3->get_src_predicate(tmp_pred_index));
            }

            if (not PLAJA_GLOBAL::queryPerBranchInit or solver->check()) {

                pred_state.push_layer();
                pred_state.set(tmp_pred_index, false);
                if (predicateRelations) { predicateRelations->fix_predicate_state_asc_if_non_lazy(pred_state, tmp_pred_index); }

                predicate_traverser.next(pred_state);

                enumerate(pred_state, predicate_traverser);

                predicate_traverser.previous();
                pred_state.pop_layer();

            }

            if constexpr (PLAJA_GLOBAL::queryPerBranchInit) { solver->pop(); }
            /* End second branch */

        }

    }

}

std::list<std::unique_ptr<PaStateValues>> InitialStatesEnumeratorPa::enumerate() {
    PLAJA_ASSERT(paStates.empty())
    PLAJA_ASSERT(locationState->size() == numLocations) // *non*-incremental usage

    numPredicates = modelZ3->get_number_predicates();

    PredicateState predicate_state(numPredicates, numLocations);
    predicate_state.set_location_state(*locationState);

    if (predicateRelations) { predicateRelations->fix_predicate_state(predicate_state); }

    auto pred_traverser = parent->init_predicate_traverser();
    enumerate(predicate_state, pred_traverser);

    reset_smt(); // free memory
    return std::move(paStates);
}

std::list<std::unique_ptr<PaStateValues>> InitialStatesEnumeratorPa::enumerate(const AbstractState& base) {
    PLAJA_ASSERT(not do_enumerate_explicitly())

    PLAJA_ASSERT(paStates.empty())
    PLAJA_ASSERT(locationState->size() == numLocations) // incremental usage
    PLAJA_ASSERT(base.get_size_locs() == numLocations) // incremental usage

    STMT_IF_DEBUG(const auto old_num_predicates = base.size() - numLocations)
    PLAJA_ASSERT((PLAJA_GLOBAL::lazyPA and old_num_predicates == numPredicates) or old_num_predicates < numPredicates) // Incremental usage.
    STMT_IF_DEBUG(for (auto it_loc = base.init_loc_state_iterator(); !it_loc.end(); ++it_loc) { PLAJA_ASSERT(it_loc.location_value() == locationState->get_int(it_loc.location_index())) }) // check for model's initial state

    PredicateState predicate_state(numPredicates, numLocations);
    predicate_state.set_location_state(*locationState);

    if (predicateRelations) { predicateRelations->fix_predicate_state(predicate_state); }

    predicate_state.push_layer();
    if (PLAJA_GLOBAL::queryPerBranchInit) { solver->push(); }

    auto predicate_traverser = parent->init_predicate_traverser();

    // set old predicate values
    for (; not predicate_traverser.end() and not predicate_traverser.traversed(base); predicate_traverser.next(predicate_state)) {

        const auto pred_index = predicate_traverser.predicate_index();

        if (predicate_state.is_defined(pred_index)) {

            PLAJA_ASSERT(base.predicate_value(pred_index) == predicate_state.predicate_value(pred_index))

            PLAJA_ASSERT(not predicate_state.is_set(pred_index))

            predicate_state.set(pred_index, predicate_state.predicate_value(pred_index));

            continue;
        }

        const auto pred_val = base.predicate_value(pred_index);

        PLAJA_ASSERT(not PA::is_unknown(pred_val))

        predicate_state.set(pred_index, pred_val);

        if (PLAJA_GLOBAL::queryPerBranchInit) { if (pred_val) { solver->add(modelZ3->get_src_predicate(pred_index)); } else { solver->add(!modelZ3->get_src_predicate(pred_index)); }; }

        if (predicateRelations) { predicateRelations->fix_predicate_state_asc_if_non_lazy(predicate_state, pred_index); }
    }

    /* Sanity (predicate traverser is expected to catch any truth value set in base). */
    if constexpr (PLAJA_GLOBAL::debug) {
        for (auto it = base.init_pred_state_iterator(); !it.end(); ++it) {
            PLAJA_ASSERT(it.is_set() == predicate_state.is_set(it.predicate_index()))
            PLAJA_ASSERT(not it.is_set() or it.predicate_value() == predicate_state.predicate_value(it.predicate_index()))
        }
    }

    enumerate(predicate_state, predicate_traverser);

    if (PLAJA_GLOBAL::queryPerBranchInit) { solver->pop(); }
    predicate_state.pop_layer();

    // reset_smt(); // Externally.
    return std::move(paStates);
}

std::list<std::unique_ptr<PaStateValues>> InitialStatesEnumeratorPa::enumerate_explicitly() {
    PLAJA_ASSERT(do_enumerate_explicitly())

    if (not conditionPtr) { return {}; } // nothing to do

    PLAJA_ASSERT(not PLAJA_GLOBAL::lazyPA) // Generate states do not reflect closure.

    PLAJA_LOG(PLAJA_UTILS::to_red_string("Performing explicit-to-abstract start state enumeration, which is only recommended for states-values-expressions!"))

    std::list<std::unique_ptr<PaStateValues>> pa_initial_states;
    explicitEnumerator->initialize();
    for (const auto& initial_state_values: explicitEnumerator->enumerate_states()) {
        pa_initial_states.push_back(modelZ3->compute_abstract_state_values(initial_state_values));
    }

    return pa_initial_states;
}

#pragma GCC diagnostic pop
