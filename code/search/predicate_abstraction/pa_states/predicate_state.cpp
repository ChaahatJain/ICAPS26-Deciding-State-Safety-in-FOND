//
// This file is part of the PlaJA code base.
// Copyright (c) (2019 - 2020) Marcel Vinzent.
// See README.md in the top-level directory for licensing information.
//

#include "predicate_state.h"
#include "../../../parser/ast/expression/expression.h"

PredicateState::PredicateState(PredicatesNumber_type num_predicates, std::size_t num_locs, const PredicatesExpression* predicates):
    PaStateValuesBase(num_predicates, num_locs, predicates)
    , predicateStates(num_predicates, PredicateValueState::UNDEFINED) {
}

PredicateState::~PredicateState() = default;

#ifndef NDEBUG

PredicateState::PredicateState(PaStateValuesBase&& other):
    PaStateValuesBase(std::move(other))
    , predicateStates(get_size_predicates(), PLAJA_GLOBAL::lazyPA ? PredicateValueState::UNDEFINED : PredicateValueState::SET) {

    if constexpr (PLAJA_GLOBAL::lazyPA) {
        for (PredicateIndex_type pred_index = 0; pred_index < get_size_predicates(); ++pred_index) {
            if (PaStateValuesBase::is_set(pred_index)) {
                predicateStates[pred_index] = PredicateValueState::SET;
            }
        }
    }

}

#endif

/**********************************************************************************************************************/

bool PredicateState::has_entailment_information() const { return true; }

PA::EntailmentValueType PredicateState::entailment_value(PredicateIndex_type pred_index) const {
    if constexpr (PLAJA_GLOBAL::lazyPA) {
        return is_entailed(pred_index) ? PaStateValuesBase::predicate_value(pred_index) : PA::unknown_value();
    } else {
        PLAJA_ASSERT(is_entailed(pred_index))
        return PaStateValuesBase::predicate_value(pred_index);
    }
}

/**********************************************************************************************************************/

void PredicateState::set(PredicateIndex_type pred_index, bool value) {
    PLAJA_ASSERT(pred_index < predicateStates.size())

    PredicateValueState& predicate_state = predicateStates[pred_index];

    switch (predicate_state) {
        case UNDEFINED: {
            set_predicate_value(pred_index, value);
            predicate_state = PredicateValueState::SET;
            break;
        }
        case ENTAILED: {
            PLAJA_ASSERT(PaStateValuesBase::predicate_value(pred_index) == value) // Not intended.
            predicate_state = PredicateValueState::SET_AND_ENTAILED;
            break;
        }
        default: { PLAJA_ABORT } // Not intended.
    }

    add_to_layer(pred_index);
}

void PredicateState::set_entailed(PredicateIndex_type pred_index, bool value) {
    PLAJA_ASSERT(pred_index < predicateStates.size())

    PredicateValueState& predicate_state = predicateStates[pred_index];

    switch (predicate_state) {
        case UNDEFINED: {
            set_predicate_value(pred_index, value);
            predicate_state = PredicateValueState::ENTAILED;
            break;
        }
        case SET: {
            PLAJA_ASSERT(PaStateValuesBase::predicate_value(pred_index) == value) // Not intended.
            predicate_state = PredicateValueState::SET_AND_ENTAILED;
            PLAJA_ABORT_IF_CONST(PLAJA_GLOBAL::useFineGrainedPredicateStateStack) // Not supported when using fine-grained stack.
            break;
        }
        case ENTAILED: {
            PLAJA_ASSERT(PaStateValuesBase::predicate_value(pred_index) == value) // Not intended.
            return;
        }
        case SET_AND_ENTAILED: { return; }
    }

    add_to_layer(pred_index);
}

/**********************************************************************************************************************/

#ifndef NDEBUG

bool PredicateState::agrees_on_predicate_states(const PredicateState& other) const { return predicateStates == other.predicateStates; }

bool PredicateState::agrees_on_defined(const PredicateState& other) const {
    if (get_size_predicates() != other.get_size_predicates()) { return false; }

    for (auto it = init_pred_state_iterator(); !it.end(); ++it) {
        if (it.is_defined() != other.is_defined(it.predicate_index())) { return false; }
        if (it.is_defined() and it.predicate_value() != other.predicate_value(it.predicate_index())) { return false; }
    }

    return true;
}

bool PredicateState::matches(const StateBase& state) const {

    if (has_location_state() and not is_same_location_state(state)) { return false; }

    for (auto it = init_pred_state_iterator(); !it.end(); ++it) { // NOLINT(readability-use-anyofallof)

        if (it.is_set() and it.predicate_value() != it.predicate().evaluate_integer(state)) { return false; }

        if (it.is_entailed() and it.entailment_value() != it.predicate().evaluate_integer(state)) { return false; }

    }

    return true;
}

#endif

/**********************************************************************************************************************/

#if 0

void PredicateState::dump() const {
#ifndef NDEBUG
    std::cout << "[" << unsat << ";";
#else
    std::cout << "[";
#endif
    const auto l = predicateStates.size();
    for (std::size_t i = 0; i < l;) {

        switch (predicateStates[i]) {
            case UNDEFINED: {
                std::cout << "UNDEFINED";
                break;
            }
            case SET: {
                std::cout << "SET" << PLAJA_UTILS::spaceString << predicateValues[i];
                break;
            }
            case ENTAILED: {
                std::cout << "ENTAILED" << PLAJA_UTILS::spaceString << predicateValues[i];
                break;
            }
            case SET_AND_ENTAILED: {
                std::cout << "SET & ENTAILED" << PLAJA_UTILS::spaceString << predicateValues[i];
                break;
            }
            default: { PLAJA_ABORT }
        }

        if (++i < l) {
            std::cout << PLAJA_UTILS::commaString << PLAJA_UTILS::spaceString;
        }
    }
    std::cout << "]" << std::endl;
}

#endif