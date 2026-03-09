//
// This file is part of the PlaJA code base.
// Copyright (c) (2019 - 2022) Marcel Vinzent.
// See README.md in the top-level directory for licensing information.
//

#include "incremental_state_space.h"
#include "../../factories/predicate_abstraction/pa_options.h"
#include "../../factories/configuration.h"
#include "../pa_states/predicate_state.h"
#include "inc_state_space_reg.h"
#include "inc_state_space_mt.h"

namespace SEARCH_SPACE_PA {

#ifdef NDEBUG

#ifdef USE_MATCH_TREE

    [[nodiscard]] inline const SEARCH_SPACE_PA::IncStateSpaceMt& de_virtualize(const SEARCH_SPACE_PA::IncStateSpace& ss) { return PLAJA_UTILS::cast_ref<SEARCH_SPACE_PA::IncStateSpaceMt>(ss); }

    [[nodiscard]] inline SEARCH_SPACE_PA::IncStateSpaceMt& de_virtualize(SEARCH_SPACE_PA::IncStateSpace& ss) { return PLAJA_UTILS::cast_ref<SEARCH_SPACE_PA::IncStateSpaceMt>(ss); }

#else

    [[nodiscard]] inline const SEARCH_SPACE_PA::IncStateSpaceReg& de_virtualize(const SEARCH_SPACE_PA::IncStateSpace& ss) { return PLAJA_UTILS::cast_ref<SEARCH_SPACE_PA::IncStateSpaceReg>(ss); }
    [[nodiscard]] inline SEARCH_SPACE_PA::IncStateSpaceReg& de_virtualize(SEARCH_SPACE_PA::IncStateSpace& ss) { return PLAJA_UTILS::cast_ref<SEARCH_SPACE_PA::IncStateSpaceReg>(ss); }

#endif

#else

    [[nodiscard]] inline const SEARCH_SPACE_PA::IncStateSpace& de_virtualize(const SEARCH_SPACE_PA::IncStateSpace& ss) { return ss; }

    [[nodiscard]] inline SEARCH_SPACE_PA::IncStateSpace& de_virtualize(SEARCH_SPACE_PA::IncStateSpace& ss) { return ss; }

#endif

}

SEARCH_SPACE_PA::IncStateSpace::IncStateSpace(const PLAJA::Configuration& config, std::shared_ptr<StateRegistryPa> state_reg_pa):
    stateRegistryPa(std::move(state_reg_pa))
    , cachedSrcId(STATE::none)
    , cachedUpdateOpId(ACTION::noneUpdateOp)
    , cachedSrc(nullptr)
    , cachedUpdateOp(nullptr)
    CONSTRUCT_IF_DEBUG(predicatesSize(0))
    , useIncSearch(config.get_bool_option(PLAJA_OPTION::incremental_search))
    CONSTRUCT_IF_DEBUG(cacheWitnesses(config.is_flag_set(PLAJA_OPTION::cacheWitnesses)))
    CONSTRUCT_IF_DEBUG(incStateSpaceRef(nullptr)) {

#ifdef USE_REF_REG

    static bool construct_ref_ss(true); // Suppress construction of ref-ss for ref-ss itself.

    if (construct_ref_ss and useIncSearch) {
        /* Pointless to do use incStateSpaceRef in non-incremental mode.
         * Additionally, this would require several modifications (also in search space class).
         */

        construct_ref_ss = false;

#ifdef USE_INC_REG
        incStateSpaceRef = std::make_unique<SEARCH_SPACE_PA::IncStateSpaceMt>(config, stateRegistryPa);
#endif

#ifdef USE_MATCH_TREE
        incStateSpaceRef = std::make_unique<SEARCH_SPACE_PA::IncStateSpaceReg>(config, stateRegistryPa);
#endif

        construct_ref_ss = true;

    }

#endif

}

SEARCH_SPACE_PA::IncStateSpace::~IncStateSpace() = default;

/* */

void SEARCH_SPACE_PA::IncStateSpace::update_stats() const { STMT_IF_DEBUG(if (incStateSpaceRef) { incStateSpaceRef->update_stats(); }) }

void SEARCH_SPACE_PA::IncStateSpace::increment() {
    cachedSrcId = STATE::none;
    cachedUpdateOpId = ACTION::noneUpdateOp;
    cachedSrc = nullptr;
    cachedUpdateOp = nullptr;
    STMT_IF_DEBUG(predicatesSize = 0;)

    STMT_IF_DEBUG(if (incStateSpaceRef) { incStateSpaceRef->increment(); })
}

void SEARCH_SPACE_PA::IncStateSpace::clear_coarser() { STMT_IF_DEBUG(if (incStateSpaceRef) { incStateSpaceRef->clear_coarser(); }) }

/**********************************************************************************************************************/

