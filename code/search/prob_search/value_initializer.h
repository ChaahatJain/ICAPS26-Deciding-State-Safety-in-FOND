//
// This file is part of the PlaJA code base.
// Copyright (c) (2019 - 2020) Marcel Vinzent.
// See README.md in the top-level directory for licensing information.
//

#ifndef PLAJA_VALUE_INITIALIZER_H
#define PLAJA_VALUE_INITIALIZER_H

#include <limits>
#include "optimization_criteria.h"

enum class OptimizationCriterion {
    PROB,
    COST
};

/**
 * How to initialize current value, with min. or max. value out of {V_DEADEND, V_GOAL}
 */
enum class InitializationCriterion {
    MAX,
    MIN
};

template<OptimizationCriterion OptType, InitializationCriterion InitType> // why to use template rather than constructor argument: compile time op + easier embedding in other classes' constructors
struct ValueInitializer {

private:
    ValueInitializer() = default;

public:
    static constexpr QValue_type V_GOAL = (OptType == OptimizationCriterion::PROB) ? 1 : 0;
    static constexpr QValue_type V_DEADEND = (OptType == OptimizationCriterion::PROB) ? 0 : std::numeric_limits<QValue_type>::infinity();
    static constexpr QValue_type V_BASIC = (InitType == InitializationCriterion::MAX) ? ((OptType == OptimizationCriterion::PROB) ? 1 : std::numeric_limits<QValue_type>::infinity()) : 0 /*in case of MIN for PROB as well as COST*/;
    static constexpr QValue_type V_DONTCARE = (InitType == InitializationCriterion::MIN) ? ((OptType == OptimizationCriterion::PROB) ? 1 : std::numeric_limits<QValue_type>::infinity()) : 0 /*in case of MAX for PROB as well as COST*/;

    template<class StateNode_t>
    static inline void set(StateNode_t& state_node) { state_node.set_v_current(V_BASIC, -1); }

    template<class StateNode_t>
    static inline void dont_care(StateNode_t& state_node) { state_node.set_v_current(V_DONTCARE, -1); }

    template<class StateNode_t>
    static inline bool is_dont_care(StateNode_t& state_node) {
        if constexpr(InitType == InitializationCriterion::MIN) {
            return V_DONTCARE <= state_node.get_v_current();
        } else {
            return V_DONTCARE >= state_node.get_v_current();
        }
    }

    template<class StateNode_t>
    static void dead_end(StateNode_t& state_node) { state_node.set_v_current(V_DEADEND, -1); }

    template<class StateNode_t>
    static inline void goal(StateNode_t& state_node) { state_node.set_v_current(V_GOAL, -1); }

};

template<OptimizationCriterion OptType, InitializationCriterion InitType>
struct UpperAndLowerValueInitializer {

private:
    explicit UpperAndLowerValueInitializer() = default;

public:
    static constexpr QValue_type V_GOAL = (OptType == OptimizationCriterion::PROB) ? 1 : 0;
    static constexpr QValue_type V_DEADEND = (OptType == OptimizationCriterion::PROB) ? 0 : std::numeric_limits<QValue_type>::infinity();
    static constexpr QValue_type V_BASIC = (InitType == InitializationCriterion::MAX) ? ((OptType == OptimizationCriterion::PROB) ? 1 : std::numeric_limits<QValue_type>::infinity()) : 0 /*in case of MIN for PROB as well as COST*/;
    static constexpr QValue_type V_BASIC_ALT = (InitType == InitializationCriterion::MIN) ? ((OptType == OptimizationCriterion::PROB) ? 1 : std::numeric_limits<QValue_type>::infinity()) : 0 /*in case of MIN for PROB as well as COST*/;

    template<class StateNode_t>
    static inline void set(StateNode_t& state_node) {
        state_node.set_v_current(V_BASIC, -1);
        state_node.set_v_alternative(V_BASIC_ALT, -1);
    }

    template<class StateNode_t>
    static inline void dont_care(StateNode_t& state_node) {
        if constexpr(InitType == InitializationCriterion::MIN) {
            if (V_BASIC_ALT <= state_node.get_v_current()) {
                state_node.set_v_current(V_BASIC_ALT, -1);
                state_node.set_v_alternative(V_BASIC_ALT, -1);
            } else {
                state_node.set_v_current(V_BASIC, -1);
                state_node.set_v_alternative(V_BASIC, -1);
            }
        } else {
            if (V_BASIC_ALT >= state_node.get_v_current()) {
                state_node.set_v_current(V_BASIC_ALT, -1);
                state_node.set_v_alternative(V_BASIC_ALT, -1);
            } else {
                state_node.set_v_current(V_BASIC, -1);
                state_node.set_v_alternative(V_BASIC, -1);
            }
        }
    }

    template<class StateNode_t>
    static inline bool is_dont_care(StateNode_t& state_node) {
        if constexpr(InitType == InitializationCriterion::MIN) {
            return V_BASIC_ALT <= state_node.get_v_current() || V_BASIC >= state_node.get_v_alternative();
        } else {
            return V_BASIC_ALT >= state_node.get_v_current() || V_BASIC <= state_node.get_v_alternative();
        }
    }

    template<class StateNode_t>
    static inline void dead_end(StateNode_t& state_node){
        state_node.set_v_current(V_DEADEND, -1);
        state_node.set_v_alternative(V_DEADEND, -1);
    }

    template<class StateNode_t>
    static inline void goal(StateNode_t& state_node) {
        state_node.set_v_current(V_GOAL, -1);
        state_node.set_v_alternative(V_GOAL, -1);
    }

};

#endif //PLAJA_VALUE_INITIALIZER_H
