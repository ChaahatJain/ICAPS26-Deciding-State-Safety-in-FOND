//
// This file is part of the PlaJA code base.
// Copyright (c) (2019 - 2020) Marcel Vinzent.
// See README.md in the top-level directory for licensing information.
//

#include <fstream>
#include "search_space_pa_base.h"
#include "../../../stats/stats_base.h"
#include "../../../utils/structs_utils/update_op_structure.h"
#include "../../factories/predicate_abstraction/pa_options.h"
#include "../../factories/configuration.h"
#include "../../information/model_information.h"
#include "../match_tree/predicate_traverser.h"
#include "../pa_states/abstract_path.h"
#include "../pa_states/predicate_state.h"
#include "../pa_transition_structure.h"
#include "../smt/model_z3_pa.h"
#include "initial_states_enumerator_pa.h"
#include "search_information_pa.h"

#ifdef USE_INC_REG

#include "../inc_state_space/inc_state_space_reg.h"

#endif
#ifdef USE_MATCH_TREE

#include "../inc_state_space/inc_state_space_mt.h"

#endif

SearchSpacePABase::SearchSpacePABase(const PLAJA::Configuration& config, const ModelZ3PA& model_z3):
    modelZ3(&model_z3)
    , initialStatesEnumerator(nullptr)
    , initialState(STATE::none)
    , numPreds(modelZ3->get_number_predicates())
    , useIncSearch(config.get_bool_option(PLAJA_OPTION::incremental_search))
    , cacheWitnesses(config.is_flag_set(PLAJA_OPTION::cacheWitnesses))
    , statsHeuristic(config.get_sharable_ptr<PLAJA::StatsBase>(PLAJA::SharableKey::STATS_HEURISTIC))
    , currentStep(1)
    , numGoalPathStates(0) {
    PLAJA_ASSERT(config.has_sharable(PLAJA::SharableKey::MODEL_Z3))

    // share search space
    if (not config.has_sharable(PLAJA::SharableKey::SEARCH_SPACE_PA)) { sharedSearchSpace = config.set_sharable(PLAJA::SharableKey::SEARCH_SPACE_PA, std::make_shared<SEARCH_SPACE_PA::SharedSearchSpace>()); }
    else { sharedSearchSpace = config.get_sharable<SEARCH_SPACE_PA::SharedSearchSpace>(PLAJA::SharableKey::SEARCH_SPACE_PA); }

    if (not sharedSearchSpace->has_state_registry()) {
        PLAJA_ASSERT(modelZ3->_predicates())
        sharedSearchSpace->set_state_registry_pa(std::make_shared<StateRegistryPa>(config, *modelZ3->_predicates(), modelZ3->get_model_info()));
    }
    stateRegPa = sharedSearchSpace->share_state_registry();

    // pa registry
    stateRegPa->add_registry(modelZ3->get_number_predicates());

    // state space
#ifdef USE_INC_REG
    if (not sharedSearchSpace->has_state_space()) { sharedSearchSpace->set_state_space(std::make_unique<SEARCH_SPACE_PA::IncStateSpaceReg>(config, stateRegPa)); }
#endif
#ifdef USE_MATCH_TREE
    if (not sharedSearchSpace->has_state_space()) { sharedSearchSpace->set_state_space(std::make_unique<SEARCH_SPACE_PA::IncStateSpaceMt>(config, stateRegPa)); }
#endif
    incStateSpace = sharedSearchSpace->share_state_space();
    incStateSpace->increment();

    // initial states enumerator
    initialStatesEnumerator = std::make_unique<InitialStatesEnumeratorPa>(config, *this);

}

SearchSpacePABase::~SearchSpacePABase() = default;

/**++++++++++++********************************************************************************************************/

UpdateOpID_type SearchSpacePABase::compute_update_op(ActionOpID_type op, UpdateIndex_type update) const { return modelZ3->get_model_info().compute_update_op_id(op, update); }

