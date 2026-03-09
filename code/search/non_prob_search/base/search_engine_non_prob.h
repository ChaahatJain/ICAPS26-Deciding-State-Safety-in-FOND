//
// This file is part of the PlaJA code base.
// Copyright (c) (2019 - 2021) Marcel Vinzent.
// See README.md in the top-level directory for licensing information.
//

#ifndef PLAJA_SEARCH_ENGINE_NON_PROB_H
#define PLAJA_SEARCH_ENGINE_NON_PROB_H

#include <list>
#include "../../../parser/ast/expression/forward_expression.h"
#include "../../../assertions.h"
#include "../../fd_adaptions/search_engine.h"
#include "../../successor_generation/forward_successor_generation.h"
#include "../../states/forward_states.h"
#include "../../using_search.h"
#include "../policy/forward_policy.h"
#include "../forward_non_prob_search.h"

class SearchEngineNonProb: public SearchEngine {

private:
    SearchSpaceBase* searchSpace;

    std::unique_ptr<InitialStatesEnumerator> initialStatesEnumerator;
    std::unique_ptr<SuccessorGenerator> successorGenerator;
    std::unique_ptr<PolicyRestriction> policyRestriction;
    const Expression* goal;
    std::list<StateID_type> frontier;

    /* State space. */
    std::unique_ptr<StateSpaceProb> stateSpace;
    STATE_SPACE::StateInformationProb* currentStateInfo; // Cache for state space maintenance.

    /* Search config. */
    bool searchPerStart;
    bool goalPathSearch;
    bool onlyPolicyTransitions; // To configure behavior of NnExplorer

    [[nodiscard]] bool check_goal(const StateBase& state) const;
    [[nodiscard]] SearchEngine::SearchStatus initialize() final;
    [[nodiscard]] SearchEngine::SearchStatus initialize(StateID_type initial_id);
    [[nodiscard]] SearchEngine::SearchStatus add_start_state();
    SearchStatus step() final;

protected:
    [[nodiscard]] inline const PolicyRestriction* get_policy_restriction() { return policyRestriction.get(); }

    [[nodiscard]] inline const StateSpaceProb* get_state_space() const { return stateSpace.get(); }

    [[nodiscard]] inline StateSpaceProb& access_state_space() {
        PLAJA_ASSERT(stateSpace)
        return *stateSpace;
    }

    inline void set_search_space(SearchSpaceBase* search_space) { searchSpace = search_space; }

    virtual void add_parent_under_policy(SearchNode& node, StateID_type parent) const;

public:
    explicit SearchEngineNonProb(const PLAJA::Configuration& config, bool goal_path_search, bool init_state_space, bool only_policy_transitions);
    ~SearchEngineNonProb() override = 0;
    DELETE_CONSTRUCTOR(SearchEngineNonProb)

    /** Compute goal paths -- after search() is completed. */
    void compute_goal_path_states();

    /* Policy interface. */

    /**
     * Search goal path for provided state.
     * @return the id of the provided state.
     */
    StateID_type search_goal_path(const StateValues& state);

    [[nodiscard]] HeuristicValue_type get_goal_distance(StateID_type state) const;

    [[nodiscard]] UpdateOpID_type get_goal_op(StateID_type state) const;

    /* Aux. */

    [[nodiscard]] struct UpdateOpStructure get_update_op(UpdateOpID_type update_op) const;

    [[nodiscard]] ActionLabel_type get_label(ActionOpID_type op_id) const;

    FCT_IF_DEBUG([[nodiscard]] std::string get_label_name(ActionLabel_type label) const;)

    /* Override stats. */
    void print_statistics() const override;
    void stats_to_csv(std::ofstream& file) const override;
    void stat_names_to_csv(std::ofstream& file) const override;

};

#endif //PLAJA_SEARCH_ENGINE_NON_PROB_H
