//
// This file is part of the PlaJA code base.
// Copyright (c) (2019 - 2020) Marcel Vinzent.
// See README.md in the top-level directory for licensing information.
//

#ifndef PLAJA_STATE_SPACE_H
#define PLAJA_STATE_SPACE_H

#include <unordered_map>
#include <list>
#include <vector>
#include <set>
#include <memory>
#include "../../utils/fd_imports/segmented_vector.h"
#include "../../utils/structs_utils/update_op_structure.h"
#include "../../utils/structs_utils/prob_op_structure.h"
#include "../../utils/structs_utils/transition_info.h"
#include "../../utils/utils.h"
#include "../../assertions.h"
#include "../factories/forward_factories.h"


/**
 * Maintains state space constructed so far, i.e.,
 * transitions of each state per operator (and by probability or update index),
 * abstraction structure (abstractedTo & reverseAbstraction) to collapse state sets (e.g. SCC in Tarjan).
 * The structure works completely on StateID-type (integer) not state(-values).
 *
 */

namespace STATE_SPACE {

#if 0
    struct ParentInformation{
        // parent state with local operator and update index to the child state:
        StateID_type parentID;
        LocalOpIndex_type localOp;
        UpdateIndex_type updateIndex;
        ParentInformation(StateID_type parent_ID, LocalOpIndex_type local_op, UpdateIndex_type update_index): parentID(parent_ID), localOp(local_op), updateIndex(update_index) {}
    };
#endif

    template<typename TInfo_t>
    struct StateInformationBase {
        typedef std::pair<StateID_type, TInfo_t> TransitionInfo_t;

    protected:
        // FORWARD STRUCTURES
        std::vector<std::vector<TransitionInfo_t>> transitions; // transition info per action op and update
        std::vector<ActionOpID_type> actionOps; // IDs of the local operators
        // BACKWARD STRUCTURE
        std::set<StateID_type> parents;

    public:
        StateInformationBase() = default;
        virtual ~StateInformationBase() = default;
        DEFAULT_CONSTRUCTOR_ONLY(StateInformationBase)
        DELETE_ASSIGNMENT(StateInformationBase)

        inline void reserve(std::size_t num_operators) {
            transitions.reserve(num_operators);
            actionOps.reserve(num_operators);
        }

        inline void reserve(LocalOpIndex_type local_op, std::size_t num_transitions) {
            PLAJA_ASSERT(0 <= local_op && local_op < transitions.size())
            transitions[local_op].reserve(num_transitions);
        }

        inline void clear_forwards_info() {
            actionOps.clear();
            transitions.clear();
        }

        inline void clear_backwards_info() { parents.clear(); }

        [[nodiscard]] inline const std::vector<std::vector<TransitionInfo_t>>& _transitions() const { return transitions; }

        [[nodiscard]] inline const std::vector<TransitionInfo_t>& _transitions(LocalOpIndex_type local_op) const {
            PLAJA_ASSERT(0 <= local_op && local_op < transitions.size())
            return transitions[local_op];
        }

        inline std::vector<TransitionInfo_t>& _transitions(LocalOpIndex_type local_op) {
            PLAJA_ASSERT(0 <= local_op && local_op < transitions.size())
            return transitions[local_op];
        } // due to update iterator

        [[nodiscard]] inline const std::vector<ActionOpID_type>& _action_ops() const { return actionOps; }

        [[nodiscard]] inline const ActionOpID_type& _action_op(LocalOpIndex_type local_op) const { return actionOps[local_op]; }

        [[nodiscard]] inline const std::set<StateID_type>& _parents() const { return parents; }

        [[nodiscard]] inline std::set<StateID_type>& _parents() { return parents; } // due to usage in FRET

        [[nodiscard]] inline std::size_t number_of_transitions() const {
            std::size_t num_transitions = 0;
            for (const auto& transitions_per_op: transitions) { num_transitions += transitions_per_op.size(); }
            return num_transitions;
        }