UpdateOpID_type SearchSpacePABase::compute_update_op(const PATransitionStructure& transition) const { return compute_update_op(transition._action_op_id(), transition._update_index()); }

UpdateOpStructure SearchSpacePABase::compute_update_op(UpdateOpID_type update_op) const { return modelZ3->get_model_info().compute_update_op_structure(update_op); }

const class ActionOp& SearchSpacePABase::retrieve_action_op(ActionOpID_type op_id) const { return modelZ3->get_action_op_concrete(op_id); }

/**++++++++++++********************************************************************************************************/

void SearchSpacePABase::increment() {

    initialStatesEnumerator->increment();
    initialStates.clear();
    initialState = STATE::none;

    stateRegPa->add_registry(modelZ3->get_number_predicates());

    incStateSpace->increment();

    // reset
    searchInformation.resize(0);
    this->reset();
}

void SearchSpacePABase::reset() {
    // search node
    ++currentStep;

    // goal states
    goalPathFrontier.clear();
    numGoalPathStates = 0;

    // incremental state space (if not used)
    if (not useIncSearch) {

        cachedStartStates.clear();

        incStateSpace->clear_coarser();

        stateRegPa->drop_coarse_registries();

    }

}

PredicateTraverser SearchSpacePABase::init_predicate_traverser() const { return stateRegPa->init_predicate_traverser(); }

void SearchSpacePABase::compute_initial_states() {
    initialStates.clear();

    if constexpr (PLAJA_GLOBAL::lazyPA) { // for lazy PA we must unset predicates not in closure
        const auto* pa_initial_state = modelZ3->get_pa_initial_state();

        PaStateValues pa_initial_state_closed(stateRegPa->get_predicates_size(), modelZ3->get_number_automata_instances(), &stateRegPa->get_predicates());
        pa_initial_state_closed.set_location_state(pa_initial_state->get_location_state());

        for (auto pred_traverser = stateRegPa->init_predicate_traverser(); !pred_traverser.end(); pred_traverser.next(*pa_initial_state)) {
            const auto pred_index = pred_traverser.predicate_index();
            pa_initial_state_closed.set_predicate_value(pred_index, pa_initial_state->predicate_value(pred_index));
        }

        initialState = add_initial_state(pa_initial_state_closed);

    } else { initialState = add_initial_state(*modelZ3->get_pa_initial_state()); }

    if (initialStatesEnumerator->do_enumerate_explicitly()) {
        for (const auto& pa_state_values: initialStatesEnumerator->enumerate_explicitly()) { add_initial_state(*pa_state_values); }
    } else {
        std::list<std::unique_ptr<PaStateValues>> start_states;
        if (cachedStartStates.empty()) {
            start_states = initialStatesEnumerator->enumerate();
        } else { // incremental usage
            const auto cached_start_states = std::move(cachedStartStates);
            for (const auto& start_id: cached_start_states) { start_states.splice(start_states.cend(), initialStatesEnumerator->enumerate(*stateRegPa->get_pa_state(numPreds, start_id))); }
            initialStatesEnumerator->reset_smt();
        }
        PLAJA_ASSERT(cachedStartStates.empty())
        cachedStartStates.reserve(start_states.size());
        for (const auto& start_state: start_states) { cachedStartStates.push_back(add_initial_state(*start_state)); }
    }

    // model's initial state (added first see above)
    // initialState = add_initial_state(*modelZ3->_initialStateValues());

    numPreds = modelZ3->get_number_predicates(); // finally updated (concludes increment)

    // reset registries
    if constexpr (PLAJA_GLOBAL::useIncMt and not PLAJA_GLOBAL::useRefReg) { stateRegPa->drop_coarse_registries(); }
}

StateID_type SearchSpacePABase::add_initial_state(const PaStateValues& pa_initial_state) {
    STMT_IF_LAZY_PA(PLAJA_ASSERT(stateRegPa->is_closed(pa_initial_state, true)))
    return *initialStates.insert(stateRegPa->add_pa_state(pa_initial_state)).first;
}

