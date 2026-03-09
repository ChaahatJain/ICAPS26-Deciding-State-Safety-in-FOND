//
// This file is part of the PlaJA code base.
// Copyright (c) (2019 - 2020) Marcel Vinzent.
// See README.md in the top-level directory for licensing information.
//

#ifndef PLAJA_PROB_SEARCH_ENGINE_H
#define PLAJA_PROB_SEARCH_ENGINE_H

#include "../../parser/ast/expression/expression.h"
#include "../../stats/stats_base.h"
#include "../fd_adaptions/search_engine.h"
#include "../fd_adaptions/state_registry.h"
#include "../successor_generation/successor_generator.h"
#include "value_initializer.h"
#include "prob_search_space.h"
#include "state_space.h"

class ProbSearchEngineBase: public SearchEngine {
private:

    const ProbSearchSpaceBase* searchSpaceBase; // base reference of child class

    /* Goal. */
    const Expression* goal;

protected:
    StateSpaceProb* stateSpace;

    /* Cost. */
    const Expression* cost;

    // output flag
    bool outputPolicyFlag;
    bool outputStateSpaceFlag;

    bool propertyFulfilled; // is the property fulfilled ?
    // shared references, e.g., for FRET/LRTDP shared between FRET and LRTDP as the both write/read runtime information
    std::unique_ptr<SuccessorGenerator> successorGeneratorOwn;
    SuccessorGenerator& successorGenerator;
    // std::unique_ptr<SearchStatistics> searchStatisticsOwn;
    // SearchStatistics& searchStatistics;

    /* Construction auxiliaries. */
    void set_search_space(ProbSearchSpaceBase& search_space_base); // non-const reference due to access to state space (see source file)
    static const ModelInformation& extract_model_info(const Model& model);

    [[nodiscard]] bool check_goal(const StateBase& state) const;

    explicit ProbSearchEngineBase(const PLAJA::Configuration& config);
    ProbSearchEngineBase(const PLAJA::Configuration& config, const ProbSearchEngineBase& parent);
public:
    ~ProbSearchEngineBase() override = 0;
    DELETE_CONSTRUCTOR(ProbSearchEngineBase)

    [[nodiscard]] inline bool is_propertyFulfilled() const { return propertyFulfilled; } // parent engine might need it

    // virtual function simplifies factory implementation of e.g. FRET
    virtual void set_subEngine(std::unique_ptr<ProbSearchEngineBase>&& sub_engine);

    /**
     * Engine may implement an early termination check which can also be called by the parent engine.
     * @return
     */
    virtual bool check_early_termination();

    // override stats:
    void print_statistics() const override;
    void stats_to_csv(std::ofstream& file) const override;
    void stat_names_to_csv(std::ofstream& file) const override;
};

/** template **********************************************************************************************************/

template<typename PSearchNodeInfo_t, template<typename> class PSearchNode_t, template<typename, template<typename> class> class PSearchSpace_t, class ValueInitializer>
class ProbSearchEngine: public ProbSearchEngineBase {
    typedef PSearchNode_t<PSearchNodeInfo_t> PSearchNode;
    typedef PSearchSpace_t<PSearchNodeInfo_t, PSearchNode_t> PSearchSpace;

protected:
    std::unique_ptr<PSearchSpace> searchSpaceOwn; // shared reference, see respective structures in ProbSearchEngineBase
    PSearchSpace& searchSpace;

    /**
     * Expand state and add information to state space.
     * Must only be called on open states nodes, i.e., not already expanded.
     * @param state_node
     * @return , false if state is absorbing.
     */
    bool expand_state(PSearchNode& state_node);
    /**
     * Compute q-value and corresponding action w.r.t to current state values of successor states
     * and update state node respectively.
     * Adaptive: state is expanded if necessary.
     * @param state_node, must not be called on already fixed states
     * @return difference between old and new state value
     */
    QValue_type adaptive_qVal_update(PSearchNode& state_node);

