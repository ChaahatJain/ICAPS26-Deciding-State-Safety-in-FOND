//
// This file is part of the PlaJA code base.
// Copyright (c) (2019 - 2023) Marcel Vinzent.
// See README.md in the top-level directory for licensing information.
//

#include "source_node.h"
#include "../../../include/compiler_config_const.h"
#include "../optimization/predicate_relations.h"
#include "../pa_states/predicate_state.h"

namespace SEARCH_SPACE_PA {

    UpdateOpNode::UpdateOpNode():
        entailments(nullptr)
        , entailmentsSize(0)
        , lastPredsSize(0) {
        static_assert(PLAJA_UTILS::is_static_type<PredicatesNumber_type, unsigned int>());
    }

    UpdateOpNode::~UpdateOpNode() = default;

    UpdateOpNode::UpdateOpNode(const SEARCH_SPACE_PA::UpdateOpNode& other):
        entailments(other.entailments ? new EntailmentInformation[other.entailmentsSize] : nullptr)
        , entailmentsSize(other.entailmentsSize)
        , lastPredsSize(other.lastPredsSize)
        , transitionWitnesses(other.transitionWitnesses) {
        PLAJA_ASSERT_EXPECT(false)
    }

    UpdateOpNode::UpdateOpNode(SEARCH_SPACE_PA::UpdateOpNode&& other) noexcept :
        entailments(std::move(other.entailments))
        , entailmentsSize(other.entailmentsSize)
        , lastPredsSize(other.lastPredsSize)
        , transitionWitnesses(std::move(other.transitionWitnesses)) {
        PLAJA_ASSERT_EXPECT(entailments == nullptr)
        PLAJA_ASSERT_EXPECT(entailmentsSize == 0)
    }

    /* */

    inline UpdateOpNode::EntailmentInformation UpdateOpNode::to_info(PredicateIndex_type pred, bool val) {
        PLAJA_ASSERT(PLAJA_UTILS::cast_numeric<int32_t>(pred) < (1 << 15)) // We have 32 bits, positive values indicate true, negative false.
        const auto pred_cast = PLAJA_UTILS::cast_numeric<int32_t>(pred) + 1;
        return val ? pred_cast : -pred_cast;
    }

    inline std::pair<PredicateIndex_type, bool> UpdateOpNode::to_info(SEARCH_SPACE_PA::UpdateOpNode::EntailmentInformation info) {
        if (info < 0) {
            return { -info - 1, false };
        } else {
            return { info - 1, true };
        }
    }

#ifndef NDEBUG

    bool UpdateOpNode::check_sanity(PredicateIndex_type pred, bool val) {
        const auto [pred_ref, val_ref] = to_info(to_info(pred, val));
        return pred_ref == pred and val_ref == val;
    }

#endif

    /* */

    void UpdateOpNode::store_entailments(const std::unordered_map<PredicateIndex_type, bool>& entailments_map, PredicatesNumber_type pred_size) {

        PLAJA_ASSERT(lastPredsSize < pred_size)
        lastPredsSize = pred_size;

        if (entailments == nullptr) {
            PLAJA_ASSERT(not has_entailment() or not has_non_empty_entailment())

            if (entailments_map.empty()) {
                entailmentsSize = -1;
                return;
            }

            entailments = std::make_unique<EntailmentInformation[]>(entailments_map.size());
            entailmentsSize = entailments_map.size();

            std::size_t index = 0;
            for (const auto& [pred, val]: entailments_map) {
                PLAJA_ASSERT(check_sanity(pred, val))
                entailments[index++] = to_info(pred, val);
            }

        } else {
            PLAJA_ASSERT(has_non_empty_entailment())

            if (entailments_map.empty()) { return; } // Nothing to do.

            auto existing_entailments = std::move(entailments);
            const auto size_existing_entailments = entailmentsSize;

            entailmentsSize += entailments_map.size();
            entailments = std::make_unique<EntailmentInformation[]>(entailmentsSize);

            std::size_t index = 0;

            /* Move existing. */
            for (; index < size_existing_entailments; ++index) { entailments[index] = existing_entailments[index]; }

            /* Add new entailments. */
            for (const auto& [pred, val]: entailments_map) {
                PLAJA_ASSERT(check_sanity(pred, val))
                entailments[index++] = to_info(pred, val);
            }

        }

    }

    void UpdateOpNode::load_entailments(PredicateState& predicate_state, const PredicateRelations* predicate_relations) const {
        PLAJA_ASSERT_EXPECT(has_entailment())

        if (not has_non_empty_entailment()) { return; }

        for (std::size_t index = 0; index < entailmentsSize; ++index) {
            const auto [pred, val] = to_info(entailments[index]);
            predicate_state.set_entailed(pred, val);
            if (predicate_relations) { predicate_relations->fix_predicate_state(predicate_state, pred); }
        }
    }

    /******************************************************************************************************************/

    SourceNode::SourceNode() = default;

    SourceNode::~SourceNode() = default;

}