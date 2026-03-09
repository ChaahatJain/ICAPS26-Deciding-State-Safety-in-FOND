//
// This file is part of the PlaJA code base (2019 - 2021).
//

#ifndef PLAJA_EXPANSION_PA_BASE_PARAMETERIZED_H
#define PLAJA_EXPANSION_PA_BASE_PARAMETERIZED_H

#include "search_pa_macros.h"

#pragma GCC diagnostic push
#pragma ide diagnostic ignored "Simplify"
#pragma ide diagnostic ignored "UnreachableCode"

#include "expansion_pa_base.h"
#include "../../smt/solver/smt_solver_z3.h"
#include "../inc_state_space/predicate_traverser.h"
#include "../optimization/pred_entailment_cache_interface.h"
#include "../optimization/predicate_relations.h"
#include "../pa_states/abstract_state.h"
#include "../pa_states/predicate_state.h"
#include "../search_space/search_node_pa.h"
#include "../smt/model_z3_pa.h"
#include "../successor_generation/action_op/action_op_pa.h"
#include "../successor_generation/update/update_pa.h"
#include "../successor_generation/successor_generator_pa.h"
#include "../pa_transition_structure.h"
#include "../solution_checker.h"

template<typename SearchPA_t, bool return_with_goal_path = false, bool optimal_search = false>
class ExpansionPABaseParameterized final: public ExpansionPABase {

private:
    bool foundTransition;
    bool hitReturn; // see bool templates
    bool goalReturn;
    SearchDepth_type currentSearchDepth;
    SearchDepth_type upperBoundGoalDistance;

/**********************************************************************************************************************/

public:
    explicit ExpansionPABaseParameterized(const SearchEngineConfigPA& config,
                                          SearchSpacePABase* search_space, const ModelZ3PA& model_z3):
        ExpansionPABase(config, search_space, model_z3)
        , foundTransition(false)
        , hitReturn(false)
        , currentSearchDepth(0)
        , upperBoundGoalDistance(std::numeric_limits<SearchDepth_type>::max()) {}

    ~ExpansionPABaseParameterized() override = default;
    DELETE_CONSTRUCTOR(ExpansionPABaseParameterized)

    void handle_transition_existence(SEARCH_SPACE_PA::SearchNode& target_node) {
        PLAJA_ASSERT(not hitReturn)

        const auto& target = *transitionStruct._target();
        auto target_id = transitionStruct._target_id();

        foundTransition = true;
        sharedStats->inc_attr_unsigned(PLAJA::StatsUnsigned::TRANSITIONS);

        // handle newly reached:
        if (not target_node.is_reached()) {
            target_node.set_reached(transitionStruct._source_id(), searchSpace->compute_update_op(transitionStruct), currentSearchDepth + 1);
            sharedStats->inc_attr_unsigned(PLAJA::StatsUnsigned::REACHABLE_STATES);

            // goal check:
            if (target_node.has_goal_path()) {
                searchSpace->add_goal_path_frontier(target_id);
                if constexpr (return_with_goal_path and not optimal_search) { hitReturn = true; }
                else { upperBoundGoalDistance = std::min(upperBoundGoalDistance, target_node._start_distance() + target_node._goal_distance()); }
            } else if (reachChecker->check(target)) {
                target_node.set_goal();
                sharedStats->inc_attr_unsigned(PLAJA::StatsUnsigned::GOAL_STATES);
                searchSpace->add_goal_path_frontier(target_id);
                if constexpr (return_with_goal_path and not optimal_search) { hitReturn = true; }
                else { upperBoundGoalDistance = std::min(upperBoundGoalDistance, target_node._start_distance()); }
            }
                // else: cache for expansion; however if searching for goal path, we may skip if start distance already exceed goal path upper bound
            else if (not return_with_goal_path or target_node._start_distance() + 1 < upperBoundGoalDistance) { newlyReached.push_back(target_id);
            //  std::cout << "Adding target: " << target_id << std::endl;
              }

        } else if (optimal_search and currentSearchDepth + 1 < target_node._start_distance()) { // reopen
            target_node.set_reached(transitionStruct._source_id(), searchSpace->compute_update_op(transitionStruct), currentSearchDepth + 1);
            // goal check:
            if (target_node.has_goal_path()) { upperBoundGoalDistance = std::min(upperBoundGoalDistance, target_node._start_distance() + target_node._goal_distance()); }
                // else: cache for expansion; however if searching for goal path, we may skip if start distance already exceeds goal path upper bound
            else if (not return_with_goal_path or target_node._start_distance() + 1 < upperBoundGoalDistance) { newlyReached.push_back(target_id); 
            // std::cout << "Adding target bleh: " << target_id << std::endl;
            }
        } else {
            PLAJA_ASSERT(PA_COMPUTATION_AUX::is_pa<SearchPA_t>())
            target_node.add_parent(transitionStruct._source_id(), searchSpace->compute_update_op(transitionStruct));
        } // PA

        if constexpr (PLAJA_GLOBAL::paCacheSuccessors) {
            PLAJA_ASSERT(not currentSuccessors.count(target_id))
            currentSuccessors.insert(target_id); // cache successor in case we are only interested in step reachability
        }

    }