    SearchStatus initialize() override; // may be overridden, but must be called by child implementation // TODO "re-initialize" function for subengines ?
public:
    explicit ProbSearchEngine(const PLAJA::Configuration& config):
        ProbSearchEngineBase(config)
        , searchSpaceOwn(new PSearchSpace(extract_model_info(*model)))
        , searchSpace(*searchSpaceOwn) {
        this->set_search_space(searchSpace);
    }

    ProbSearchEngine(const PLAJA::Configuration& config,
                     ProbSearchEngine<PSearchNodeInfo_t, PSearchNode_t, PSearchSpace_t, ValueInitializer>* parent):
        ProbSearchEngineBase(config, *parent)
        , searchSpaceOwn(nullptr)
        , searchSpace(parent->searchSpace) {
        this->set_search_space(searchSpace);
    }

    ~ProbSearchEngine() override = 0;
    DELETE_CONSTRUCTOR(ProbSearchEngine)

    void print_statistics() const override {
        // Debugging:
        if (searchSpaceOwn && outputStateSpaceFlag) { stateSpace->dump_state_space(); }
        if (searchSpaceOwn && outputPolicyFlag) { searchSpaceOwn->dump_policy_graph(searchSpaceOwn->get_initialState().get_id_value()); }
        //
        ProbSearchEngineBase::print_statistics();
    }

};

/** external definitions **********************************************************************************************/


template<typename PSearchNodeInfo_t, template<typename> class PSearchNode_t, template<typename, template<typename> class> class PSearchSpace_t, class ValueInitializer>
ProbSearchEngine<PSearchNodeInfo_t, PSearchNode_t, PSearchSpace_t, ValueInitializer>::~ProbSearchEngine() = default;

template<typename PSearchNodeInfo_t, template<typename> class PSearchNode_t, template<typename, template<typename> class> class PSearchSpace_t, class ValueInitializer>
bool ProbSearchEngine<PSearchNodeInfo_t, PSearchNode_t, PSearchSpace_t, ValueInitializer>::expand_state(PSearchNode& state_node) {
    PLAJA_ASSERT(state_node.is_opened())

    StateID_type state_id = state_node.get_state_id();
    STATE_SPACE::StateInformationProb& state_info = stateSpace->add_state_information(state_id);

    // Expand state
    State current = searchSpace.get_state(state_id);
    state_node.close();
    searchStatistics->inc_attr_unsigned(PLAJA::StatsUnsigned::EXPANDED_STATES);

    // Explore all transitions from this state
    successorGenerator.explore(current);

    // Dead-end check
    std::size_t number_of_applicable_ops = successorGenerator._number_of_enabled_ops();

    if (number_of_applicable_ops) { searchStatistics->inc_attr_unsigned(PLAJA::StatsUnsigned::ACTION_OPS, number_of_applicable_ops); }
    else { // numberOfEnabledOps == 0
        PLAJA_ASSERT(!state_node.is_goal())
        state_node.mark_as_dead_end();
        ValueInitializer::dead_end(state_node);
        searchStatistics->inc_attr_unsigned(PLAJA::StatsUnsigned::DEAD_END_STATES); // also a dead-end, since goal states will not be expanded
        searchStatistics->inc_attr_unsigned(PLAJA::StatsUnsigned::TERMINAL_STATES);
        return false;
    }

    // Store information in state space & add nodes of successors.
    // initialize structures ...
    state_info.reserve(number_of_applicable_ops);
    for (auto op_it = successorGenerator.init_action_op_it_per_explore(); !op_it.end(); ++op_it) { // for each operator ...
        const ActionOpInit& action_op = *op_it;
        state_info.add_operator(action_op._op_id(), action_op.size());
        searchStatistics->inc_attr_unsigned(PLAJA::StatsUnsigned::TRANSITIONS, action_op.size());
        for (auto transition_it = action_op.transitionIterator(); !transition_it.end(); ++transition_it) { // for each possible successor of an operator
            std::unique_ptr<State> successor = this->successorGenerator.compute_successor(transition_it.transition(), current);
            searchStatistics->inc_attr_unsigned(PLAJA::StatsUnsigned::GENERATED_STATES);
            PSearchNode successor_node = searchSpace.add_node(*successor);
            if (successor_node.is_new()) {
                searchStatistics->inc_attr_unsigned(PLAJA::StatsUnsigned::REACHABLE_STATES);
                successor_node.set_cost(cost->evaluateFloating(successor.get())); // set cost of this state, i.e., just the value the cost expression evaluates to
                if (check_goal(*successor)) { // goal can be checked immediately, thus not checked e.g. in adaptive_qVal-functions, in contrast to dead-end
                    successor_node.set_goal();
                    ValueInitializer::goal(successor_node);
                    searchStatistics->inc_attr_unsigned(PLAJA::StatsUnsigned::GOAL_STATES);
                } else {
                    ValueInitializer::set(successor_node);
                    successor_node.set_open();
                }
            }
            // update state space
            StateID_type successor_id = successor_node.get_state_id();
            STATE_SPACE::StateInformationProb& succ_info = stateSpace->add_state_information(successor_id);
            if (succ_info.is_abstracted()) { // if state is expanded after successor has already been abstracted
                STATE_SPACE::StateInformationProb& actual_succ_info = stateSpace->add_state_information(succ_info.abstractedTo);
                state_info.add_successor_to_current_op(succ_info.abstractedTo, transition_it.prob());
                actual_succ_info.add_parent(state_id); // add parent
            } else {
                state_info.add_successor_to_current_op(successor_id, transition_it.prob());
                succ_info.add_parent(state_id);
            }
        }
    }
    //
    return true;

}

