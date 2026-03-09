//
// This file is part of the PlaJA code base.
// Copyright (c) (2019 - 2020) Marcel Vinzent.
// See README.md in the top-level directory for licensing information.
//

#ifndef PLAJA_FRET_H
#define PLAJA_FRET_H

#include <stack>
#include "prob_search_engine.h"
#include "../../utils/fd_imports/hash_extension.h"


/** FRET STRUCTURES ***************************************************************************************************/


using OpSuccessors = std::vector<SuccessorProbPair>;

/**
 * Basic data of a state in the Tarjan algorithm.
 */
struct TarjanNode {
    size_t dfsIndex;
    size_t lowLink;
    bool visited;
    bool onStack;
    bool leaf; // leaf SCC, i.e., no path to fixed state (i.p. goal or dead-end) and also no outgoing transition under considered graph construction
    TarjanNode() : dfsIndex(-1 /*any value*/), lowLink(-1), visited(false), onStack(false), leaf(true) {}
};

/**
 * Node maintained by the Eliminate-Traps-Mechanism to simulate Tarjan recursion.
 */
struct ETNode {
    StateID_type stateID;
    std::vector<LocalOpIndex_type> chs; // list of greedy operators w.r.t. current state value
    LocalOpIndex_type localOpRef; // reference to next local operator
    StateSpaceProb::PerOpTransitionIterator it; // points to successor considered next; preserved during interruption (Tarjan recursive call)
    bool interrupted; // is the node interrupted, i.e., Tarjan proceeding started and was interrupted to proceed successors not proceeded before (Tarjan recursion)

    explicit ETNode(StateID_type state_id) : stateID(state_id), localOpRef(0), it(), interrupted(false) {}
    ETNode(StateID_type state_id, LocalOpIndex_type local_op) : stateID(state_id), localOpRef(0), it(), interrupted(false) {
        chs.push_back(local_op);
    }
    LocalOpIndex_type next_op() {
        if (localOpRef >= chs.size()) {
            return -1;
        } else {
            return chs[localOpRef++];
        }
    }
};

/**********************************************************************************************************************/

class FRET_Base {

protected:
    std::unique_ptr<ProbSearchEngineBase> subEngine; // Find & Revise engine

    // statistics:
    size_t numIters;
    // size_t sizeGv;   // # policy graph nodes; unused legacy
    // size_t sizeGvLeaf; // # policy graph leaf nodes; unused legacy

    const QValue_type epsilon; // consistency criterion
    const bool localTrapElimination;  // construct policy graph for (1) current policy (local); or (2) for all greedy policies of current value
    const QValue_type delta;  // for (2) the precision under which we pick the operators with the optimal q-value

    // trap elimination data
    bool onlyUnitSCCs; // have there been only unit SCC in current trap elimination, i.e., SCC of size one with at least one 100% self-loop operator ?
    QValue_type leafDiff; // largest difference for an updated value, i.e, to test V_[i+1] != V'_i up to arbitrary precision (see step-function)
    bool extendedSearchSpace; // indicates whether elimination of traps did extend the search space
    std::list<StateID_type> policyQueue; // queue used during policy extraction, during trap elimination goals reachable in policy graph must be added

    // tarjan structures
    size_t currentIndex; // tarjan dfs-index
    std::stack<StateID_type> stack; // tarjan stack
    std::stack<ETNode*> queueOfInterrupts; // queue of Tarjan states interrupted, i.e., state for which we first have to proceed successors (we use loops rather than actual recursion)

    // Helper functions to handle successors:

    static inline void apply_abstraction(StateSpaceProb& state_space, const std::set<StateID_type>& scc, StateID_type rep) {
        for (StateID_type it : scc) {
            if (it != rep) {
                state_space.replace_state(it, rep);
            }
        }
    }

    // Add new/old operators to a state (intended usage: old operators have been remove as part of preceeding steps).
    static inline void update_successors(StateSpaceProb& state_space, StateID_type state_id, std::unordered_map<ActionOpID_type, utils::HashSet<OpSuccessors>>& new_state_ops) {
        STATE_SPACE::StateInformationProb& state_info = state_space.get_state_information(state_id);
        for (auto& [op, set_of_successors] : new_state_ops) {
            for (const auto& succs: set_of_successors) {
                state_info.add_operator_completely(op, succs);
            }
        }
    }