SEARCH_SPACE_PA::SourceNode* SEARCH_SPACE_PA::IncStateSpace::get_and_cache_source_node(const AbstractState& src) {
    if (cachedSrcId == src.get_id_value()) {
        PLAJA_ASSERT(cachedSrc)
        PLAJA_ASSERT(src.get_size_predicates() == predicatesSize)
        PLAJA_ASSERT(get_source_node(src) == cachedSrc) // Sanity check.
        return cachedSrc;
    } else {
        auto* src_node = de_virtualize(*this).get_source_node(src);
        if (not src_node) { return nullptr; }

        cachedSrcId = src.get_id_value();
        cachedSrc = src_node;
        STMT_IF_DEBUG(predicatesSize = src.get_size_predicates();)
        return cachedSrc;
    }
}

SEARCH_SPACE_PA::UpdateOpNode* SEARCH_SPACE_PA::IncStateSpace::get_and_cache_update_op(const AbstractState& src, UpdateOpID_type update_op_id) {
    if (cachedUpdateOpId == update_op_id and cachedSrcId == src.get_id_value()) {
        PLAJA_ASSERT(cachedUpdateOp)
        PLAJA_ASSERT(src.get_size_predicates() == predicatesSize)
        PLAJA_ASSERT(get_update_op(src, update_op_id) == cachedUpdateOp) // Sanity check.
        return cachedUpdateOp;
    } else {
        auto* src_node = get_and_cache_source_node(src);
        if (not src_node) { return nullptr; }

        auto* update_op = src_node->get_update_op(update_op_id);
        if (not update_op) { return nullptr; }

        cachedUpdateOpId = update_op_id;
        cachedUpdateOp = update_op;
        return cachedUpdateOp;
    }
}

SEARCH_SPACE_PA::SourceNode& SEARCH_SPACE_PA::IncStateSpace::add_and_cache_source_node(const AbstractState& src) {
    if (cachedSrcId == src.get_id_value()) {
        PLAJA_ASSERT(cachedSrc)
        PLAJA_ASSERT(src.get_size_predicates() == predicatesSize)
        PLAJA_ASSERT(get_source_node(src) == cachedSrc) // Sanity check.
        return *cachedSrc;
    } else {
        cachedSrcId = src.get_id_value();
        cachedSrc = &de_virtualize(*this).add_source_node(src);
        STMT_IF_DEBUG(predicatesSize = src.get_size_predicates();)
        return *cachedSrc;
    }
}

SEARCH_SPACE_PA::UpdateOpNode& SEARCH_SPACE_PA::IncStateSpace::add_and_cache_update_op(const AbstractState& src, UpdateOpID_type update_op_id) {
    if (cachedUpdateOpId == update_op_id and cachedSrcId == src.get_id_value()) {
        PLAJA_ASSERT(cachedUpdateOp)
        PLAJA_ASSERT(src.get_size_predicates() == predicatesSize)
        PLAJA_ASSERT(get_update_op(src, update_op_id) == cachedUpdateOp) // Sanity check.
        return *cachedUpdateOp;
    } else {
        auto& src_node = add_and_cache_source_node(src);
        cachedUpdateOpId = update_op_id;
        cachedUpdateOp = &src_node.add_update_op(update_op_id);
        return *cachedUpdateOp;
    }
}

/**********************************************************************************************************************/

void SEARCH_SPACE_PA::IncStateSpace::set_excluded_label(const AbstractState& src, ActionLabel_type action_label) { // NOLINT(*-no-recursion)
    if (not useIncSearch) { return; }

    auto* src_node = get_and_cache_source_node(src);
    PLAJA_ASSERT(src_node);
    src_node->set_excluded_label(action_label);

    STMT_IF_DEBUG(if (incStateSpaceRef) { incStateSpaceRef->set_excluded_label(src, action_label); })
}

void SEARCH_SPACE_PA::IncStateSpace::set_excluded_op(const AbstractState& src, ActionOpID_type action_op) { // NOLINT(*-no-recursion)
    if (not useIncSearch) { return; }

    auto* src_node = get_and_cache_source_node(src);
    PLAJA_ASSERT(src_node);
    src_node->set_excluded_op(action_op);

    STMT_IF_DEBUG(if (incStateSpaceRef) { incStateSpaceRef->set_excluded_op(src, action_op); })
}

void SEARCH_SPACE_PA::IncStateSpace::store_entailments(const AbstractState& src, UpdateOpID_type update_op_id, const std::unordered_map<PredicateIndex_type, bool>& entailments, PredicatesNumber_type preds_size) {
    if (not useIncSearch) { return; }

    add_and_cache_update_op(src, update_op_id).store_entailments(entailments, preds_size);

#ifndef NDEBUG
    if (has_ref()) {

        access_ref().add_and_cache_update_op(src, update_op_id).store_entailments(entailments, preds_size);

        PredicateState target(src.get_size_predicates(), 0, src.get_predicates());
        PredicateState copy(target);
        PLAJA_ASSERT(copy.agrees_on_predicate_states(target))
        auto rlt_ref = access_ref().load_entailments(copy, src, update_op_id, nullptr);
        auto rlt = load_entailments(target, src, update_op_id, nullptr);
        PLAJA_ASSERT(copy.agrees_on_predicate_states(target))
        PLAJA_ASSERT(rlt_ref == rlt)
    }
#endif

}

