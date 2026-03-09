//
// This file is part of the PlaJA code base.
// Copyright (c) (2019 - 2023) Marcel Vinzent.
// See README.md in the top-level directory for licensing information.
//

#ifndef PLAJA_PRED_ENTAILMENT_CACHE_INTERFACE_H
#define PLAJA_PRED_ENTAILMENT_CACHE_INTERFACE_H

#include <unordered_map>
#include "../../../assertions.h"
#include "../using_predicate_abstraction.h"
#include "../inc_state_space/forward_inc_state_space_pa.h"
#include "../forward_pa.h"

/**
 * Access cached entailment info (for entailment per op-update) from previous iterations of PA computation (in coarser state spaces).
 */
struct PredEntailmentCacheInterface final {
private:
    /* To store new entailment information. */
    SEARCH_SPACE_PA::IncStateSpace* incStateSpace;
    const PATransitionStructure* transition;
    const class PredicateRelations* predicateRelations;

    mutable PredicatesNumber_type lastPredsSize; // The size of predicate set considered during the computation of the loaded entailment information.

public:
    PredEntailmentCacheInterface(SEARCH_SPACE_PA::IncStateSpace& inc_ss, const PATransitionStructure& transition, const PredicateRelations* predicate_relations);
    ~PredEntailmentCacheInterface() = default;
    DELETE_CONSTRUCTOR(PredEntailmentCacheInterface)

    [[nodiscard]] inline const class PredicateRelations* get_predicate_relations() const { return predicateRelations; }

    void load_entailment(class PredicateState& target) const;

    /** The size of predicate set considered during the computation of the loaded entailment information. */
    [[nodiscard]] inline PredicatesNumber_type get_last_predicates_size() const { return lastPredsSize; }

    void store_entailment(const std::unordered_map<PredicateIndex_type, bool>& entailments, PredicatesNumber_type preds_size) const;

};

#endif //PLAJA_PRED_ENTAILMENT_CACHE_INTERFACE_H