    // outputs & stats;
    void print_statistics() const;
    void stats_to_csv(std::ofstream& file) const;
    void stat_names_to_csv(std::ofstream& file) const;

    void set_sub_engine_base(std::unique_ptr<ProbSearchEngineBase>&& sub_engine);

    explicit FRET_Base(const PLAJA::Configuration& config);
public:
    virtual ~FRET_Base() = 0;
    DELETE_CONSTRUCTOR(FRET_Base)

};

/**********************************************************************************************************************/

/**
 * Implements Find & Revise with Elimination of Traps.
 * Tarjan SCC is implemented internally.
 */
template<typename PSearchNodeInfo_t, template<typename> class PSearchNode_t, template<typename, template<typename> class> class PSearchSpace_t, class ValueInitializer>
class FRET: public ProbSearchEngine<PSearchNodeInfo_t, PSearchNode_t, PSearchSpace_t, ValueInitializer>, public FRET_Base {
    typedef PSearchNode_t<PSearchNodeInfo_t> PSearchNode;
    typedef PSearchSpace_t<PSearchNodeInfo_t, PSearchNode_t> PSearchSpace;
    typedef ProbSearchEngine<PSearchNodeInfo_t, PSearchNode_t, PSearchSpace_t, ValueInitializer> PSearchEngine;

/** FRET STRUCTURES ***************************************************************************************************/

    /***
     * Construct OpenElem out of search node for the case where we consider only the current greedy policy graph.
     */
    struct PolicyConstruct {
        ETNode* operator()(PSearchNode state_node) { // for policy graph ? MLV
            return new ETNode(state_node.get_state_id(), state_node.get_policy_choice_index());
        }
        [[nodiscard]] inline bool requires_update() const { return false; }
    };

    /***
     * Construct OpenElem out of search node for the case where we consider the all greedy policy graphs.
     */
    struct ValueConstruct {
    private:
        PSearchSpace& searchSpace;
        const QValue_type delta;
    public:
        ValueConstruct(PSearchSpace& search_space, QValue_type delta) : searchSpace(search_space), delta(delta) {}
        ETNode* operator()(PSearchNode state_node) {
            auto* res = new ETNode(state_node.get_state_id());
            res->chs = searchSpace.compute_greedy_actions(state_node, delta).localOpIndexes; // pick all actions with the optimal q-value up to precision delta
            return res;
        }
        [[nodiscard]] inline bool requires_update() const { return true; } // need to (re-) update here because, we may encounter states not encountered during LRTDP before, hence which may still be epsilon-inconsistent
    };

/**********************************************************************************************************************/

private:
    PolicyConstruct policyConstruct;    // construct for (1)
    ValueConstruct valueConstruct;      // construct for (2)

    // Helper functions to handle successors:

    /**
     * Set state as fixed with self-loop action operator.
     * @param state_node
     * @param action_op_id, the ID of the self-loop action operator (assumed to be not part of the current state information)
     */
    void fix_as_self_loop(PSearchNode& state_node, ActionOpID_type action_op_id) {
        ValueInitializer::dead_end(state_node); // real operator should not be -1
        state_node.mark_fixed();
        this->searchStatistics->inc_attr_unsigned(PLAJA::StatsUnsigned::FIXED_CHOICES);
        // still have to (re)add self-loop to state space
        STATE_SPACE::StateInformationProb& state_info = this->stateSpace->add_state_information(state_node.get_state_id());
        state_info.add_operator_completely(action_op_id, std::vector<SuccessorProbPair>{ {state_node.get_state_id(), 1} });
        state_info.add_parent(state_node.get_state_id());
    }

    // SCC handling:

    void handle_scc(segmented_vector::SegmentedVector<TarjanNode>& data, PSearchNode& state_node);

    // TARJAN proceeding of a state:
    StateID_type proceed(segmented_vector::SegmentedVector<TarjanNode>& data, ETNode& et_node, bool& new_et_node);

