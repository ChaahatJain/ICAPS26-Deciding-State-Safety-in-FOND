//
// This file is part of the PlaJA code base.
// Copyright (c) (2019 - 2023) Marcel Vinzent.
// See README.md in the top-level directory for licensing information.
//

#include "pa_entailment_composed.h"

PaEntailmentComposed::PaEntailmentComposed(const PaEntailmentValues& values1, const PaEntailmentValues& values2):
    PaEntailmentBase(values1.get_predicates(), values1.get_size_predicates())
    , values1(&values1)
    , values2(&values2) {

    PLAJA_ASSERT(values2.get_predicates() == get_predicates())
    PLAJA_ASSERT(values2.get_size_predicates() == get_size_predicates())
}

PaEntailmentComposed::~PaEntailmentComposed() = default;

/* */

bool PaEntailmentComposed::is_entailed(PredicateIndex_type pred_index) const { return values1->is_entailed(pred_index) or values2->is_entailed(pred_index); }

PA::EntailmentValueType PaEntailmentComposed::get_value(PredicateIndex_type pred_index) const {

    const auto value1 = values1->get_value(pred_index);

    if (PA::is_unknown(value1)) {
        return values2->get_value(pred_index);
    } else {
        PLAJA_ASSERT(not values2->is_entailed(pred_index) or values2->get_value(pred_index) == value1)
        return value1;
    }

}