template<typename PSearchNodeInfo_t, template<typename> class PSearchNode_t, template<typename, template<typename> class> class PSearchSpace_t, class ValueInitializer>
QValue_type ProbSearchEngine<PSearchNodeInfo_t, PSearchNode_t, PSearchSpace_t, ValueInitializer>::adaptive_qVal_update(PSearchNode& state_node) {
    PLAJA_ASSERT(state_node.is_opened() || state_node.is_closed())
    QValue_type old_v = searchSpace.get_value(state_node);
    // Expand if necessary
    if (state_node.is_closed() || expand_state(state_node)) {  // expand is false = (absorbing) dead-end
        // compute q-value
        searchSpace.update_q_val(state_node);
        if (ValueInitializer::is_dont_care(state_node)) { // all successors already have dont_care value
            ValueInitializer::dont_care(state_node); // set not just the value (redundant) but also local operator (-1)
            state_node.set_dont_care();
            searchStatistics->inc_attr_unsigned(PLAJA::StatsUnsigned::DONT_CARE_STATES);
        }
    }
    return std::fabs(old_v - searchSpace.get_value(state_node));
}

template<typename PSearchNodeInfo_t, template<typename> class PSearchNode_t, template<typename, template<typename> class> class PSearchSpace_t, class ValueInitializer>
SearchEngine::SearchStatus ProbSearchEngine<PSearchNodeInfo_t, PSearchNode_t, PSearchSpace_t, ValueInitializer>::initialize() {
    if (searchSpaceOwn) {
        const State& initial_state = searchSpace.get_initialState();
        this->searchStatistics->inc_attr_unsigned(PLAJA::StatsUnsigned::GENERATED_STATES);
        this->searchStatistics->inc_attr_unsigned(PLAJA::StatsUnsigned::START_STATES);
        this->searchStatistics->inc_attr_unsigned(PLAJA::StatsUnsigned::REACHABLE_STATES);
        PSearchNode init_node = searchSpace.add_node(initial_state);
        init_node.set_cost(cost->evaluateFloating(&initial_state)); // set cost of this state, i.e., just the value the cost expression evaluates
        ValueInitializer::set(init_node);
        init_node.set_open();
    }
    return SearchStatus::IN_PROGRESS;
}

#endif //PLAJA_PROB_SEARCH_ENGINE_H
