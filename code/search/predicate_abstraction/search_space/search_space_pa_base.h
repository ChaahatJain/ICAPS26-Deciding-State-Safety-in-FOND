//
// This file is part of the PlaJA code base.
// Copyright (c) (2019 - 2020) Marcel Vinzent.
// See README.md in the top-level directory for licensing information.
//

#ifndef PLAJA_SEARCH_SPACE_PA_BASE_H
#define PLAJA_SEARCH_SPACE_PA_BASE_H

#include <list>
#include <memory>
#include "../../../include/ct_config_const.h"
#include "../../../stats/forward_stats.h"
#include "../../../utils/fd_imports/segmented_vector.h"
#include "../../../utils/structs_utils/forward_struct_utils.h"
#include "../../factories/forward_factories.h"
#include "../../using_search.h"
#include "../inc_state_space/forward_inc_state_space_pa.h"
#include "../pa_states/abstract_state.h"
#include "../pa_states/forward_pa_states.h"
#include "../smt/forward_smt_pa.h"
#include "../forward_pa.h"
#include "forward_search_space.h"
#include "search_node_pa.h"

class SearchSpacePABase final {

private:
    const ModelZ3PA* modelZ3; // underlying Z3 Model
    std::unique_ptr<InitialStatesEnumeratorPa> initialStatesEnumerator;
    StateID_type initialState; // (first) model initial state
    std::unordered_set<StateID_type> initialStates; // abstract initial states
    std::vector<StateID_type> cachedStartStates; // TODO redundant cache, though let's leave it like that for now, as Transport seems to be insanely lucky with the reverted start state expansion order resulting from unordered-set
    PredicatesNumber_type numPreds;
    //
    std::shared_ptr<StateRegistryPa> stateRegPa;
    bool useIncSearch;
    bool cacheWitnesses;
    std::shared_ptr<SEARCH_SPACE_PA::IncStateSpace> incStateSpace;
    std::shared_ptr<SEARCH_SPACE_PA::SharedSearchSpace> sharedSearchSpace;
    //
    PLAJA::StatsBase* statsHeuristic;

public:
    explicit SearchSpacePABase(const PLAJA::Configuration& config, const ModelZ3PA& model_z3);
    virtual ~SearchSpacePABase();
    DELETE_CONSTRUCTOR(SearchSpacePABase)

    // short-cuts:
    [[nodiscard]] UpdateOpID_type compute_update_op(const PATransitionStructure& transition) const;
    [[nodiscard]] UpdateOpID_type compute_update_op(ActionOpID_type op, UpdateIndex_type update) const;
    [[nodiscard]] UpdateOpStructure compute_update_op(UpdateOpID_type update_op) const;
    [[nodiscard]] const class ActionOp& retrieve_action_op(ActionOpID_type op_id) const;

    void increment();
    void reset(); // for incremental usage

    [[nodiscard]] PredicateTraverser init_predicate_traverser() const;

    void compute_initial_states();
    StateID_type add_initial_state(const PaStateValues& pa_initial_state);

    [[nodiscard]] inline std::size_t get_number_initial_states() const { return initialStates.size(); }

    [[nodiscard]] inline StateID_type _initialState() const {
        PLAJA_ASSERT(initialState != STATE::none)
        return initialState;
    }

    [[nodiscard]] inline const std::unordered_set<StateID_type>& _initialStates() const { return initialStates; }

    /** GET STATES ***************************************************************************************************/

    [[nodiscard]] std::unique_ptr<AbstractState> get_abstract_state(PredicatesNumber_type num_preds, StateID_type state_id) const;

    [[nodiscard]] std::unique_ptr<AbstractState> get_abstract_state(StateID_type state_id) const;

    [[nodiscard]] inline std::unique_ptr<AbstractState> get_abstract_initial_state() const {
        PLAJA_ASSERT(initialState != STATE::none)
        return get_abstract_state(initialState);
    }

    [[nodiscard]] std::unique_ptr<AbstractState> set_abstract_state(const PredicateState& pa_state_values) const;

    FCT_IF_DEBUG([[nodiscard]] std::unique_ptr<AbstractState> set_abstract_state(const PaStateValues& pa_state_values) const;)

    [[nodiscard]] std::unique_ptr<AbstractState> set_abstract_state(const AbstractState& different_reg_state) const;

    /** Compute abstraction of concrete state. */
    [[nodiscard]] std::unique_ptr<AbstractState> compute_abstraction(const StateBase& concrete_state) const;

    void unregister_state_if_possible(StateID_type id);

    FCT_IF_DEBUG([[nodiscard]] std::unique_ptr<PaStateValues> get_pa_state_values(StateID_type state_id) const;)