/**++++++++++++********************************************************************************************************/

std::unique_ptr<AbstractState> SearchSpacePABase::get_abstract_state(PredicatesNumber_type num_preds, StateID_type state_id) const {
    PLAJA_ASSERT_EXPECT(num_preds == modelZ3->get_number_predicates())
    return stateRegPa->get_pa_state(num_preds, state_id);
}

std::unique_ptr<AbstractState> SearchSpacePABase::get_abstract_state(StateID_type state_id) const { return stateRegPa->get_pa_state(state_id); }

std::unique_ptr<AbstractState> SearchSpacePABase::set_abstract_state(const PredicateState& pa_state_values) const {
    STMT_IF_LAZY_PA(PLAJA_ASSERT(stateRegPa->is_closed(pa_state_values, true)))
    return stateRegPa->add_pa_state(pa_state_values);
}

#ifndef NDEBUG

std::unique_ptr<AbstractState> SearchSpacePABase::set_abstract_state(const PaStateValues& pa_state_values) const {
    return stateRegPa->get_pa_state(pa_state_values.get_size_predicates(), stateRegPa->add_pa_state(pa_state_values));
}

#endif

std::unique_ptr<AbstractState> SearchSpacePABase::set_abstract_state(const AbstractState& different_reg_state) const {
    PLAJA_ASSERT(different_reg_state.get_size_predicates() < modelZ3->get_number_predicates())

    PredicateState predicate_state(modelZ3->get_number_predicates(), modelZ3->get_number_automata_instances());

    different_reg_state.to_location_state(predicate_state.get_location_state());

    for (auto it = different_reg_state.init_pred_state_iterator(); !it.end(); ++it) {
        predicate_state.set(it.predicate_index(), it.predicate_value());
    }

    return set_abstract_state(predicate_state);
}

std::unique_ptr<AbstractState> SearchSpacePABase::compute_abstraction(const StateBase& concrete_state) const { return stateRegPa->add_pa_state(concrete_state); }

void SearchSpacePABase::unregister_state_if_possible(StateID_type id) {
    PLAJA_ASSERT(PLAJA_GLOBAL::useIncMt and not PLAJA_GLOBAL::useRefReg) // Non-match tree state space uses state ids for exclusion information. NOLINT(*-dcl03-c)
    stateRegPa->unregister_state_if_possible(id);
}

#ifndef NDEBUG

std::unique_ptr<PaStateValues> SearchSpacePABase::get_pa_state_values(StateID_type state_id) const { return get_abstract_state(state_id)->to_pa_state_values(true); }

std::unique_ptr<PaStateValues> SearchSpacePABase::get_pa_initial_state_values() const {
    PLAJA_ASSERT(initialState != STATE::none)
    return get_pa_state_values(initialState);
}

#endif

/**++++++++++++********************************************************************************************************/
#ifdef USE_VERITAS
void SearchSpacePABase::add_transition(const PATransitionStructure& transition, const StateValues& witness_src, const StateValues& witness_suc, std::vector<veritas::Interval> box) {
    PLAJA_ASSERT(not incStateSpace->transition_exists(*transition._source(), transition.get_update_op_id(), *transition._target()))

    // TODO Currently we only store the witness information in state space.
    //  Might think about option to (only) add abstract transition information (similar to transition exclusion).
    //  This information is so far only stored (backwards) in search nodes.
    if (not cacheWitnesses) { return; }
    if (!box.empty()) {
        int src_size = witness_src.get_int_state_size() + witness_src.get_floating_state_size() - 1; // -1 to remove location variable.
        int suc_size = witness_suc.get_int_state_size() + witness_suc.get_floating_state_size() - 1;
        std::vector<veritas::Interval> box1(box.begin(), box.begin() + src_size);
        std::vector<veritas::Interval> box2(box.begin() + src_size, box.begin() + src_size + suc_size);
        incStateSpace->add_transition_witness(*transition._source(), transition.get_update_op_id(), add_witness(witness_src, box1), add_witness(witness_suc, box2));
    } else {
        incStateSpace->add_transition_witness(*transition._source(), transition.get_update_op_id(), add_witness(witness_src, box), add_witness(witness_suc, box));
    }
}
#endif

