//
// This file is part of the PlaJA code base.
// Copyright (c) (2019 - 2023) Marcel Vinzent.
// See README.md in the top-level directory for licensing information.
//

#include "pa_entailment_base.h"
#include "../../../parser/ast/expression/non_standard/predicates_expression.h"

PaEntailmentBase::PaEntailmentBase(const PredicatesExpression* predicates, std::size_t num_preds):
    predicates(predicates)
    , numPredicates(num_preds) {
}

PaEntailmentBase::~PaEntailmentBase() = default;

/* */

const Expression& PaEntailmentBase::get_predicate(PredicateIndex_type pred_index) const {
    PLAJA_ASSERT(pred_index < get_size_predicates())
    PLAJA_ASSERT(predicates)
    PLAJA_ASSERT(predicates->get_predicate(pred_index))
    return *predicates->get_predicate(pred_index);
}

/* */

bool PaEntailmentBase::operator==(const PaEntailmentBase& other) const {
    if (get_size_predicates() != other.get_size_predicates()) { return false; }

    for (auto it = init_iterator(); !it.end(); ++it) { if (it.get_value() != other.get_value(it.predicate_index())) { return false; } }

    return true;
}
