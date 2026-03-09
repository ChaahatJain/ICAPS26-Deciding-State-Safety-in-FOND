//
// This file is part of the PlaJA code base.
// Copyright (c) (2019 - 2020) Marcel Vinzent.
// See README.md in the top-level directory for licensing information.
//

#ifndef PLAJA_LRTDP_H
#define PLAJA_LRTDP_H

#include <stack>
#include "prob_search_engine.h"
#include "../using_search.h"

class LRTDP_Base {
protected:
    std::size_t numTrials;

    // Options:
    bool vCurrentIsLower; // to indicate whether v_current or v_alternative (cf. prob_search_node_info.h) is lower bound (differs e.g. for reasonable LRTDP on Pmin vs. Pmax).
    QValue_type epsilon;
    // early-termination:
    QValue_type delta; // interrupt if value is sufficiently determined: V^U(I) - V^L(I) <= delta; for tie-breaking also the tolerance to compute the set of optimal action
    bool tvFlag; // target value flag
    QValue_type targetValue; // interrupt if value is reached by current policy (V^L > targetValue) or is proven to be unreachable (V^U < targetValue)

    // outputs & stats:

    void print_statistics() const;
    void stats_to_csv(std::ofstream& file) const;
    static void stat_names_to_csv(std::ofstream& file) ;

    explicit LRTDP_Base(bool v_current_is_lower);
public:
    virtual ~LRTDP_Base() = 0;

};


template<typename PSearchNodeInfo_t, template<typename> class PSearchNode_t, template<typename, template<typename> class> class PSearchSpace_t, class ValueInitializer>
class LRTDP_: public ProbSearchEngine<PSearchNodeInfo_t, PSearchNode_t, PSearchSpace_t, ValueInitializer>, public LRTDP_Base {
    typedef PSearchNode_t<PSearchNodeInfo_t> PSearchNode;
    typedef ProbSearchEngine<PSearchNodeInfo_t, PSearchNode_t, PSearchSpace_t, ValueInitializer> PSearchEngine;

private:
    bool check_solved(StateID_type id);
    SearchEngine::SearchStatus step() override; // LRTDP-trial

public:
    LRTDP_(const PLAJA::Configuration& config, bool v_current_is_lower):
        ProbSearchEngine<PSearchNodeInfo_t, PSearchNode_t, PSearchSpace_t, ValueInitializer>(config),
        LRTDP_Base(v_current_is_lower) {}
    LRTDP_(const PLAJA::Configuration& config, bool v_current_is_lower,
           ProbSearchEngine<PSearchNodeInfo_t, PSearchNode_t, PSearchSpace_t, ValueInitializer>* parent):
        ProbSearchEngine<PSearchNodeInfo_t, PSearchNode_t, PSearchSpace_t, ValueInitializer>(config, parent),
        LRTDP_Base(v_current_is_lower) {}
    ~LRTDP_() override = default;


    void print_statistics() const override { LRTDP_Base::print_statistics(); PSearchEngine::print_statistics(); }
    void stats_to_csv(std::ofstream& file) const override { LRTDP_Base::stats_to_csv(file); PSearchEngine::stats_to_csv(file); }
    void stat_names_to_csv(std::ofstream& file) const override { LRTDP_Base::stat_names_to_csv(file); PSearchEngine::stat_names_to_csv(file); }

};

template<typename PSearchNodeInfo_t, template<typename> class PSearchNode_t, template<typename, template<typename> class> class PSearchSpace_t, class ValueInitializer>
struct LRTDP: public LRTDP_<PSearchNodeInfo_t, PSearchNode_t, PSearchSpace_t, ValueInitializer> {
    typedef PSearchNode_t<PSearchNodeInfo_t> PSearchNode;

    LRTDP(const PLAJA::Configuration& config, bool v_current_is_lower):
        LRTDP_<PSearchNodeInfo_t, PSearchNode_t, PSearchSpace_t, ValueInitializer>(config, v_current_is_lower) {}
    LRTDP(const PLAJA::Configuration& config, bool v_current_is_lower,
          ProbSearchEngine<PSearchNodeInfo_t, PSearchNode_t, PSearchSpace_t, ValueInitializer>* parent):
          LRTDP_<PSearchNodeInfo_t, PSearchNode_t, PSearchSpace_t, ValueInitializer>(config, v_current_is_lower, parent) {}

    bool check_early_termination() override;
};