void SearchSpacePABase::add_transition(const PATransitionStructure& transition, const StateValues& witness_src, const StateValues& witness_suc) {
    PLAJA_ASSERT(not incStateSpace->transition_exists(*transition._source(), transition.get_update_op_id(), *transition._target()))

    // TODO Currently we only store the witness information in state space.
    //  Might think about option to (only) add abstract transition information (similar to transition exclusion).
    //  This information is so far only stored (backwards) in search nodes.
    if (not cacheWitnesses) { return; }
    incStateSpace->add_transition_witness(*transition._source(), transition.get_update_op_id(), add_witness(witness_src), add_witness(witness_suc));
}

StateID_type SearchSpacePABase::retrieve_witness(StateID_type source, UpdateOpID_type update_op_id, StateID_type successor) const {
    auto rlt = incStateSpace->retrieve_witness(*get_abstract_state(source), update_op_id, *get_abstract_state(successor));
    PLAJA_ASSERT(rlt == nullptr or *rlt != STATE::none)
    return rlt == nullptr ? STATE::none : *rlt; // In this context, None is to be interpreted "I have no witness".
}

/** post search ********************************************************************************************************/

AbstractPath SearchSpacePABase::extract_path_backwards(StateID_type current_id) const {
    std::list<UpdateOpStructure> op_path;
    std::list<StateID_type> witness_path;
    std::list<StateID_type> pa_states;

    const auto* current_node = &get_node_info(current_id);
    pa_states.push_front(current_id);

    for (;;) { // extract path ...
        PLAJA_ASSERT(SEARCH_SPACE_PA::SearchNode::is_reached(*current_node, currentStep))
        if (current_node->is_start()) { break; }

        // add operator
        op_path.emplace_front(compute_update_op(current_node->get_parent_update_op()));

        // add witness
        witness_path.push_front(retrieve_witness(current_node->get_parent(), current_node->get_parent_update_op(), current_id));

        current_id = current_node->get_parent();
        current_node = &get_node_info(current_node->get_parent());

        // add parent
        pa_states.push_front(current_id);
    }

    AbstractPath abstract_path(*modelZ3->_predicates(), *this);
    abstract_path.set_op_path(op_path);
    abstract_path.set_witness_path(witness_path);
    abstract_path.set_pa_path(pa_states);

    return abstract_path;
}

AbstractPath SearchSpacePABase::extract_goal_path_forward(StateID_type state_id) const {
    PLAJA_ASSERT(information_size() < STATE::none)

    AbstractPath path(*modelZ3->_predicates(), *this);

    StateID_type current_id = state_id;
    const auto* current_node = &get_node_info(state_id);

    // no goal path state
    if (not current_node->has_goal_path()) { return path; }

    path.append_abstract_state(current_id);

    if (current_node->is_goal()) { return path; } // state is goal itself

    while (not current_node->is_goal()) {
        const auto* successor_node = &get_node_info(current_node->get_goal_path_successor());

        // find successor op
        UpdateOpID_type update_op_id = ACTION::noneUpdateOp;
        for (auto it = successor_node->_parents(); !it.end(); ++it) {
            if (it.parent_state() == current_id) {
                update_op_id = it.parent_update_op();
                break;
            }
        }
        PLAJA_ASSERT(update_op_id != ACTION::noneUpdateOp)
        auto update_op_struct = compute_update_op(update_op_id);
        PLAJA_ASSERT(update_op_struct.actionOpID != ACTION::noneOp and update_op_struct.updateIndex != ACTION::noneUpdate)

        // add to path
        path.append_op(update_op_struct.actionOpID, update_op_struct.updateIndex);

        // add witness
        path.append_witness(retrieve_witness(current_id, update_op_id, current_node->get_goal_path_successor()));

        // successor
        path.append_abstract_state(current_node->get_goal_path_successor());

        current_id = current_node->get_goal_path_successor();
        current_node = &get_node_info(current_id);
    }

    return path;
}