    // ELIMINATE TRAPS:
    template<typename Construct_t>
    void eliminate_traps(segmented_vector::SegmentedVector<TarjanNode>& data, StateID_type state_id, Construct_t& construct);

    SearchEngine::SearchStatus initialize() override {
        this->share_timers(subEngine.get());
        return PSearchEngine::initialize();
    }
    SearchEngine::SearchStatus step() override;

    // in general (for non-local-trap-elimination) policy greedy w.r.t to V* may be improper, we can fix that by setting operators in a backward fashion:
    void fix_policy(segmented_vector::SegmentedVector<TarjanNode>& data);

public:
    explicit FRET(const PLAJA::Configuration& config):
            ProbSearchEngine<PSearchNodeInfo_t, PSearchNode_t, PSearchSpace_t, ValueInitializer>(config),
            FRET_Base(config),
            valueConstruct(this->searchSpace, delta) {}
    ~FRET() override = default;

    void print_statistics() const override { FRET_Base::print_statistics(); }
    void stats_to_csv(std::ofstream& file) const override { FRET_Base::stats_to_csv(file); }
    void stat_names_to_csv(std::ofstream& file) const override { FRET_Base::stat_names_to_csv(file); }

    // Set Find & Revise engine
    void set_subEngine(std::unique_ptr<ProbSearchEngineBase>&& sub_engine) override { set_sub_engine_base(std::move(sub_engine)); };
};


/** external definitions **********************************************************************************************/


