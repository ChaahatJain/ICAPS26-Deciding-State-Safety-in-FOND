//
// This file is part of the PlaJA code base.
// Copyright (c) (2019 - 2020) Marcel Vinzent.
// See README.md in the top-level directory for licensing information.
//

#ifndef PLAJA_PREDICATE_STATE_H
#define PLAJA_PREDICATE_STATE_H

#pragma GCC diagnostic push
#pragma ide diagnostic ignored "Simplify"

#include <forward_list>
#include "pa_state_values.h"

// #define USE_FINE_GRAINED_PREDICATE_STATE_STACK // only cache the predicates modified per stack layer

#ifdef USE_FINE_GRAINED_PREDICATE_STATE_STACK
namespace PLAJA_GLOBAL { constexpr bool useFineGrainedPredicateStateStack = true; }
#else
namespace PLAJA_GLOBAL { constexpr bool useFineGrainedPredicateStateStack = false; }
#endif

/** Extension of PaStateValues that allows incremental construction. */
class PredicateState final: public PaStateValuesBase {

    using LocationState = StateValues;

private:
    enum PredicateValueState {
        UNDEFINED,
        SET,
        ENTAILED, // predicate (value) is entailed by other predicate or structure of the underlying sat problem
        SET_AND_ENTAILED
    };

    // Predicates states.
    std::vector<PredicateValueState> predicateStates;

    // Stacks.
#ifdef USE_FINE_GRAINED_PREDICATE_STATE_STACK
    std::forward_list<std::forward_list<PredicateIndex_type>> predicateStatesStack;
#else
    std::forward_list<std::vector<PredicateValueState>> predicateStatesStack;
#endif

public:
    explicit PredicateState(PredicatesNumber_type num_predicates, std::size_t num_locs, const PredicatesExpression* predicates = nullptr);
    ~PredicateState() final;
#ifdef NDEBUG
    PredicateState(const PredicateState& other) = delete;
#else
    PredicateState(const PredicateState& other) = default;
#endif
    PredicateState(PredicateState&& other) = default;
    DELETE_ASSIGNMENT(PredicateState)

    FCT_IF_DEBUG(explicit PredicateState(PaStateValuesBase&& other);)

    /******************************************************************************************************************/

    /* Getter. */

    [[nodiscard]] inline bool is_undefined(PredicateIndex_type pred_index) const {
        PLAJA_ASSERT(pred_index < predicateStates.size())
        return predicateStates[pred_index] == PredicateValueState::UNDEFINED;
    }

    [[nodiscard]] inline bool is_defined(PredicateIndex_type pred_index) const { return not is_undefined(pred_index); }

    [[nodiscard]] bool is_set(PredicateIndex_type pred_index) const final {
        PLAJA_ASSERT(pred_index < predicateStates.size())
        const auto& pred_state = predicateStates[pred_index];
        return pred_state == PredicateValueState::SET or pred_state == PredicateValueState::SET_AND_ENTAILED;
    }

    [[nodiscard]] bool has_entailment_information() const final;

    [[nodiscard]] bool is_entailed(PredicateIndex_type pred_index) const final {
        PLAJA_ASSERT(pred_index < predicateStates.size())
        const auto& pred_state = predicateStates[pred_index];
        return pred_state == PredicateValueState::ENTAILED or pred_state == PredicateValueState::SET_AND_ENTAILED;
    }

    [[nodiscard]] PA::EntailmentValueType entailment_value(PredicateIndex_type pred_index) const final;

    // TODO: This returns the entailed value even if not set.
    //  This is not equivalent to the behavior for other PaStateBase implementations.
    //  Prone to result in bugs.

    /** Shall only be called if predicate is *defined*. */
    [[nodiscard]] PA::TruthValueType predicate_value(PredicateIndex_type pred_index) const final {
        PLAJA_ASSERT(is_defined(pred_index) /*or PA::is_unknown(PaStateValuesBase::predicate_value(pred_index))*/)
        return PaStateValuesBase::predicate_value(pred_index);
    }