        [[nodiscard]] inline std::size_t number_of_action_ops() const { return actionOps.size(); }

        [[nodiscard]] inline std::set<StateID_type> collect_successors() const {
            std::set<StateID_type> collected_successors;
            for (const auto& transitions_per_op: transitions) {
                for (const auto& transition: transitions_per_op) {
                    collected_successors.insert(transition.first);
                }
            }
            return collected_successors;
        }

        [[nodiscard]] inline std::size_t number_of_successors() const { return collect_successors().size(); }

        // find:

        [[nodiscard]] std::tuple<LocalOpIndex_type, ActionOpID_type, TInfo_t> find_transition(StateID_type successor_id) const {
            std::size_t num_ops = number_of_action_ops();
            for (LocalOpIndex_type local_op = 0; local_op < num_ops; ++local_op) {
                for (const auto& transition_info: transitions[local_op]) {
                    if (transition_info.first == successor_id) { return { local_op, actionOps[local_op], transition_info.second }; }
                }
            }
            // return none:
            if constexpr (std::is_same_v<TransitionInfo_t, SuccessorProbPair>) {
                return { ACTION::noneLocalOp, ACTION::noneOp, PLAJA::noneProb };
            } else {
                static_assert(std::is_same_v<TransitionInfo_t, SuccessorUpdatePair>);
                return { ACTION::noneLocalOp, ACTION::noneOp, ACTION::noneUpdate };
            }
        }

        // for readability

        [[nodiscard]] inline ProbOpStructure find_prob(StateID_type successor_id) const {
            if constexpr (std::is_same_v<TransitionInfo_t, SuccessorProbPair>) {
                auto transition_info = find_transition(successor_id);
                return { std::get<1>(transition_info), std::get<2>(transition_info) };
            } else { PLAJA_ABORT }
        }

        [[nodiscard]] inline UpdateOpStructure find_update(StateID_type successor_id) const {
            if constexpr (std::is_same_v<TransitionInfo_t, SuccessorUpdatePair>) {
                auto transition_info = find_transition(successor_id);
                return { std::get<1>(transition_info), std::get<2>(transition_info) };
            } else { PLAJA_ABORT }
        }

        //

        [[nodiscard]] inline LocalOpIndex_type find_action_op(ActionOpID_type action_op_id) const {
            LocalOpIndex_type index = 0;
            for (auto op: actionOps) {
                if (action_op_id == op) {
                    return index;
                }
                ++index;
            }
            return ACTION::noneLocalOp;
        }

        [[nodiscard]] inline bool has_successor(LocalOpIndex_type local_op, StateID_type successor_id) const { // NOLINT(*-easily-swappable-parameters)
            const auto& transitions_per_op = _transitions(local_op);
            return std::any_of(transitions_per_op.cbegin(), transitions_per_op.cend(), [successor_id](const SuccessorProbPair& transition) { return transition.first == successor_id; });
        }

        // construction:

        inline void add_successor(LocalOpIndex_type local_op, StateID_type successor_id, TInfo_t info) { _transitions(local_op).emplace_back(successor_id, info); }

        inline void add_successor(ActionOpID_type action_op_id, StateID_type successor_id, TInfo_t info) { add_successor(find_action_op(action_op_id), successor_id, info); }

        /**
         * Add successor to the last action operator added.
         * @param successor
         * @param info
         */
        inline void add_successor_to_current_op(StateID_type successor, TInfo_t info) {
            PLAJA_ASSERT(!transitions.empty())
            transitions.back().emplace_back(successor, info);
        }

        inline LocalOpIndex_type add_operator(ActionOpID_type action_op, std::size_t num_transitions) { // NOLINT(*-easily-swappable-parameters)
            transitions.emplace_back();
            transitions.back().reserve(num_transitions);
            actionOps.push_back(action_op);
            return PLAJA_UTILS::cast_numeric<LocalOpIndex_type>(actionOps.size() - 1);
        }