template<typename PSearchNodeInfo_t, template<typename> class PSearchNode_t, template<typename, template<typename> class> class PSearchSpace_t, class ValueInitializer>
struct LRTDP_DUAL: public LRTDP_<PSearchNodeInfo_t, PSearchNode_t, PSearchSpace_t, ValueInitializer> {
    typedef PSearchNode_t<PSearchNodeInfo_t> PSearchNode;

    LRTDP_DUAL(const PLAJA::Configuration& config, bool v_current_is_lower):
        LRTDP_<PSearchNodeInfo_t, PSearchNode_t, PSearchSpace_t, ValueInitializer>(config, v_current_is_lower) {}
    LRTDP_DUAL(const PLAJA::Configuration& config, bool v_current_is_lower,
               ProbSearchEngine<PSearchNodeInfo_t, PSearchNode_t, PSearchSpace_t, ValueInitializer>* parent):
               LRTDP_<PSearchNodeInfo_t, PSearchNode_t, PSearchSpace_t, ValueInitializer>(config, v_current_is_lower, parent) {}

    bool check_early_termination() override;
};


/** external definitions **********************************************************************************************/


template<typename PSearchNodeInfo_t, template<typename> class PSearchNode_t, template<typename, template<typename> class> class PSearchSpace_t, class ValueInitializer>
bool LRTDP_<PSearchNodeInfo_t, PSearchNode_t, PSearchSpace_t, ValueInitializer>::check_solved(StateID_type start_id) {
    // std::cout << "check_solved: " << start_id << std::endl;
    bool rv = true;
    std::stack<StateID_type> open;
    std::stack<StateID_type> closed;
    PSearchNode start_node = this->searchSpace.get_node(start_id);

    if (!start_node.is_solved()) {
        open.push(start_id);
        start_node.mark(); // mark is used here to indicate whether state is in open U closed
    }

    // if state *is* solved, stack remain empty and no loops will be entered in the following ...
    while (!open.empty()) {
        // std::cout << "top open:" << open.top() << std::endl;
        PSearchNode state_n = this->searchSpace.get_node(open.top());
        closed.push(open.top());
        open.pop();

        if (state_n.is_fixed()) {
            // std::cout << "... is fixed already" << std::endl;
            continue;
        }
        // check residual
        if (this->adaptive_qVal_update(state_n) > epsilon) { // HAVE TO UPDATE WHEN TERMINATING TRIAL AT EPSILON CONSISTENT STATES!!! (i.p. think of MINCOST traps)
            // std::cout << "... is not epsilon-consistent" << std::endl;
            rv = false;
            continue;
        }
        if (state_n.is_fixed()) { // if (in call above) dead-end is expanded for the first time, or all successors have been identified as don't cares by now
            // std::cout << "... is fixed already" << std::endl;
            continue;
        }

        // explore state
        for (auto per_op_it = this->stateSpace->perOpTransitionIterator(state_n.get_state_id(), state_n.get_policy_choice_index()); !per_op_it.end(); ++per_op_it) {
            PLAJA_ASSERT(per_op_it.prob() > 0) // action op *init* construction should handle such cases
            PSearchNode succ_node = this->searchSpace.get_node(per_op_it.id());
            if (!succ_node.is_solved() && !succ_node.is_marked()) {
                open.push(per_op_it.id());
                succ_node.mark();
            }
        }
    }

    // std::cout << "rv " << rv << std::endl;
    if (rv) {
        while (!closed.empty()) {
            PSearchNode state_n = this->searchSpace.get_node(closed.top());
            state_n.unmark();
            closed.pop();
            state_n.set_solved();
            // std::cout << state_n.get_stateID() << std::endl;
        }
    } else {
        while (!closed.empty()) {
            PSearchNode state_n = this->searchSpace.get_node(closed.top());
            state_n.unmark();
            closed.pop();
            if (!state_n.is_fixed()) {
                // std::cout << state_n.get_stateID() << std::endl;
                this->adaptive_qVal_update(state_n);
            }
        }
    }

    return rv;
}