template<typename PSearchNodeInfo_t, template<typename> class PSearchNode_t, template<typename, template<typename> class> class PSearchSpace_t, class ValueInitializer>
void FRET<PSearchNodeInfo_t, PSearchNode_t, PSearchSpace_t, ValueInitializer>::handle_scc(segmented_vector::SegmentedVector<TarjanNode>& data, PSearchNode& state_node) {
    StateID_type state_ID = state_node.get_state_id();

    if (data[state_ID].leaf) { // is leaf SCC (trap) in (all/current) policy graph? (only those SCC are handled)

        std::set<StateID_type> scc;
        { // construct SCC-set and erase stack accordingly
            StateID_type s_ID;
            do {
                s_ID = stack.top();
                // std::cout << s_ID << std::endl;
                PLAJA_ASSERT(data[s_ID].leaf)
                scc.insert(s_ID);
                data[s_ID].onStack = false;
                stack.pop();
            } while (s_ID != state_ID);

            // std::cout << "root " << state_ID << " " << scc.size() << std::endl;
        }

        // data structures used:
        std::unordered_map<ActionOpID_type, utils::HashSet<OpSuccessors>> new_operators; // operators of abstracted states added to root; also ROOT's operators, i.e., they are first removed and then reinserted
        std::set<StateID_type> united_parents; // all parents over all SCC-states
        std::unordered_set<StateID_type> combined_successors; // all possible successors over all states in SCC over all operators
        bool self_loop = false; // to add abstracted self-loop to state information (see below)
        bool left_out = false; // left an operator out (as only 100% self-loop)
        bool kept_op = false; // kept at least one operator, as not 100% self-loop; if this is not set, state will become absorbing
        ActionOpID_type self_loop_op = ACTION::noneOp; // action op id of the self-loop representation, if existing and applicable (see following code)
        QValue_type min_old_v = std::numeric_limits<QValue_type>::max();
        QValue_type max_old_v = std::numeric_limits<QValue_type>::min();

        // for each state in SCC...
        for (StateID_type s_ID: scc) {
            PSearchNode state_n = this->searchSpace.get_node(s_ID);
            QValue_type old_val = this->searchSpace.get_value(state_n);

            // find minimal, maximal value over all states in SCC
            min_old_v = std::min(old_val, min_old_v);
            max_old_v = std::max(old_val, max_old_v);

            // Collect parents and erase the information for the actual node
            STATE_SPACE::StateInformationProb& state_info = this->stateSpace->add_state_information(s_ID);
            auto& state_info_parents = state_info._parents();
            united_parents.insert(state_info_parents.begin(), state_info_parents.end());
            state_info.clear_backwards_info();

            for (auto app_op_it = this->stateSpace->applicableOpIterator(s_ID); !app_op_it.end(); ++app_op_it) { // for each applicable op ...
                std::vector<SuccessorProbPair> successors;
                bool non_scc = false; // successor not in SCC
                bool self_loop_tmp = false; // self-loop in current op, temporary variable, as global self-loop flag may only be set if we include this op, i.e., if there is at least one non-scc successor

                for (auto succ_it = app_op_it.transitions_of_op(); !succ_it.end(); ++succ_it) { // for each successor of the operator ...
                    if (scc.count(succ_it.id()) > 0) { // successor in scc
                        successors.emplace_back(state_ID, succ_it.prob()); // add representative ID to successor
                        self_loop_tmp = true;
                    } else {
                        combined_successors.insert(succ_it.id()); // store all non-scc successors, later on we use it
                        successors.emplace_back(succ_it.id(), succ_it.prob());
                        non_scc = true;
                    }
                }

                if (non_scc) { // at least one successor not in SCC
                    new_operators[app_op_it.action_op_id()].insert(successors); // ... add this op to new operators
                    self_loop = self_loop || self_loop_tmp; // register self-loop if the operator is actually stored
                } else {
                    self_loop_op = app_op_it.action_op_id(); // 100% self-loop op may be stored if it is actually optimal w.r.t optimization criterion *over other operators*, e.g. Pmin; otherwise it is the very purpose of FRET to delete it
                }
                left_out = left_out || !non_scc; // left at least one operator out -- as it is a 100% self-loop, i.e., if there is no non-scc successor
                kept_op = kept_op || non_scc; // abstract state is non-absorbing
            }
            // delete successor information for abstracted state
            state_info.clear_forwards_info();
        }

        // Handle parents not in SCC, i.p. update their state information
        std::set<StateID_type> combined_parents; // parents not in SCC
        std::set_difference(united_parents.begin(), united_parents.end(), scc.begin(), scc.end(), std::inserter(combined_parents, combined_parents.end()));
        united_parents.clear();
        // For all parents ...
        for (StateID_type parent_ID: combined_parents) {
            for (auto app_op_it = this->stateSpace->applicableOpIterator(parent_ID); !app_op_it.end(); ++app_op_it) { // for each operator ...
                for (auto succ_it = this->stateSpace->perOpTransitionUpdater(parent_ID, app_op_it.local_op_index()); !succ_it.end(); ++succ_it) {
                    if (scc.count(succ_it.id()) > 0) { // if successor is in SCC ...
                        succ_it.update(state_ID); // substitute by representative
                    }
                }
            }
        }
        // if self-loop, representative is also a combined parent
        if (self_loop) {
            combined_parents.insert(state_ID);
        }
        // Update parent information of representative
        STATE_SPACE::StateInformationProb& state_info = this->stateSpace->add_state_information(state_ID);
        state_info._parents().swap(combined_parents);

        // Handle successors, i.p. update their parent information
        for (StateID_type succ_ID: combined_successors) { // for all successors ...
            STATE_SPACE::StateInformationProb& succ_info = this->stateSpace->add_state_information(succ_ID);
            auto& succ_info_parents = succ_info._parents();

            std::set<StateID_type> old_parents;
            succ_info_parents.swap(old_parents);

            // update parents of successor to be non-scc parents plus representative
            std::set_difference(old_parents.begin(), old_parents.end(), scc.begin(), scc.end(), std::inserter(succ_info_parents, succ_info_parents.end()));
            succ_info_parents.insert(state_ID);
        }
        combined_successors.clear();
        update_successors(*this->stateSpace, state_ID, new_operators); // add new/old operators to representative
        new_operators.clear();


        // update value (and policy choice)
        if (!kept_op) { // absorbing
            state_node.mark_as_dead_end();
            ValueInitializer::dead_end(state_node);
            this->searchStatistics->inc_attr_unsigned(PLAJA::StatsUnsigned::DEAD_END_STATES);
        } else { // else normal update
            QValue_type tmp = this->searchSpace.update_q_val(state_node).first;
            // if we have a self-loop operator, we have to check whether choosing self-loop is not worse (i.e. better or as good) as the best choice out of the non-self-loop operators (for PMIN)
            // theoretically such a "optimal" policy is improper (optimization over proper policies), however, for PROB we relax this question anyway, technically treating all absorbing states as goal states with (actual goal) or without (DE) reward.
            if (left_out && !this->searchSpace.qval_is_worse(state_node, ValueInitializer::V_DEADEND, epsilon)) {
                fix_as_self_loop(state_node, self_loop_op);
            } else {
                while (tmp > epsilon) { // make epsilon-consistent (only effect if self-loop)
                    tmp = this->searchSpace.update_q_val(state_node).first;
                }
            }
        }
        // Determine maximal difference compared to any old state value
        QValue_type diff_old = fabs(min_old_v - this->searchSpace.get_value(state_node));
        if (diff_old > leafDiff) {
            leafDiff = diff_old;
        }
        diff_old = fabs(max_old_v - this->searchSpace.get_value(state_node));
        if (diff_old > leafDiff) {
            leafDiff = diff_old;
        }

        apply_abstraction(*this->stateSpace, scc, state_ID);
        onlyUnitSCCs = onlyUnitSCCs && (scc.size() == 1 /*SCC is singleton*/ && !left_out /*no 100% self-loop-operator in SCC*/); // over all SCC in this eliminate-traps step

    } else { // non-leaf SCC
        // erase stack
        StateID_type s_ID;
        do {
            s_ID = stack.top();
            data[s_ID].leaf = false;
            data[s_ID].onStack = false;
            stack.pop();
        } while (s_ID != state_ID);

    }

}

