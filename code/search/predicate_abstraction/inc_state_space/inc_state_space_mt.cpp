//
// This file is part of the PlaJA code base.
// Copyright (c) (2019 - 2023) Marcel Vinzent.
// See README.md in the top-level directory for licensing information.
//

#include "inc_state_space_mt.h"
#include "../../../exception/constructor_exception.h"
#include "../../../stats/stats_base.h"
#include "../../factories/configuration.h"
#include "../match_tree/mt_size.h"
#include "../pa_states/predicate_state.h"

SEARCH_SPACE_PA::IncStateSpaceMt::IncStateSpaceMt(const PLAJA::Configuration& config, std::shared_ptr<StateRegistryPa> state_reg_pa):
    IncStateSpace(config, std::move(state_reg_pa))
    , matchTree(config.has_sharable(PLAJA::SharableKey::PA_MT) ? config.get_sharable<PAMatchTree>(PLAJA::SharableKey::PA_MT) : config.set_sharable<PAMatchTree>(PLAJA::SharableKey::PA_MT, std::make_shared<PAMatchTree>(config, stateRegistryPa->get_predicates())))
    , stats(config.get_sharable_ptr<PLAJA::StatsBase>(PLAJA::SharableKey::STATS)) {

    /*
     * MT-based state space stores information (transition existence/exclusion) solely per predicate truth value (ignoring location values).
     * In principle, implementation is still correct, since PA machinery checks location values explicitly.
     * Issues should only arise once locations become part of the PA SMT encoding (e.g., for policies that take locations as inputs).
     */
    PLAJA_LOG_IF(stateRegistryPa->num_locations() > 1, PLAJA_UTILS::to_red_string("Warning: MT-based state space is not firmly tested on models with multiple locations!"))

    PLAJA_ASSERT_EXPECT(stats);
    auto mt_size = matchTree->compute_size();
    stats->add_unsigned_stats(PLAJA::StatsUnsigned::MT_SIZE, "MTSize", mt_size.size);
    stats->add_unsigned_stats(PLAJA::StatsUnsigned::MT_MAX_DEPTH, "MTMaxDepth", mt_size.maxDepth);
    stats->add_double_stats(PLAJA::StatsDouble::MT_AV_DEPTH, "MTAvDepth", 0);
}

SEARCH_SPACE_PA::IncStateSpaceMt::~IncStateSpaceMt() = default;

/*********************************************************************************************************************/

void SEARCH_SPACE_PA::IncStateSpaceMt::update_stats() const {
    auto mt_size = matchTree->compute_size();

    stats->set_attr_unsigned(PLAJA::StatsUnsigned::MT_SIZE, mt_size.size);
    stats->set_attr_unsigned(PLAJA::StatsUnsigned::MT_MAX_DEPTH, mt_size.maxDepth);
    stats->set_attr_double(PLAJA::StatsDouble::MT_AV_DEPTH, mt_size.get_av_depth());

    IncStateSpace::update_stats();
}

void SEARCH_SPACE_PA::IncStateSpaceMt::increment() {
    matchTree->increment();
    IncStateSpace::increment();
}

/**********************************************************************************************************************/

SEARCH_SPACE_PA::SourceNode* SEARCH_SPACE_PA::IncStateSpaceMt::get_source_node(const AbstractState& src) { return &matchTree->access_information<SEARCH_SPACE_PA::MtNodeInformation>(src).get_source_node(); }

SEARCH_SPACE_PA::UpdateOpNode* SEARCH_SPACE_PA::IncStateSpaceMt::get_update_op(const AbstractState& src, UpdateOpID_type update_op_id) { return matchTree->access_information<SEARCH_SPACE_PA::MtNodeInformation>(src).get_source_node().get_update_op(update_op_id); }

SEARCH_SPACE_PA::SourceNode& SEARCH_SPACE_PA::IncStateSpaceMt::add_source_node(const AbstractState& src) { return matchTree->access_information<SEARCH_SPACE_PA::MtNodeInformation>(src).get_source_node(); }

SEARCH_SPACE_PA::UpdateOpNode& SEARCH_SPACE_PA::IncStateSpaceMt::add_update_op(const AbstractState& src, UpdateOpID_type update_op_id) { return matchTree->access_information<SEARCH_SPACE_PA::MtNodeInformation>(src).get_source_node().add_update_op(update_op_id); }

/**********************************************************************************************************************/

void SEARCH_SPACE_PA::IncStateSpaceMt::set_excluded_internal(const AbstractState& src, UpdateOpID_type update_op_id, const AbstractState& excluded_successor) {
    matchTree->access_information<SEARCH_SPACE_PA::MtNodeInformation>(src).get_source_node().set_excluded(update_op_id, &matchTree->add_path(excluded_successor));
}