template<typename PSearchNodeInfo_t, template<typename> class PSearchNode_t, template<typename, template<typename> class> class PSearchSpace_t, class ValueInitializer>
SearchEngine::SearchStatus LRTDP_<PSearchNodeInfo_t, PSearchNode_t, PSearchSpace_t, ValueInitializer>::step() { // implements LRTDP trial

    // Early termination check
    if (this->check_early_termination()) {
        std::cout << "Early-termination check succeeded ..." << std::endl;
        return SearchEngine::SOLVED;
    }

    StateID_type s_id = this->searchSpace.get_initialState().get_id_value();
    PSearchNode initial_node = this->searchSpace.get_node(s_id);

    if (initial_node.is_solved()) {
        this->propertyFulfilled = true; // at this point: as we check early termination just before, we know that target value is not failed
        return SearchEngine::SOLVED;
    }

    ++numTrials;
    if (numTrials % 10000 == 0) {
        std::cout << numTrials << " LRTDP TRIALs until now ..." << std::endl;
    }

    std::stack<StateID_type> visited;

    while (true) {
        PSearchNode state_n = this->searchSpace.get_node(s_id);
        if (state_n.is_solved()) break;

        // insert into visited
        visited.push(s_id);

        // check termination at fixed states
        if (state_n.is_fixed()) {
            // std::cout << s_id << "is already fixed" << std::endl;
            break;
        }

        // pick best action and update
        QValue_type diff = this->adaptive_qVal_update(state_n); // expands if necessary
        if (diff <= epsilon || state_n.is_fixed()) { // TODO ISSUE may have to think about it when trying to handle MAXCOST, may en up with only NaN-differences and thereby non-termination
            // std::cout << "epsilon consistent or fixed, diff:" << diff << std::endl;
            break; // epsilon consistent, respectively fixed as dead-end or don't care
        }

        // stochastically simulate next state
        s_id = this->stateSpace->sample(s_id, state_n.get_policy_choice_index());
        // std::cout << "sampled " << s_id  << " for local operator index " << state_n.get_policy_choice_index() << std::endl;
    }

    while (!visited.empty()) {
        if (!check_solved(visited.top())) break;
        visited.pop();
    }

    return SearchEngine::IN_PROGRESS;
}

// Early termination checks:

template<typename PSearchNodeInfo_t, template<typename> class PSearchNode_t, template<typename, template<typename> class> class PSearchSpace_t, class ValueInitializer>
bool LRTDP<PSearchNodeInfo_t, PSearchNode_t, PSearchSpace_t, ValueInitializer>::check_early_termination() {
    PSearchNode initial_node = this->searchSpace.get_node(this->searchSpace.get_initialState().get_id_value());
    if (this->tvFlag) { // target value specified ?
        // if yes, we can check whether target value is already reached
        if (this->vCurrentIsLower) {
            if (initial_node.get_v_current() > this->targetValue) { // in LRTDP current is admissible, i.e., if current is lower, we check for MIN and exceeding target means failing
                this->propertyFulfilled = false;
                return true;
            }
        } else {
            if (initial_node.get_v_current() < this->targetValue) { // v_current is upper bound, i.e., we compute MAX, and target_value is interpreted as lower bound
                this->propertyFulfilled = false;
                return true;
            }
        }
    }
    return false;
}

//  early termination check for double bound:

template<typename PSearchNodeInfo_t, template<typename> class PSearchNode_t, template<typename, template<typename> class> class PSearchSpace_t, class ValueInitializer>
bool LRTDP_DUAL<PSearchNodeInfo_t, PSearchNode_t, PSearchSpace_t, ValueInitializer>::check_early_termination() {
    PSearchNode initial_node = this->searchSpace.get_node(this->searchSpace.get_initialState().get_id_value());

    if (this->tvFlag) { // target value specified ?
        // if yes, we can check whether target value is already reached
        if (this->vCurrentIsLower) {
            if (initial_node.get_v_current() > this->targetValue) {  // in LRTDP current ist admissible, i.e., if current is lower, we check for MIN and exceeding target means failing
                this->propertyFulfilled = false;
                return true;
            }
            if (initial_node.get_v_alternative() < this->targetValue)  {
                this->propertyFulfilled = true;
                return true;
            }
        } else {
            if (initial_node.get_v_current() < this->targetValue) {
                this->propertyFulfilled = false;
                return true;
            }
            if (initial_node.get_v_alternative() > this->targetValue) {
                this->propertyFulfilled = true; // here we check for MAX, exceeding target means success
                return true;
            }
        }
    }
    // as we have upper and lower bound, we can check if value is sufficiently determined
    if (std::fabs(initial_node.get_v_current() - initial_node.get_v_alternative()) <= this->delta) {
        return this->propertyFulfilled = true;
    }

    return false;
}


#endif //PLAJA_LRTDP_H