template<typename PSearchNodeInfo_t, template<typename> class PSearchNode_t, template<typename, template<typename> class> class PSearchSpace_t, class ValueInitializer>
StateID_type FRET<PSearchNodeInfo_t, PSearchNode_t, PSearchSpace_t, ValueInitializer>::proceed(segmented_vector::SegmentedVector<TarjanNode>& data, ETNode& et_node, bool& new_et_node) {

    new_et_node = false; // reset, such that only set if a unvisited successor state occurs
    PSearchNode state_n = this->searchSpace.get_node(et_node.stateID);
    TarjanNode& s_data = data[et_node.stateID];
    StateID_type res = -1; // initially arbitrary value, new_et_node indicates whether res is a valid state ID

    while (true) { // for all greedy operators (see while break below) ...
        for (auto it = et_node.it; !it.end(); ++it) { // for each successor...
            TarjanNode& succ_data = data[it.id()];
            if (et_node.interrupted) { // simulates Tarjan returning from a recursive call
                et_node.interrupted = false;
                if (succ_data.lowLink < s_data.lowLink) {
                    s_data.lowLink = succ_data.lowLink;
                }
            }
            else if (succ_data.onStack) { // Tarjan successor already on stack
                if (succ_data.dfsIndex < s_data.lowLink) {
                    s_data.lowLink = succ_data.dfsIndex;
                }
            }
            else if (!succ_data.visited) { // found unvisited successor, simulate recursion by interruption
                new_et_node = true;
                res = it.id();
                et_node.interrupted = true;
                queueOfInterrupts.push(&et_node);
                return res;
            }
            // (initialised) successor only not on stack if fixed or its SCC has been handled before
            // second case "succ not leaf" is transitive application, i.e., we check whether state is part of a leaf SCC (a trap)
            if (!succ_data.onStack || !succ_data.leaf) {
                s_data.leaf = false;
            }
        }

        // proceed with next operator
        LocalOpIndex_type next_op = et_node.next_op();
        if (next_op < 0) { // loop condition: unhandled op
            break;
        }
        et_node.it = this->stateSpace->perOpTransitionIterator(state_n.get_state_id(), next_op); // as we also enter the while loop after interruption, iterator update may only happen at the end
    }


    // Completed Tarjan proceeding for current state ...
    delete(&et_node);
    if (s_data.lowLink == s_data.dfsIndex) { // ROOT found
        handle_scc(data, state_n);
    }
    return res;
}