        inline LocalOpIndex_type add_operator_completely(ActionOpID_type action_op_id, std::vector<TransitionInfo_t> transitions_) {
            transitions.push_back(std::move(transitions_));
            actionOps.push_back(action_op_id);
            return PLAJA_UTILS::cast_numeric<LocalOpIndex_type>(actionOps.size() - 1);
        }

        inline bool add_parent(StateID_type parent_id) { return parents.insert(parent_id).second; }

    };

}

template<typename StateInformation_t, typename TInfo_t>
class StateSpaceBase {
    typedef std::pair<StateID_type, TInfo_t> TransitionInfo_t;

protected:
    segmented_vector::SegmentedVector<StateInformation_t> cachedStateInformation;
    // The indexing is dense when properly used, i.e.,
    // state_registry.* assigns ids dense to registered states
    // and under proper usage each state is also registered in the state space
    // note however, e.g., under PA unreachable states may be registered
    // ... thus we maintain the number of reachable state explicitly

public:
    StateSpaceBase() = default;
    virtual ~StateSpaceBase() = default;
    DELETE_CONSTRUCTOR(StateSpaceBase)

    /**
     * The number of state for which there is information.
     * @return
     */
    [[nodiscard]] inline std::size_t information_size() const {
        return cachedStateInformation.size();
    }

    /** Get *existing* state information structure for the respective state ID. */
    inline StateInformation_t& get_state_information(StateID_type id) {
        PLAJA_ASSERT(id < cachedStateInformation.size())
        return cachedStateInformation[id];
    }

    [[nodiscard]] inline const StateInformation_t& get_state_information(StateID_type id) const {
        PLAJA_ASSERT(id < cachedStateInformation.size())
        return cachedStateInformation[id];
    }

    /**
     *  Get state information structure for the respective state ID.
     *  State space structure is extended if necessary.
     */
    inline StateInformation_t& add_state_information(StateID_type id) {
        std::size_t l = cachedStateInformation.size();
        if (id >= l) {
            cachedStateInformation.resize(id + 1);
        }
        return cachedStateInformation[id];
    }

    // state info construction for convenience:

    inline bool add_parent(StateID_type id, StateID_type parent_id) {
        return add_state_information(id).add_parent(parent_id);
    }

    // iterators:

    // Iterator over transitions of a state for a certain local operator.
    class PerOpTransitionIterator { // TODO maybe explicit assignment operator as we use it in FRET
        friend class StateSpaceBase<StateInformation_t, TInfo_t>; //
        friend class ApplicableOpIterator; //
        friend class TransitionIterator;

    private:
        const std::vector<TransitionInfo_t>* transitions;
        unsigned int local_ref;

        explicit PerOpTransitionIterator(const std::vector<TransitionInfo_t>& transitions_):
            transitions(&transitions_)
            , local_ref(0) {}

    public:
        PerOpTransitionIterator():
            transitions(nullptr)
            , local_ref(-1) {} // <- thus transitions pointer rather than reference
        DEFAULT_CONSTRUCTOR(PerOpTransitionIterator)

        inline void operator++() { ++local_ref; }

        [[nodiscard]] inline bool end() const { return transitions == nullptr || local_ref >= transitions->size(); }

        [[nodiscard]] inline StateID_type id() const { return (*transitions)[local_ref].first; }

        [[nodiscard]] inline TInfo_t info() const { return (*transitions)[local_ref].second; }

        // for readability
        [[nodiscard]] inline PLAJA::Prob_type prob() const {
            if constexpr (std::is_same_v<TransitionInfo_t, SuccessorProbPair>) { return info(); } else { PLAJA_ABORT }
        }

        [[nodiscard]] inline UpdateIndex_type update() const {
            if constexpr (std::is_same_v<TransitionInfo_t, SuccessorUpdatePair>) { return info(); } else { PLAJA_ABORT }
        }
    };

