//
// This file is part of the PlaJA code base.
// Copyright (c) (2019 - 2022) Marcel Vinzent.
// See README.md in the top-level directory for licensing information.
//

#include "hamming_distance.h"
#include "../../factories/configuration.h"
#include "../../smt/solver/smt_solver_z3.h"
#include "../optimization/pa_entailments.h"
#include "../optimization/predicate_relations.h"
#include "../pa_states/abstract_state.h"
#include "../smt/model_z3_pa.h"
#include "heuristic_state_queue.h"

HammingDistance::HammingDistance(const PLAJA::Configuration& config, const ModelZ3PA& model_pa):
    entailments(new PaEntailments(config, model_pa, model_pa.get_reach())) {

    PLAJA_LOG_DEBUG_IF(not entailments->has_stats(), PLAJA_UTILS::to_red_string("Hamming distance without stats."))

    if constexpr (PLAJA_GLOBAL::lazyPA) {
        if (config.has_sharable(PLAJA::SharableKey::PREDICATE_RELATIONS)) { predicateRelations = config.get_sharable<PredicateRelations>(PLAJA::SharableKey::PREDICATE_RELATIONS); }
        else { predicateRelations = config.set_sharable(PLAJA::SharableKey::PREDICATE_RELATIONS, std::make_shared<PredicateRelations>(config, model_pa, nullptr)); }
        PLAJA_ASSERT(predicateRelations->has_empty_ground_truth())
    }

    increment();
}

HammingDistance::~HammingDistance() = default;

/* */

HeuristicValue_type HammingDistance::_evaluate(const AbstractState& state) { return static_cast<const HammingDistance*>(this)->_evaluate(state); }

HeuristicValue_type HammingDistance::_evaluate(const AbstractState& state) const {

    /* |p_unsafe - s_p| where p_unsafe(i) = 1 if "unsafe => p_i" and p_unsafe(p_i) = 0 "if unsafe => !p_i". */
    int distance = 0;

    for (const auto& [pred_index, value]: entailments->get_entailments()) {

        if constexpr (PLAJA_GLOBAL::lazyPA) {

            if (not state.is_set(pred_index)) {
                distance += predicateRelations->is_entailed(pred_index, value, state) ? 0 : 1;
                continue;
            }

        }

        PLAJA_ASSERT(state.is_set(pred_index))

        /*
         * In non-lazy PA we distinguish matched truth values (distance 0) and unmatched truth values (1).
         * For lazy PA we distinguish matched truth values (0), unknown truth values (1) and unmatched truth values (2).
         */
        distance += (value != state.predicate_value(pred_index) ? (PLAJA_GLOBAL::lazyPA ? 2 : 1) : 0);
    }

    return distance;
}

void HammingDistance::increment() { entailments->increment(); }

std::unique_ptr<HeuristicStateQueue> HammingDistance::make_heuristic_state_queue(PLAJA::Configuration& config, const ModelZ3PA& model_z3) {
    return std::make_unique<HeuristicStateQueue>(config, std::make_unique<HammingDistance>(config, model_z3));
}