template<typename PSearchNodeInfo_t, template<typename> class PSearchNode_t, template<typename, template<typename> class> class PSearchSpace_t, class ValueInitializer>
template<typename Construct_t>
void FRET<PSearchNodeInfo_t, PSearchNode_t, PSearchSpace_t, ValueInitializer>::eliminate_traps(segmented_vector::SegmentedVector<TarjanNode>& data, StateID_type state_id, Construct_t& construct) {

    QValue_type old_val;
    bool new_et_node = true; // as long as there is a new ET-node, dfs (Tarjan recursion) continues

    // Loop for the overall Eliminate Traps algorithm (including handling of SCC)
    while (new_et_node || !queueOfInterrupts.empty()) { // continue if there are elements for which Tarjan proceeding is not finished yet

        while (new_et_node) { // simulates Tarjan dfs ...
            new_et_node = false;

            // Initialize Tarjan node:
            PSearchNode state_n = this->searchSpace.get_node(state_id);
            data[state_id].visited = true;
            data[state_id].dfsIndex = currentIndex;
            data[state_id].lowLink = currentIndex++;

            // This state has not been expanded yet, thus extend search space
            if (state_n.is_opened()) {
                PLAJA_ASSERT(construct.requires_update() /*true for value construct*/) // for policy graph in LRTDP we expand all states reachable with the current policy, as all have to be solved, so no need to expand for policy construct
                extendedSearchSpace = true;
                old_val = this->searchSpace.get_value(state_n);
                bool dead_end = this->expand_state(state_n);
                if (dead_end) {
                    old_val = fabs(old_val - this->searchSpace.get_value(state_n)); // temporary abuse of state-value variable old_val as difference-value
                    PLAJA_ASSERT(!std::isnan(old_val)) // TODO ISSUE may have to think about it when trying to handle MAXCOST, currently this yields NaN for which old_val > leafDiff evaluates (in the local setting?) to false
                    if (old_val > leafDiff) {
                        leafDiff = old_val;
                    }
                }
                data.resize(this->searchSpace.number_of_states()); // extend TARJAN data accordingly
            }

            if (!state_n.is_fixed() && construct.requires_update() /*see value construction for explanation*/) {
                old_val = this->searchSpace.get_value(state_n);
                while (this->searchSpace.update_q_val(state_n).first > epsilon); // state value must be epsilon-consistent
                old_val = fabs(old_val - this->searchSpace.get_value(state_n));
                PLAJA_ASSERT(!std::isnan(old_val)) // // TODO ISSUE may have to think about it when trying to handle MAXCOST
                if (old_val > leafDiff) {
                    leafDiff = old_val;
                }
            }

            if (state_n.is_fixed()) {
                if (state_n.is_goal()) {
                    policyQueue.push_back(state_n.get_state_id()); // for policy extraction (in case we terminate after this ET-step)
                }
                break; // do not push fixed, i.p. goals and dead-ends (or don't care states) on stack, SCC with paths to goals or dead-ends are non-leaf SCCs
            }
            PLAJA_ASSERT(state_n.get_policy_choice_index() >= 0)

            PLAJA_ASSERT(construct.requires_update() || state_n.is_solved())

            // Tarjan proceeding ...
            data[state_id].onStack = true;
            stack.push(state_id);
            ETNode* et_node = construct(state_n);
            state_id = proceed(data, *et_node, new_et_node); // proceed Tarjan on this state
        }

        // simulates Tarjan returning from dfs ...
        while (!new_et_node /*then we must simulate recursion again*/ && !queueOfInterrupts.empty()) {
            ETNode* et_node = queueOfInterrupts.top();
            queueOfInterrupts.pop();
            state_id = proceed(data, *et_node, new_et_node); // returns next state to be considered
        }

    }
}