    void compute_transition(PredicateState& pred_state) {

        // Generate potential target
        transitionStruct.set_target(searchSpace->set_abstract_state(pred_state));
        sharedStats->inc_attr_unsigned(PLAJA::StatsUnsigned::GENERATED_STATES);
        auto target_node = searchSpace->add_node(transitionStruct._target()->to_ptr());

        if (target_node.is_dead_end()) {
            PLAJA_ABORT
            return;
        } // do not expect any dead ends for now

        if constexpr (return_with_goal_path or PLAJA_GLOBAL::paOnlyReachability) { // reachability
            if (target_node.is_reached()) {
                if (not optimal_search or target_node._start_distance() <= currentSearchDepth + 1) { return; } // for optimal search we may have to reopen
            }
        } else { // step reachability
            PLAJA_STATIC_ASSERT(PA_COMPUTATION_AUX::is_pa<SearchPA_t>())
            // in predicate abstraction & if only interested in step-reachability, we may still skip, if there is already an update from current source to target:
            if constexpr (PLAJA_GLOBAL::paCacheSuccessors) { if (currentSuccessors.count(transitionStruct._target_id())) { return; } }
        }

        // TODO maybe once we have some public MT traverser (e.g., for lazy abstraction),
        //  we can check for exclusion and existence separately, the former prior to adding the node or even during successor enumeration.
        const auto witness = searchSpace->transition_exists(*transitionStruct._source(), transitionStruct._action_op_id(), transitionStruct._update_index(), *transitionStruct._target());
        if (witness) { // already know result
            if (*witness != STATE::none) {
                sharedStats->inc_attr_unsigned(PLAJA::StatsUnsigned::REUSED_TRANSITION_EXISTENCE);
                handle_transition_existence(target_node);
            } else { // excluded, i.e., transition does not exist
                sharedStats->inc_attr_unsigned(PLAJA::StatsUnsigned::REUSED_TRANSITION_EXCLUSION);
                if (PLAJA_GLOBAL::useIncMt and not PLAJA_GLOBAL::useRefReg and not target_node.is_reached()) { searchSpace->unregister_state_if_possible(target_node._state().get_id_value()); }
            }
            return;
        }

        // Transition test
        if constexpr (not PLAJA_GLOBAL::queryPerBranchExpansion) { push_solution(); } // PUSH transition test solution frame
        if (PLAJA_GLOBAL::queryPerBranchExpansion or transition_test()) {
            // policy transition test ...
            if (nn_sat_transition_test()) { handle_transition_existence(target_node); }
            else if (PLAJA_GLOBAL::useIncMt and not PLAJA_GLOBAL::useRefReg and not target_node.is_reached()) { searchSpace->unregister_state_if_possible(target_node._state().get_id_value()); }
        } else if (PLAJA_GLOBAL::useIncMt and not PLAJA_GLOBAL::useRefReg and not target_node.is_reached()) { searchSpace->unregister_state_if_possible(target_node._state().get_id_value()); }
        if constexpr (not PLAJA_GLOBAL::queryPerBranchExpansion) { pop_solution(); } // POP transition test solution frame

    }