AbstractPath SearchSpacePABase::extract_goal_path(StateID_type state_id) const {
    auto path_from_start = extract_path_backwards(state_id);
    auto path_to_goal = extract_goal_path_forward(state_id);
    path_from_start.append(std::move(path_to_goal));
    return path_from_start;
}

//

std::size_t SearchSpacePABase::compute_goal_path_states() {
    // PLAJA_ASSERT(goalPathFrontier.size() == 1)

    while (not goalPathFrontier.empty()) {
        ++numGoalPathStates;

        const auto current_id(goalPathFrontier.front());
        const auto& current_node = get_node_info(current_id);
        goalPathFrontier.pop_front();
        PLAJA_ASSERT(current_node.has_goal_path())

        for (auto it = current_node._parents(); !it.end(); ++it) {
            auto& parent_node = get_node_info(it.parent_state());
            PLAJA_ASSERT(SEARCH_SPACE_PA::SearchNode::has_been_reached(parent_node))
            PLAJA_ASSERT(not parent_node.is_dead_end())

            if (parent_node.has_goal_path()) {
                if (parent_node.get_goal_distance() <= current_node.get_goal_distance() + 1) { continue; }
                else { statsHeuristic->inc_attr_unsigned(PLAJA::StatsUnsigned::UPDATED_ESTIMATES); }
            }

            // PLAJA_ASSERT(not parent_node.has_goal_path() or parent_node.is_reached()) // in the current config the distances for states explored in previous steps should be optimal (if present)
            parent_node.set_goal_path_successor(current_id, current_node.get_goal_distance() + 1);
            PLAJA_ASSERT(it.parent_state() != parent_node.get_goal_path_successor())
            goalPathFrontier.push_back(it.parent_state());
        }

    }

    return numGoalPathStates;
}

void SearchSpacePABase::compute_dead_ends_backwards(StateID_type state_id) {

    auto* node = &get_node_info(state_id);
    PLAJA_ASSERT(not node->known_goal_distance())
    node->set_dead_end();

    // backwards ...
    std::list<StateID_type> queue { state_id };
    while (not queue.empty()) {

        node = &get_node_info(queue.front());
        PLAJA_ASSERT(not node->is_dead_end())
        queue.pop_front();

        for (auto it = node->_parents(); !it.end(); ++it) {
            auto* parent_node = &get_node_info(it.parent_state());
            PLAJA_ASSERT(not parent_node->has_goal_path())
            if (parent_node->is_dead_end()) { continue; }
            parent_node->set_dead_end();
            queue.push_back(it.parent_state());
        }

    }

}

std::size_t SearchSpacePABase::number_of_initial_goal_path_states() const {
    std::size_t rlt = 0;
    for (const StateID_type initial_state_id: initialStates) { rlt += has_goal_path(initial_state_id); }
    return rlt;
}

/** stats *************************************************************************************************************/

void SearchSpacePABase::update_statistics() const { incStateSpace->update_stats(); }

void SearchSpacePABase::print_statistics() const {
    PLAJA_LOG(PLAJA_UTILS::string_f("RegisteredStates: %u", stateRegPa->size()))
    PLAJA_LOG(PLAJA_UTILS::string_f("BytesPerState: %u", stateRegPa->get_max_num_bytes_per_state()))
}

void SearchSpacePABase::stats_to_csv(std::ofstream& file) const {
    file << PLAJA_UTILS::commaString << stateRegPa->size();
    file << PLAJA_UTILS::commaString << stateRegPa->get_max_num_bytes_per_state();
}

void SearchSpacePABase::stat_names_to_csv(std::ofstream& file) {
    file << ",RegisteredStates,BytesPerState";
}