template<typename PSearchNodeInfo_t, template<typename> class PSearchNode_t, template<typename, template<typename> class> class PSearchSpace_t, class ValueInitializer>
SearchEngine::SearchStatus FRET<PSearchNodeInfo_t, PSearchNode_t, PSearchSpace_t, ValueInitializer>::step() {

    ++numIters; // stats
    if (numIters % 100 == 0) std::cout << numIters << " FRET iterations until now ..." << std::endl;

    // FIND & REVISE:
    subEngine->search_internal(); // TODO for now subengine do not have to initialize
    PLAJA_ASSERT(subEngine->get_status() != SearchEngine::IN_PROGRESS)
    if (subEngine->get_status() != SearchEngine::SOLVED) return subEngine->get_status();
    // else subEngine is solved:

    if (subEngine->check_early_termination()) {
        this->propertyFulfilled = subEngine->is_propertyFulfilled();
        return SearchEngine::SOLVED;
    }

    // if (numIters % 100 == 0) std::cout << "FRET eliminate traps" << std::endl;

    // set progress conditions
    onlyUnitSCCs = true;
    leafDiff = 0;
    extendedSearchSpace = false;
    policyQueue.clear();

    // Tarjan data:
    currentIndex = 0;
    segmented_vector::SegmentedVector<TarjanNode> data; // Tarjan information
    data.resize(this->searchSpace.number_of_states());

    // ELIMINATE TRAPS:
    StateID_type initial_ID = this->searchSpace.get_initialState().get_id_value();
    if (localTrapElimination) { // consider only current greedy policy graph
        eliminate_traps(data, initial_ID, policyConstruct);
    } else { // or all possible greedy policy graph
        eliminate_traps(data, initial_ID, valueConstruct);
    }

    // Solved ?
    if (!onlyUnitSCCs/*at least one non-singleton SCC or singleton with a 100% self-loop operator*/ || leafDiff > epsilon /*V_i+1 != V'_i*/ || extendedSearchSpace) {
        this->searchSpace.increase_current_step(); // each node in the underlying Find & Revise step will have to be solved anew ...
        return SearchEngine::IN_PROGRESS;
    }
    // else: SOLVED
    this->propertyFulfilled = subEngine->is_propertyFulfilled();
    if (!localTrapElimination) fix_policy(data);
    return SearchEngine::SOLVED;
}

template<typename PSearchNodeInfo_t, template<typename> class PSearchNode_t, template<typename, template<typename> class> class PSearchSpace_t, class ValueInitializer>
void FRET<PSearchNodeInfo_t, PSearchNode_t, PSearchSpace_t, ValueInitializer>::fix_policy(segmented_vector::SegmentedVector<TarjanNode>& data) {

    std::cout << "FRET: fix policy" << std::endl;
    segmented_vector::SegmentedVector<bool> fixed;
    fixed.resize(this->searchSpace.number_of_states(), false);

    while (!policyQueue.empty()) {
        StateID_type successor_ID = policyQueue.front();
        policyQueue.pop_front();
        const STATE_SPACE::StateInformationProb& succ_info = this->stateSpace->add_state_information(successor_ID);
        for (StateID_type parent_ID: succ_info._parents()) {
            if (data[parent_ID].visited && !fixed[parent_ID]) { // is parent part of policy graph
                PSearchNode parent_n = this->searchSpace.get_node(parent_ID);
                QValLocalOps qval_ops = this->searchSpace.compute_greedy_actions(parent_n, delta); // implicitly assumes non-local policy-graph, however calling this function after local trap elimination we just (re-)establish the proper policy
                const STATE_SPACE::StateInformationProb& parent_info = this->stateSpace->add_state_information(parent_ID);
                for (LocalOpIndex_type local_ch: qval_ops.localOpIndexes) {
                    if (parent_info.has_successor(local_ch, successor_ID)) {
                        parent_n.set_policy_choice_index(local_ch);
                        policyQueue.push_back(parent_ID); // may only push if choice is actually set as policy correction relies on a goal-backwards propagation
                        // moreover, it is only such states that need to be fixed, i.e., if a state is not reachable from goal by policy graph, its choice is correct already
                        fixed[parent_ID] = true; // mark states already added to queue
                        break;
                    }
                    // no problem (should only happen if reaching goal is not desirable e.g. for Pmin) then any greedy operator is valid (as for Pmin getting stuck in "non-leaf" cycles is not suboptimal)
                    // no need to set choice, as we already chose one during search
                    // successor is not contained in greedy operators
                }
            }
        }

    }
}

#endif //PLAJA_FRET_H