    /** stack interface ***********************************************************************************************/

    inline void push_layer() {
        if constexpr (PLAJA_GLOBAL::useFineGrainedPredicateStateStack) {
            predicateStatesStack.emplace_front();
        } else {
            predicateStatesStack.push_front(predicateStates);
        }
    }

#ifdef USE_FINE_GRAINED_PREDICATE_STATE_STACK
    inline void add_to_layer(PredicateIndex_type pred_index) {
        if (not predicateStatesStack.empty()) { predicateStatesStack.front().push_front(pred_index); } // add to last layer if present
    }
#else

    inline void add_to_layer(PredicateIndex_type /*pred_index*/) {}

#endif

    inline void reset_layer() {
        PLAJA_ASSERT(not predicateStatesStack.empty())
        if constexpr (PLAJA_GLOBAL::useFineGrainedPredicateStateStack) {
            for (PredicateIndex_type pred_index: predicateStatesStack.front()) { unset(pred_index); }
        } else {
            predicateStates = predicateStatesStack.front();
        }
    }

    inline void pop_layer() {
        PLAJA_ASSERT(not predicateStatesStack.empty())
        if constexpr (PLAJA_GLOBAL::useFineGrainedPredicateStateStack) {
            for (PredicateIndex_type pred_index: predicateStatesStack.front()) { unset(pred_index); }
        } else {
            predicateStates = std::move(predicateStatesStack.front());
        }
        predicateStatesStack.pop_front();
    }

    /** setter ********************************************************************************************************/

    void set(PredicateIndex_type pred_index, bool value);

    void set_entailed(PredicateIndex_type pred_index, bool value);

    /** Shall only be called if predicate is *defined*. */
    inline void unset(PredicateIndex_type pred_index) {
        PLAJA_ASSERT(is_defined(pred_index))
        predicateStates[pred_index] = PredicateValueState::UNDEFINED;
    }

    /******************************************************************************************************************/

    FCT_IF_DEBUG([[nodiscard]] bool agrees_on_predicate_states(const PredicateState& other) const;)

    /** Compares truth values of defined predicates. */
    FCT_IF_DEBUG([[nodiscard]] bool agrees_on_defined(const PredicateState& other) const;)

    /**
     * Concrete state matches predicate state?
     * Unlike is_same_predicate_state intended to be called with "non-closed" states.
     * Reflects entailment values.
     */
    FCT_IF_DEBUG([[nodiscard]] bool matches(const StateBase& state) const;)

    /******************************************************************************************************************/

    // template<bool CONST, typename PredicateState_t>
    class PredicateStateIterator final: public PaStateBase::PredicateStateIterator {
        friend PredicateState;

    private:

        explicit PredicateStateIterator(const PredicateState& predicate_state):
            PaStateBase::PredicateStateIterator(predicate_state, 0) {}

    public:
        ~PredicateStateIterator() override = default;
        DELETE_CONSTRUCTOR(PredicateStateIterator)

        [[nodiscard]] inline const PredicateState& operator()() const { return PLAJA_UTILS::cast_ref<PredicateState>(*paState); }

        [[nodiscard]] inline bool is_undefined() const { return operator()().is_undefined(predicate_index()); }

        [[nodiscard]] inline bool is_defined() const { return operator()().is_defined(predicate_index()); }

        // template<bool is_const = CONST>
        // [[nodiscard]] inline typename std::enable_if<is_const, void>::type set(PredicateIndex_type pred_index, bool value) { predicateState.set(pred_index, value); }

    };

    using PredicateStateIteratorConst = PredicateStateIterator; // <true, const PredicateState>
    [[nodiscard]] inline PredicateStateIteratorConst init_pred_state_iterator() const { return PredicateStateIteratorConst(*this); }

};

#pragma GCC diagnostic pop

#endif //PLAJA_PREDICATE_STATE_H