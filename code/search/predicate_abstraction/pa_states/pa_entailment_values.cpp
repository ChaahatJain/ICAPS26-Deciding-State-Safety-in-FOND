//
// This file is part of the PlaJA code base.
// Copyright (c) (2019 - 2023) Marcel Vinzent.
// See README.md in the top-level directory for licensing information.
//

#include "pa_entailment_values.h"

PaEntailmentValues::PaEntailmentValues(const PredicatesExpression* predicates, std::size_t num_preds):
    PaEntailmentBase(predicates, num_preds)
    , entailmentValues(num_preds, PA::unknown_value()) {
}

PaEntailmentValues::~PaEntailmentValues() = default;

/* */

PA::EntailmentValueType PaEntailmentValues::get_value(PredicateIndex_type pred_index) const {
    PLAJA_ASSERT(pred_index < entailmentValues.size())
    return entailmentValues[pred_index];
}

bool PaEntailmentValues::is_entailed(PredicateIndex_type pred_index) const { return not PA::is_unknown(get_value(pred_index)); }

/* */

void PaEntailmentValues::set(PredicateIndex_type pred_index, bool value) {
    PLAJA_ASSERT(pred_index < entailmentValues.size())
    entailmentValues[pred_index] = value;
    PLAJA_ASSERT(entailmentValues[pred_index] == PA::False or entailmentValues[pred_index] == PA::True)
}

void PaEntailmentValues::unset(PredicateIndex_type pred_index) {
    PLAJA_ASSERT(is_entailed(pred_index))
    entailmentValues[pred_index] = PA::unknown_value();
}

/* */

void PaEntailmentValues::push_layer() {
    if constexpr (PLAJA_GLOBAL::useFineGrainedEntailmentStack) { stack.emplace_front(); }
    else { stack.push_front(entailmentValues); }
}

void PaEntailmentValues::reset_layer() {
    PLAJA_ASSERT(not stack.empty())
    if constexpr (PLAJA_GLOBAL::useFineGrainedEntailmentStack) {
        for (const PredicateIndex_type pred_index: stack.front()) { unset(pred_index); }
    } else {
        entailmentValues = stack.front();
    }
}

void PaEntailmentValues::pop_layer() {
    PLAJA_ASSERT(not stack.empty())
    if constexpr (PLAJA_GLOBAL::useFineGrainedEntailmentStack) { for (const PredicateIndex_type pred_index: stack.front()) { unset(pred_index); } }
    else { entailmentValues = std::move(stack.front()); }
    stack.pop_front();
}