void SEARCH_SPACE_PA::IncStateSpaceMt::refine_for(const AbstractState& source) {

#ifdef LAZY_PA

    // first we construct the path and cache the predicates sequence
    std::list<PredicateIndex_type> predicates_path;
    auto source_it = PA_MATCH_TREE::TraverserConstructor::init(*matchTree);
    while (not source_it.traversed(source)) {
        predicates_path.push_back(source_it.predicate_index());
        source_it.next(source);
    }

    auto& main_node = source_it.node();

#else

    auto& main_node = matchTree->add_path(source);

#endif

    if (not main_node.get_information()) { main_node.set_information(std::make_unique<SEARCH_SPACE_PA::MtNodeInformation>()); }
    auto& main_source_node = main_node.get_information<SEARCH_SPACE_PA::MtNodeInformation>()->get_source_node();

    for (auto path_traverser = PA_MATCH_TREE::TraverserModifier::init(*matchTree); !path_traverser.traversed(source);
         path_traverser.next(source)
#ifdef LAZY_PA
        , predicates_path.pop_front()
#endif
        ) {

        STMT_IF_LAZY_PA(PLAJA_ASSERT(predicates_path.front() == path_traverser.predicate_index()))

        auto* info = path_traverser.get_information<SEARCH_SPACE_PA::MtNodeInformation>();
        if (not info) { continue; }

        for (auto& [update_op_id, update_op_node]: info->get_source_node().get_transitions_per_op()) {

            auto& transition_witnesses = update_op_node.get_transition_witnesses();
            for (auto it = transition_witnesses.begin(); it != transition_witnesses.end();) {

                const auto concrete_source = stateRegistryPa->lookup_witness(it->get_source());

                PLAJA_ASSERT(path_traverser.get_prefix_state().matches(concrete_source))

#ifdef LAZY_PA
                if (std::all_of(predicates_path.cbegin(), predicates_path.cend(), [&source, &concrete_source](PredicateIndex_type pred_index) { return source.agree(concrete_source, pred_index); })) {
#else
                if (source.is_same_predicate_state_sub(concrete_source, path_traverser.predicate_index())) {
#endif

                    PLAJA_ASSERT(source.is_abstraction(concrete_source))
                    main_source_node.add_transition_witness(update_op_id, it->get_source(), it->get_successor());
                    it = transition_witnesses.erase(it);

                } else { ++it; }

            }

        }

    }

    STMT_IF_LAZY_PA(PLAJA_ASSERT(predicates_path.empty()))

    STMT_IF_DEBUG(if (has_ref()) { access_ref().refine_for(source); })
}

bool SEARCH_SPACE_PA::IncStateSpaceMt::is_excluded_label_internal(const AbstractState& source, ActionLabel_type action_label) const {

    for (auto path_traverser = PA_MATCH_TREE::TraverserConst::init(*matchTree); !path_traverser.end(); path_traverser.next(source)) {

        auto* info = path_traverser.get_information<const SEARCH_SPACE_PA::MtNodeInformation>();
        if (not info) { continue; }
        if (info->get_source_node().is_excluded_label(action_label)) { return true; }

    }

    return false;

}

bool SEARCH_SPACE_PA::IncStateSpaceMt::is_excluded_op_internal(const AbstractState& source, ActionOpID_type action_op) const {

    for (auto path_traverser = PA_MATCH_TREE::TraverserConst::init(*matchTree); !path_traverser.end(); path_traverser.next(source)) {

        auto* info = path_traverser.get_information<const SEARCH_SPACE_PA::MtNodeInformation>();
        if (not info) { continue; }
        if (info->get_source_node().is_excluded_op(action_op)) { return true; }

    }

    return false;

}

PredicatesNumber_type SEARCH_SPACE_PA::IncStateSpaceMt::load_entailments_internal(PredicateState& target, const AbstractState& source, UpdateOpID_type update_op, const PredicateRelations* predicate_relations) const {

    PLAJA_ASSERT(stateRegistryPa->has_pa_state(source.get_size_predicates(), source.get_id_value()))

    PredicatesNumber_type last_preds_size = 0;

    auto path_traverser = PA_MATCH_TREE::TraverserConst::init(*matchTree);
    PLAJA_ASSERT(not path_traverser.get_information())

    for (path_traverser.next(source); !path_traverser.traversed(source); path_traverser.next(source)) {
        PLAJA_ASSERT(not path_traverser.end())

        auto* info = path_traverser.get_information<const SEARCH_SPACE_PA::MtNodeInformation>();

        if (not info) { continue; }

        const auto* update_op_node = info->get_source_node().get_update_op(update_op);

        if (not update_op_node or not update_op_node->has_entailment()) { continue; }

        PLAJA_ASSERT(not PLAJA_GLOBAL::lazyPA)
        PLAJA_ASSERT(last_preds_size < update_op_node->retrieve_last_predicates_size())
        last_preds_size = update_op_node->retrieve_last_predicates_size();
        PLAJA_ASSERT(last_preds_size <= path_traverser.predicate_index())

        update_op_node->load_entailments(target, predicate_relations);

    }

    PLAJA_ASSERT(path_traverser.is_sane())
    PLAJA_ASSERT(path_traverser.is_final()) // expected, since source should be closed
    PLAJA_ASSERT(path_traverser.equals_prefix(source))

    auto* info = path_traverser.get_information<const SEARCH_SPACE_PA::MtNodeInformation>();
    PLAJA_ASSERT(info)
    const auto& source_node = info->get_source_node();
    {
        const auto* update_op_node = source_node.get_update_op(update_op);
        if (update_op_node and update_op_node->has_entailment()) {

            PLAJA_ASSERT(not PLAJA_GLOBAL::lazyPA)
            PLAJA_ASSERT(last_preds_size < update_op_node->retrieve_last_predicates_size())
            last_preds_size = update_op_node->retrieve_last_predicates_size();
            PLAJA_ASSERT(last_preds_size <= source.get_size_predicates())

            update_op_node->load_entailments(target, predicate_relations);
        }

    }

    return last_preds_size;

}

inline std::unique_ptr<StateID_type> SEARCH_SPACE_PA::IncStateSpaceMt::transition_exists_internal(const AbstractState& source, UpdateOpID_type update_op_id, const AbstractState& successor) const {

    std::list<const PAMatchTree::Node*> successor_nodes;
    // collect successor match tree nodes; these are used to compactly represent exclusion
    for (auto succ_traverser = PA_MATCH_TREE::TraverserConst::init(*matchTree); !succ_traverser.end(); succ_traverser.next(successor)) { successor_nodes.push_back(&succ_traverser.node()); }

    auto path_traverser = PA_MATCH_TREE::TraverserConst::init(*matchTree);
    // auto succ_reader = matchTree->pathReader(successor);
    for (; !path_traverser.traversed(source); path_traverser.next(source)) {
        PLAJA_ASSERT(not path_traverser.end())

        // PLAJA_ASSERT(succ_reader._predicate_index() <= path_traverser._predicate_index())
        // while (succ_reader._predicate_index() != path_traverser._predicate_index()) { ++succ_reader; }

        auto* info = path_traverser.get_information<const SEARCH_SPACE_PA::MtNodeInformation>();
        if (not info) { continue; }

        const auto& source_node = info->get_source_node();
        const auto* update_op_node = source_node.get_update_op(update_op_id);
        if (not update_op_node) { continue; }

        // check whether sub-state of successor is marked as excluded
        if (std::any_of(successor_nodes.cbegin(), successor_nodes.cend(), [update_op_node](const PAMatchTree::Node* node) { return PLAJA_UTILS::cast_ptr<UpdateOpNodeP<const PAMatchTree::Node*>>(update_op_node)->is_excluded(node); })) { return std::make_unique<StateID_type>(STATE::none); }
        PLAJA_ASSERT(not std::any_of(update_op_node->get_transition_witnesses().cbegin(), update_op_node->get_transition_witnesses().cend(), [this, &source](const SEARCH_SPACE_PA::TransitionWitness& witness) { return source.is_abstraction(stateRegistryPa->lookup_witness(witness.get_source())); }))

    }

    PLAJA_ASSERT(path_traverser.is_sane())
    PLAJA_ASSERT(path_traverser.is_final()) // expected, since source should be closed
    PLAJA_ASSERT(path_traverser.equals_prefix(source))

    auto* info = path_traverser.get_information<const SEARCH_SPACE_PA::MtNodeInformation>();
    PLAJA_ASSERT(info)
    const auto& source_node = info->get_source_node();
    {
        const auto* update_op_node = source_node.get_update_op(update_op_id);
        if (not update_op_node) { return nullptr; }

        // check whether sub-state of successor is marked as excluded
        if (std::any_of(successor_nodes.cbegin(), successor_nodes.cend(), [update_op_node](const PAMatchTree::Node* node) { return PLAJA_UTILS::cast_ptr<UpdateOpNodeP<const PAMatchTree::Node*>>(update_op_node)->is_excluded(node); })) { return std::make_unique<StateID_type>(STATE::none); }

        for (const auto& transition_witness: update_op_node->get_transition_witnesses()) {

            if (not source.is_same_location_state(stateRegistryPa->lookup_witness(transition_witness.get_source()))) { continue; }

            STMT_IF_DEBUG(auto concrete_src = stateRegistryPa->lookup_witness(transition_witness.get_source()))
            PLAJA_ASSERT(path_traverser.get_prefix_state().matches(concrete_src))
            PLAJA_ASSERT(source.is_abstraction(concrete_src))

            const auto concrete_successor = stateRegistryPa->lookup_witness(transition_witness.get_successor());
            if (successor.is_abstraction(concrete_successor)) { return std::make_unique<StateID_type>(transition_witness.get_source()); }
        }

    }

    return nullptr;

}