    // *Non*-const iterator over transitions of a state for a certain local operator.
    class PerOpTransitionUpdater {
        friend class StateSpaceBase<StateInformation_t, TInfo_t>;

    private:
        std::vector<TransitionInfo_t>* transitions;
        unsigned int local_ref;

        explicit PerOpTransitionUpdater(std::vector<TransitionInfo_t>& transitions_):
            transitions(&transitions_)
            , local_ref(0) {}

    public:
        DELETE_CONSTRUCTOR(PerOpTransitionUpdater)

        inline void operator++() { ++local_ref; }

        [[nodiscard]] inline bool end() const { return transitions == nullptr || local_ref >= transitions->size(); }

        inline void update(StateID_type id) const { (*transitions)[local_ref].first = id; }

        [[nodiscard]] inline StateID_type id() const { return (*transitions)[local_ref].first; }

        [[nodiscard]] inline TInfo_t info() const { return (*transitions)[local_ref].second; }

        // for readability
        [[nodiscard]] inline PLAJA::Prob_type prob() const {
            if constexpr (std::is_same_v<TransitionInfo_t, SuccessorProbPair>) { return info(); } else { PLAJA_ABORT }
        }

        [[nodiscard]] inline UpdateIndex_type update() const {
            if constexpr (std::is_same_v<TransitionInfo_t, SuccessorUpdatePair>) { return info(); } else { PLAJA_ABORT }
        }
    };

    /**
     * Iterator for transitions of the respective state for the given local operator index.
     * @param id, must have already been "added" to state space.
     * @param local_op, must be valid w.r.t to state info structure stored in state space.
     * @return
     */
    [[nodiscard]] inline PerOpTransitionIterator perOpTransitionIterator(StateID_type id, LocalOpIndex_type local_op) const {
        return PerOpTransitionIterator(get_state_information(id)._transitions(local_op));
    }

    /**
    * Iterator for transitions of the respective state for the given local operator index. Allows to update the values.
    * @param id, must have already been "added" to state space.
    * @param local_op, must be valid w.r.t to state info structure stored in state space.
    * @return
    */
    inline PerOpTransitionUpdater perOpTransitionUpdater(StateID_type id, LocalOpIndex_type local_op) {
        return PerOpTransitionUpdater(get_state_information(id)._transitions(local_op));
    }

    // Iterate over action operators applicable in a state.
    class ApplicableOpIterator {
        friend class StateSpaceBase<StateInformation_t, TInfo_t>; //
        friend class TransitionIterator;

    private:
        const std::vector<std::vector<TransitionInfo_t>>* transitions;
        const std::vector<ActionOpID_type>* actionOps;
        LocalOpIndex_type local_ref;

        explicit ApplicableOpIterator(const StateInformation_t& state_info):
            transitions(&state_info._transitions())
            , actionOps(&state_info._action_ops())
            , local_ref(0) {}

    public:
        inline void operator++() { local_ref++; }

        [[nodiscard]] inline bool end() const { return local_ref >= actionOps->size(); }

        [[nodiscard]] inline LocalOpIndex_type local_op_index() const { return local_ref; }

        [[nodiscard]] inline ActionOpID_type action_op_id() const { return (*actionOps)[local_ref]; }

        [[nodiscard]] inline PerOpTransitionIterator transitions_of_op() const { return PerOpTransitionIterator((*transitions)[local_ref]); }

    };

    [[nodiscard]] inline ApplicableOpIterator applicableOpIterator(StateID_type id) const {
        return ApplicableOpIterator(get_state_information(id));
    }

    // Just iterate over all transitions.
    class TransitionIterator {
        friend class StateSpaceBase<StateInformation_t, TInfo_t>;

    private:
        ApplicableOpIterator opIterator;
        PerOpTransitionIterator perOpTransitionIt;