    FCT_IF_DEBUG([[nodiscard]] std::unique_ptr<PaStateValues> get_pa_initial_state_values() const;)

    /** Witness interface. ********************************************************************************************/
#ifdef USE_VERITAS
    inline StateID_type add_witness(const StateValues& witness, std::vector<veritas::Interval> box) { return stateRegPa->add_witness(witness, box); }
    inline std::vector<veritas::Interval> get_box_witness(StateID_type witness) const { return stateRegPa->get_box_witness(witness); }
    void add_transition(const PATransitionStructure& transition, const StateValues& witness_src, const StateValues& witness_suc, std::vector<veritas::Interval> box);
#endif

    inline StateID_type add_witness(const StateValues& witness) { return stateRegPa->add_witness(witness); }
    void add_transition(const PATransitionStructure& transition, const StateValues& witness_src, const StateValues& witness_suc);

    [[nodiscard]] inline State lookup_witness(StateID_type witness) const { return stateRegPa->lookup_witness(witness); }
    [[nodiscard]] inline std::unique_ptr<State> get_witness(StateID_type witness) const { return stateRegPa->get_witness(witness); }

    [[nodiscard]] StateID_type retrieve_witness(StateID_type source, UpdateOpID_type update_op_id, StateID_type successor) const;

    /** Incremental interface. ****************************************************************************************/

    [[nodiscard]] inline SEARCH_SPACE_PA::IncStateSpace& access_inc_ss() { return *incStateSpace; }


/** search node *******************************************************************************************************/

private:
    unsigned int currentStep;
    segmented_vector::SegmentedVector<SEARCH_SPACE_PA::SearchNodeInfo> searchInformation;

public:
    [[nodiscard]] inline std::size_t information_size() const { return searchInformation.size(); } // mainly for debugging purposes

    /**
     * Add node to search space if necessary.
     * Can also be used as normal get_node.
     */
    [[nodiscard]] inline SEARCH_SPACE_PA::SearchNodeInfo& add_node_info(StateID_type state_id) {
        if (state_id >= searchInformation.size()) { searchInformation.resize(state_id + 1); }
        return searchInformation[state_id];
    }

    [[nodiscard]] inline SEARCH_SPACE_PA::SearchNodeInfo& get_node_info(StateID_type state_id) {
        PLAJA_ASSERT(state_id < searchInformation.size())
        return searchInformation[state_id];
    }

    [[nodiscard]] inline const SEARCH_SPACE_PA::SearchNodeInfo& get_node_info(StateID_type state_id) const {
        PLAJA_ASSERT(state_id < searchInformation.size())
        return searchInformation[state_id];
    }

    [[nodiscard]] inline SEARCH_SPACE_PA::SearchNode add_node(std::unique_ptr<AbstractState>&& abstract_state) {
        return { add_node_info(abstract_state->get_id_value()), currentStep, std::move(abstract_state) };
    }

    [[nodiscard]] inline SEARCH_SPACE_PA::SearchNode get_node(StateID_type state_id) {
        return { get_node_info(state_id), currentStep, get_abstract_state(state_id) };
    }

/** goals states *******************************************************************************************************/

private:
    std::list<StateID_type> goalPathFrontier;
    std::size_t numGoalPathStates;

public:
    inline void add_goal_path_frontier(StateID_type state_id) { goalPathFrontier.push_back(state_id); }

    [[nodiscard]] inline bool goal_path_frontier_empty() const { return goalPathFrontier.empty(); }

    [[nodiscard]] inline StateID_type goal_frontier_state() const {
        PLAJA_ASSERT(not goal_path_frontier_empty())
        return *goalPathFrontier.cbegin();
    }

    [[nodiscard]] inline std::size_t number_of_goal_path_states() const { return numGoalPathStates; }

/** post search ********************************************************************************************************/

private:
    [[nodiscard]] AbstractPath extract_path_backwards(StateID_type state_id) const;
    [[nodiscard]] AbstractPath extract_goal_path_forward(StateID_type state_id) const; // !empty path if unsafe source or safe

public:

    std::size_t compute_goal_path_states(); // returns the number of goal path states
    void compute_dead_ends_backwards(StateID_type state_id);
    [[nodiscard]] AbstractPath extract_goal_path(StateID_type state_id) const;

    [[nodiscard]] std::size_t number_of_initial_goal_path_states() const;

    [[nodiscard]] inline bool has_goal_path(StateID_type state_id) const { return get_node_info(state_id).has_goal_path(); }

/** stats *************************************************************************************************************/

public:

    // Statistics:
    void update_statistics() const;
    void print_statistics() const;
    void stats_to_csv(std::ofstream& file) const;
    static void stat_names_to_csv(std::ofstream& file);

};

#endif //PLAJA_SEARCH_SPACE_PA_BASE_H