void SEARCH_SPACE_PA::IncStateSpace::add_transition_witness(const AbstractState& src, UpdateOpID_type update_op_id, StateID_type src_witness, StateID_type suc_witness) {
    PLAJA_ASSERT_EXPECT(cacheWitnesses)

    /* Note, we always store transition witnesses, even if useIncSearch is false. */
    PLAJA_ASSERT(src.is_abstraction(stateRegistryPa->lookup_witness(src_witness)))
    add_and_cache_source_node(src).add_transition_witness(update_op_id, src_witness, suc_witness);
    STMT_IF_DEBUG(if (incStateSpaceRef) { incStateSpaceRef->add_and_cache_source_node(src).add_transition_witness(update_op_id, src_witness, suc_witness); })
}

void SEARCH_SPACE_PA::IncStateSpace::set_excluded(const AbstractState& src, UpdateOpID_type update_op_id, const AbstractState& excluded_successor) {
    if (not useIncSearch) { return; }
    de_virtualize(*this).set_excluded_internal(src, update_op_id, excluded_successor);
    STMT_IF_DEBUG(if (incStateSpaceRef) { incStateSpaceRef->set_excluded_internal(src, update_op_id, excluded_successor); })
}

/**********************************************************************************************************************/

bool SEARCH_SPACE_PA::IncStateSpace::is_excluded_label(const AbstractState& source, ActionLabel_type action_label) const {
    if (not useIncSearch) { return false; }
    const bool rlt = de_virtualize(*this).is_excluded_label_internal(source, action_label);

#ifndef NDEBUG
    if (incStateSpaceRef) {
        auto rlt_ref = incStateSpaceRef->is_excluded_label_internal(source, action_label);
        PLAJA_ASSERT(rlt == rlt_ref)
    }
#endif

    return rlt;
}

bool SEARCH_SPACE_PA::IncStateSpace::is_excluded_op(const AbstractState& source, ActionOpID_type action_op) const {
    if (not useIncSearch) { return false; }
    const bool rlt = de_virtualize(*this).is_excluded_op_internal(source, action_op);

#ifndef NDEBUG
    if (incStateSpaceRef) {
        auto rlt_ref = incStateSpaceRef->is_excluded_op_internal(source, action_op);
        PLAJA_ASSERT(rlt == rlt_ref)
    }
#endif

    return rlt;
}

PredicatesNumber_type SEARCH_SPACE_PA::IncStateSpace::load_entailments(PredicateState& target, const AbstractState& source, UpdateOpID_type update_op_id, const PredicateRelations* predicate_relations) const {
    if (not useIncSearch) { return 0; }

#ifndef NDEBUG
    if (has_ref()) {
        PredicateState copy(target);
        PLAJA_ASSERT(copy.agrees_on_predicate_states(target))
        auto rlt_ref = incStateSpaceRef->load_entailments_internal(copy, source, update_op_id, predicate_relations);
        auto rlt = load_entailments_internal(target, source, update_op_id, predicate_relations);
        PLAJA_ASSERT(copy.agrees_on_predicate_states(target))
        if (rlt_ref != rlt) {
            PLAJA_LOG_DEBUG(rlt)
            PLAJA_LOG_DEBUG(rlt_ref)
        }
        PLAJA_ASSERT(rlt_ref == rlt)
        return rlt;
    }
#endif

    return de_virtualize(*this).load_entailments_internal(target, source, update_op_id, predicate_relations);

}

std::unique_ptr<StateID_type> SEARCH_SPACE_PA::IncStateSpace::retrieve_witness(const AbstractState& source, UpdateOpID_type update_op_id, const AbstractState& successor) const {
    auto rlt = de_virtualize(*this).transition_exists_internal(source, update_op_id, successor);

#ifndef NDEBUG
    if (incStateSpaceRef) {

        auto rlt_ref = incStateSpaceRef->transition_exists_internal(source, update_op_id, successor);

        PLAJA_ASSERT((not rlt and not rlt_ref) or (rlt and rlt_ref))

        PLAJA_ASSERT((not rlt and not rlt_ref) or *rlt == *rlt_ref)

    }
#endif

    return rlt;
}

std::unique_ptr<StateID_type> SEARCH_SPACE_PA::IncStateSpace::transition_exists(const AbstractState& source, UpdateOpID_type update_op_id, const AbstractState& successor) const {
    return useIncSearch ? retrieve_witness(source, update_op_id, successor) : nullptr;
}