        explicit TransitionIterator(const StateInformation_t& state_info):
            ApplicableOpIterator(state_info)
            , PerOpTransitionIterator() {
            if (!opIterator) { perOpTransitionIt = opIterator.transitions_of_op(); }

            while (perOpTransitionIt.end() && !opIterator.end()) {
                ++opIterator;
                perOpTransitionIt = opIterator.transitions_of_op();
            }
        }

    public:
        DELETE_CONSTRUCTOR(TransitionIterator)

        inline void operator++() {
            ++perOpTransitionIt;
            while (perOpTransitionIt.end() && !opIterator.end()) {
                ++opIterator;
                perOpTransitionIt = opIterator.transitions_of_op();
            }
        }

        [[nodiscard]] inline bool end() const { return !opIterator.end(); }

        [[nodiscard]] inline LocalOpIndex_type local_op_index() const { return opIterator.local_op_index(); }

        [[nodiscard]] inline ActionOpID_type action_op_id() const { return opIterator.action_op_id(); }

        [[nodiscard]] inline const PerOpTransitionIterator& transition() const { return perOpTransitionIt; }
    };

    /**
     * Iterator for all transitions of the respective state.
     * @param id, must have already been "added" to state space.
     * @return
     */
    [[nodiscard]] inline TransitionIterator transitionIterator(StateID_type id) const {
        return TransitionIterator(get_state_information(id));
    }

};


/**********************************************************************************************************************/


namespace STATE_SPACE {

    struct StateInformationProb: public StateInformationBase<PLAJA::Prob_type> {
        StateID_type abstractedTo; // state id of the state this state is abstracted to

    public:
        StateInformationProb():
            StateInformationBase<PLAJA::Prob_type>()
            , abstractedTo(STATE::none) {};
        ~StateInformationProb() override = default;
        DEFAULT_CONSTRUCTOR_ONLY(StateInformationProb)
        DELETE_ASSIGNMENT(StateInformationProb)

        [[nodiscard]] inline bool is_abstracted() const { return abstractedTo != STATE::none; }

    };

#if 0
    struct StateInformationPA: public StateInformationBase<UpdateIndex_type> {
    public:
        StateInformationPA() = default;
        ~StateInformationPA() override = default;
    };
#endif

}

class StateSpaceProb: public StateSpaceBase<STATE_SPACE::StateInformationProb, PLAJA::Prob_type> {

private:
    std::unordered_map<StateID_type, std::vector<StateID_type>> reverseAbstraction; // per state which states abstract to it; for subsequent abstractions
    class RandomNumberGenerator* rng; // to pick random state; pointer as calls to RNG should not be considered as a modification of the state space structure

public:
    explicit StateSpaceProb();
    ~StateSpaceProb() override;
    DELETE_CONSTRUCTOR(StateSpaceProb)

    /**
     * Replace state with ID state1 by the state with ID state2.
     * This does not update parent information, but only the abstraction information.
     * @param state1
     * @param state2
     */
    void replace_state(StateID_type state1, StateID_type state2);

    // Check whether state is abstracted according to state information:
    [[nodiscard]] inline bool is_abstracted(StateID_type id) const { return get_state_information(id).is_abstracted(); }

    inline StateID_type is_abstracted_to(StateID_type id) { return get_state_information(id).abstractedTo; }

    /**
     * Given a state and a local operator index, randomly pick one of the transitions.
     * Must not be called on absorbing states.
     */
    [[nodiscard]] StateID_type sample(StateID_type state_id, LocalOpIndex_type local_op) const;

    void dump_state(StateID_type id) const;
    void dump_state_space() const;

};

#if 0
class StateSpacePA: public StateSpaceBase<STATE_SPACE::StateInformationPA, UpdateIndex_type> {
public:
    StateSpacePA();
    ~StateSpacePA() override;
};
#endif

#endif //PLAJA_STATE_SPACE_H