    void enumerate_pa_states(PredicateState& pred_state, PredicateTraverser& predicate_traverser/*PredicateIndex_type pred_index*/) {

        if (predicate_traverser.end()) {
            compute_transition(pred_state);
            transitionStruct.unset_target();
        } else { // if (pred_index < pred_state.get_size_predicates()) { // continue recursion

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
                    transitionStruct._update()->add_predicate_to_solver(*solver, pred_index, true);
                }

                if (not PLAJA_GLOBAL::queryPerBranchExpansion or solver->check()) {

                    pred_state.push_layer();
                    pred_state.set(pred_index, true);
                    if (predicateRelations) { predicateRelations->fix_predicate_state_asc_if_non_lazy(pred_state, pred_index); }

                    predicate_traverser.next(pred_state);

                    enumerate_pa_states(pred_state, predicate_traverser);

                    predicate_traverser.previous();
                    pred_state.pop_layer();

                    if (hitReturn) { return; } // PATH_SEARCH: termination condition
                }

                if constexpr (PLAJA_GLOBAL::queryPerBranchExpansion) { solver->pop(); }
                /* End first branch */

                /* Second branch */
                if constexpr (PLAJA_GLOBAL::queryPerBranchExpansion) {
                    solver->push();
                    transitionStruct._update()->add_predicate_to_solver(*solver, pred_index, false);
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
        } // else { compute_transition(pred_state); }

    }

    /**
     * @return true if new transition was found
     */
    bool enumerate_pa_states(PredicateState& pred_state) {
        foundTransition = false;
        PUSH_LAP(sharedStats, PLAJA::StatsDouble::PA_SUCCESSOR_ENUM_TIME)
        auto pred_traverser = searchSpace->init_predicate_traverser();
        enumerate_pa_states(pred_state, pred_traverser /*0*/);
        POP_LAP(sharedStats, PLAJA::StatsDouble::PA_SUCCESSOR_ENUM_TIME)
        return foundTransition;
    }

    bool step_per_action_op() {

        bool found_transition = false;

        nn_sat_push_action_op(); // PUSH ACTION OP
        if constexpr (PLAJA_GLOBAL::doNNApplicabilityTest) {
            if (not nn_sat_applicability_test()) {
                nn_sat_pop_action_op(); // POP ACTION OP
                return false;
            }
        }

        PUSH_LAP(sharedStats, PLAJA::StatsDouble::PA_UPDATE_IT_TIME)
        for (auto update_it = transitionStruct._action_op()->updateIteratorPA(); !update_it.end(); ++update_it) { // for each PA update structure

            const auto& update = update_it.update();
            push_update(update_it.update_index(), update); // PUSH UPDATE

            PredicateState predicate_state(modelZ3->get_number_predicates(), modelZ3->get_number_automata_instances(), modelZ3->_predicates());
            transitionStruct._source()->to_location_state(predicate_state.get_location_state()); // copy non-updated locations
            update.evaluate_locations(predicate_state.get_location_state());

            // Entailment Optimizations
            PUSH_LAP(sharedStats, PLAJA::StatsDouble::PA_UPD_ENTAILMENT_TIME)
            {
                PredEntailmentCacheInterface entailments_cache_interface(searchSpace, &transitionStruct, predicateRelations.get());
                update.fix(predicate_state, *solver, *transitionStruct._source(), entailments_cache_interface);
            }
            POP_LAP(sharedStats, PLAJA::StatsDouble::PA_UPD_ENTAILMENT_TIME)
            STMT_IF_DEBUG(if (predicateRelations) { predicateRelations->check_all_entailments(predicate_state); }) // ./PlaJA ../../benchmarks/racetrack-C6/benchmarks/tiny/tiny_09.jani --additional_properties ../../benchmarks/racetrack-C6/additional_properties/predicates/tiny/tiny_c_map_c_car.jani --engine PREDICATE_ABSTRACTION --prop 8 --predicate-relations --predicate-sat
            found_transition = enumerate_pa_states(predicate_state) || found_transition;
            pop_update(); // POP UPDATE

            if (hitReturn) { break; } // PATH_SEARCH: termination condition
        }
        POP_LAP(sharedStats, PLAJA::StatsDouble::PA_UPDATE_IT_TIME)

        nn_sat_pop_action_op(); // POP ACTION OP

        if (found_transition) { sharedStats->inc_attr_unsigned(PLAJA::StatsUnsigned::ACTION_OPS); }
        return found_transition;
    }

/**********************************************************************************************************************/

