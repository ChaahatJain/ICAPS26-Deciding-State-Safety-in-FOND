//
// This file is part of the PlaJA code base.
// Copyright (c) (2019 - 2023) Marcel Vinzent.
// See README.md in the top-level directory for licensing information.
//

#include "pred_entailment_cache_interface.h"
#include "../inc_state_space/incremental_state_space.h"
#include "../pa_transition_structure.h"

PredEntailmentCacheInterface::PredEntailmentCacheInterface(SEARCH_SPACE_PA::IncStateSpace& inc_ss, const PATransitionStructure& transition_ref, const class PredicateRelations* predicate_relations):
    incStateSpace(&inc_ss)
    , transition(&transition_ref)
    , predicateRelations(predicate_relations)
    , lastPredsSize(0) {
}

void PredEntailmentCacheInterface::load_entailment(PredicateState& target) const {
    PLAJA_ASSERT(transition->get_update_op_id() != ACTION::noneUpdateOp)
    lastPredsSize = incStateSpace->load_entailments(target, *transition->_source(), transition->get_update_op_id(), predicateRelations);
}

void PredEntailmentCacheInterface::store_entailment(const std::unordered_map<PredicateIndex_type, bool>& entailments, PredicatesNumber_type preds_size) const {
    incStateSpace->store_entailments(*transition->_source(), transition->get_update_op_id(), entailments, preds_size);
}