    SearchEngine::SearchStatus step(StateID_type src_id) override { // i.e., the "this" pointer
        hitReturn = false;
        goalReturn = false;
        reset();
        const auto src_node = searchSpace->get_node(src_id);
        currentSearchDepth = src_node._start_distance();
        upperBoundGoalDistance = std::numeric_limits<SearchDepth_type>::max();

        // Expand state
        sharedStats->inc_attr_unsigned(PLAJA::StatsUnsigned::EXPANDED_STATES);

        // Explore
        if (terminalChecker && terminalChecker->check(src_node._state())) {
            PLAJA_ASSERT_EXPECT(terminalChecker->is_exact())
            sharedStats->inc_attr_unsigned(PLAJA::StatsUnsigned::TERMINAL_TESTS_SAT);
            if (terminalChecker->is_exact()) {
                sharedStats->inc_attr_unsigned(PLAJA::StatsUnsigned::TERMINAL_STATES);
                return SearchEngine::IN_PROGRESS;
            }
        } 
        push_source(src_node._state().to_ptr()); // PUSH SOURCE STATE

        // refine incremental state space (move witness downwards)
        PUSH_LAP(sharedStats, PLAJA::StatsDouble::PA_SS_REFINE_TIME)
        searchSpace->refine_transitions(*transitionStruct._source());
        POP_LAP(sharedStats, PLAJA::StatsDouble::PA_SS_REFINE_TIME)

        PUSH_LAP(sharedStats, PLAJA::StatsDouble::PA_SUC_GEN_EXPLORE_TIME)
        if (exploreNonInc) { successorGenerator->explore(*transitionStruct._source(), *solver); }
        else { successorGenerator->explore_for_incremental(*transitionStruct._source(), *solver); }
        POP_LAP(sharedStats, PLAJA::StatsDouble::PA_SUC_GEN_EXPLORE_TIME)

        bool terminal_state = true; // (under policy restriction)

        // Iterate:
        PUSH_LAP(sharedStats, PLAJA::StatsDouble::PA_ACTION_LABEL_IT_TIME)
        for (auto action_it = successorGenerator->actionIteratorPA(); !action_it.end(); ++action_it) {
            // already excluded ?
            if (searchSpace->is_excluded_label(*transitionStruct._source(), action_it.action_label())) { continue; }

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
            bool found_transition = false; // (under policy restriction)

            // iterate per action
            PUSH_LAP(sharedStats, PLAJA::StatsDouble::PA_ACTION_OP_IT_TIME)
            for (auto per_action_it = action_it.iterator(); !per_action_it.end(); ++per_action_it) {
                const ActionOpPA& action_op = *per_action_it;

                // already excluded ?
                if (searchSpace->is_excluded_op(*transitionStruct._source(), action_op._op_id())) { continue; }

                set_action_op(action_op); // SET ACTION OP

                solver->push(); // PUSH GUARD
                action_op.add_to_solver(*solver);

                if (exploreNonInc) { // Already checked applicability.
                } else {
                    // incremental solving
                    if (not applicability_test()) {
                        solver->pop(); // POP GUARD
                        unset_action_op(); // UNSET ACTION OP
                        continue;
                    }
                }

                any_applicable_op = true;
                found_transition = step_per_action_op() or found_transition;
                solver->pop(); // POP GUARD
                unset_action_op(); // UNSET ACTION OP

                if constexpr (PLAJA_GLOBAL::paOnlyStepReachabilityPerOperator) { currentSuccessors.clear(); } // PA
                if (hitReturn) { break; } // PATH_SEARCH: termination condition
                // if (goalReturn) { break; }
            }
            POP_LAP(sharedStats, PLAJA::StatsDouble::PA_ACTION_OP_IT_TIME)

            if (not any_applicable_op) { searchSpace->set_excluded_label(*transitionStruct._source(), transitionStruct._action_label()); }
            if (found_transition) {
                sharedStats->inc_attr_unsigned(PLAJA::StatsUnsigned::ACTION_LABELS);
                terminal_state = false;
            } // (under policy restriction)

            pop_action(); // POP ACTION

            if constexpr (PLAJA_GLOBAL::paOnlyStepReachabilityPerLabel) { currentSuccessors.clear(); } // PA
            if (hitReturn) { break; } // PATH_SEARCH: termination condition
            // if (goalReturn) { break; }
        }
        POP_LAP(sharedStats, PLAJA::StatsDouble::PA_ACTION_LABEL_IT_TIME)

        // de-caching
        if constexpr (PLAJA_GLOBAL::paOnlyStepReachability) { currentSuccessors.clear(); } // PA

        // check terminal ...
        if (terminal_state) { sharedStats->inc_attr_unsigned(PLAJA::StatsUnsigned::TERMINAL_STATES); }

        pop_source(); // POP SOURCE STATE

        return hitReturn ? SearchEngine::SOLVED : SearchEngine::IN_PROGRESS; // termination condition
    }

};

#pragma GCC diagnostic pop

#endif // PLAJA_EXPANSION_PA_BASE_PARAMETERIZED_H