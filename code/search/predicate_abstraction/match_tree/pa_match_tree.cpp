//
// This file is part of the PlaJA code base.
// Copyright (c) (2019 - 2022) Marcel Vinzent.
// See README.md in the top-level directory for licensing information.
//

#include "pa_match_tree.h"
#include "../../../parser/ast/expression/non_standard/predicates_expression.h"
#include "../../factories/predicate_abstraction/pa_options.h"
#include "../../factories/configuration.h"
#include "../optimization/predicate_relations.h"
#include "../pa_states/predicate_state.h"
#include "mt_size.h"

namespace PA_MATCH_TREE {

#ifndef NDEBUG

    template<typename Container>
    void dump_closure(const Container& closure) {

        std::stringstream ss;
        ss << "Closure:";

        for (auto pred: closure) { ss << PLAJA_UTILS::spaceString << pred; }

        PLAJA_LOG_DEBUG(ss.str())

    }

#endif

}

/**********************************************************************************************************************/

PaMtNodeInformation::PaMtNodeInformation() = default;

PaMtNodeInformation::~PaMtNodeInformation() = default;

/**********************************************************************************************************************/

#ifdef LAZY_PA

PAMatchTree::Node::Node():
    predicateIndex(PREDICATE::none) {}

#else

PAMatchTree::Node::Node() = default;

#endif

PAMatchTree::Node::~Node() = default;

PAMatchTree::Node& PAMatchTree::Node::add_successor(bool value) {
    auto& node = value ? posNode : negNode;
    if (not node) { node = std::make_unique<Node>(); }
    return *node;
}

#if 0
void PAMatchTree::Node::set_predicate_closure(std::vector<PredicateIndex_type>&& predicate_closure) {
    PLAJA_ASSERT(predicateClosure.empty())
    predicateClosure = std::move(predicate_closure);
}
#endif

void PAMatchTree::Node::compute_size(PA_MATCH_TREE::MtSize& mt_size, unsigned depth) const { // NOLINT(misc-no-recursion)

    mt_size.inc_size();

    bool leaf = true;

    if (posNode) {
        posNode->compute_size(mt_size, depth + 1);
        leaf = false;
    }

    if (negNode) {
        negNode->compute_size(mt_size, depth + 1);
        leaf = false;
    }

    if (leaf) { mt_size.update_depth(depth); }

}

// FCT_IF_LAZY_PA(FCT_IF_DEBUG(void PAMatchTree::Node::dump_closure() const { PA_MATCH_TREE::dump_closure(get_closure()); }))

/**********************************************************************************************************************/

PAMatchTree::PAMatchTree(const PLAJA::Configuration& config, const PredicatesExpression& predicates):
    predicates(&predicates)
    , predicateRelations(nullptr)
    , root(new Node()) {

    if (config.get_bool_option(PLAJA_OPTION::predicate_relations)) {

        if (not config.has_sharable(PLAJA::SharableKey::PREDICATE_RELATIONS)) {
            config.set_sharable(PLAJA::SharableKey::PREDICATE_RELATIONS, std::shared_ptr<PredicateRelations>(PredicateRelations::construct(config, nullptr).release()));
        }

        predicateRelations = config.get_sharable<PredicateRelations>(PLAJA::SharableKey::PREDICATE_RELATIONS);

        PLAJA_ASSERT(predicateRelations->has_empty_ground_truth())

    }

    STMT_IF_LAZY_PA(const auto num_init_pred = predicates.get_number_predicates();)
    STMT_IF_LAZY_PA(for (PredicateIndex_type pred_index = 0; pred_index < num_init_pred; ++pred_index) { add_global_predicate(pred_index); })

}

PAMatchTree::~PAMatchTree() = default;

/**********************************************************************************************************************/

std::size_t PAMatchTree::get_size_predicates() const { return get_predicates().get_number_predicates(); }

void PAMatchTree::increment() { if (predicateRelations) { predicateRelations->increment(); } }

void PAMatchTree::add_global_predicate(const PredicateIndex_type predicate_index) { // NOLINT(readability-convert-member-functions-to-static)
#ifdef LAZY_PA
    PLAJA_ASSERT(std::all_of(globalPredicates.cbegin(), globalPredicates.cend(), [predicate_index](PredicateIndex_type other) { return predicate_index != other; }))
    globalPredicates.push_back(predicate_index);
#else
    MAYBE_UNUSED(predicate_index)
    PLAJA_ABORT
#endif
}

#ifdef LAZY_PA

void PAMatchTree::set_closure(PAMatchTree::Node& current_node, const std::vector<PredicateIndex_type>& predicate_closure, std::size_t index, PredicateState* prefix_state) { // NOLINT(misc-no-recursion)

    PLAJA_ASSERT(current_node.get_predicate_index() == PREDICATE::none)

    if (index >= predicate_closure.size()) { return; }

    const auto predicate_index = predicate_closure[index];

    PLAJA_ASSERT(not prefix_state or not prefix_state->is_entailed(predicate_index))

    current_node.set_predicate_index(predicate_index);

    // true

    auto next_index = index + 1;

    if (prefix_state) {
        prefix_state->push_layer();
        prefix_state->set(predicate_index, true);
        predicateRelations->fix_predicate_state(*prefix_state, predicate_index);
        while (next_index < predicate_closure.size() and prefix_state->is_entailed(predicate_closure[next_index])) { ++next_index; }
    }

    set_closure(current_node.add_successor(true), predicate_closure, next_index, prefix_state);

    if (prefix_state) { prefix_state->pop_layer(); }

    // false

    next_index = index + 1;

    if (prefix_state) {
        prefix_state->push_layer();
        prefix_state->set(predicate_index, false);
        predicateRelations->fix_predicate_state(*prefix_state, predicate_index);
        while (next_index < predicate_closure.size() and prefix_state->is_entailed(predicate_closure[next_index])) { ++next_index; }
    }

    set_closure(current_node.add_successor(false), predicate_closure, next_index, prefix_state);

    if (prefix_state) { prefix_state->pop_layer(); }

}

void PAMatchTree::set_closure(const PaStateBase& pa_state, std::vector<PredicateIndex_type>&& predicate_closure) {

    auto traverser = PA_MATCH_TREE::TraverserConstructor::init(*this);

    STMT_IF_DEBUG(for (auto pred_index: predicate_closure) { PLAJA_ASSERT(std::all_of(globalPredicates.cbegin(), globalPredicates.cend(), [pred_index](PredicateIndex_type other_index) { return pred_index != other_index; })) })

    while (not traverser.traversed(pa_state)) {

#if 0
#ifndef NDEBUG
        // sanity: partially redundant, though ...
        for (auto pred_index: predicate_closure) {
            PLAJA_ASSERT(std::all_of(traverser.get_closure().cbegin(), traverser.get_closure().cend(), [pred_index](PredicateIndex_type other_index) { return pred_index != other_index; }))
            PLAJA_ASSERT(std::all_of(traverser.node().get_closure().cbegin(), traverser.node().get_closure().cend(), [pred_index](PredicateIndex_type other_index) { return pred_index != other_index; }))
        }
        for (auto pred_index: traverser.get_closure()) {
            PLAJA_ASSERT(std::all_of(traverser.get_closure_prefix().cbegin(), traverser.get_closure_prefix().cend(), [pred_index](PredicateIndex_type other_index) { return pred_index != other_index; }))
        }
        for (auto pred_index: traverser.node().get_closure()) {
            PLAJA_ASSERT(std::all_of(traverser.get_closure_prefix().cbegin(), traverser.get_closure_prefix().cend(), [pred_index](PredicateIndex_type other_index) { return pred_index != other_index; }))
        }
#endif
#endif

        traverser.next(pa_state);
    }
    PLAJA_ASSERT(traverser.is_sane())
    PLAJA_ASSERT(traverser.closure.empty())

    PLAJA_ASSERT(traverser.is_final()) // expected: pa_state should be closed
    PLAJA_ASSERT(traverser.equals_prefix(pa_state))

    STMT_IF_DEBUG(for (auto pred_index: predicate_closure) { PLAJA_ASSERT(std::all_of(traverser.get_prefix().cbegin(), traverser.get_prefix().cend(), [pred_index](PredicateIndex_type other_index) { return pred_index != other_index; })) })

    STMT_IF_DEBUG(for (auto pred_index: predicate_closure) { PLAJA_ASSERT(not traverser.get_prefix_state().is_entailed(pred_index)) }) // no point to add entailed preds
    // traverser.currentNode->set_predicate_closure(std::move(predicate_closure));
    // auto prefix = traverser.get_prefix_state().to_pa_state_values();
    set_closure(*traverser.currentNode, predicate_closure, 0, traverser.mtPrefix.get());
}

#else

void PAMatchTree::set_closure(const PaStateBase& /*pa_state*/, std::vector<PredicateIndex_type>&& /*predicate_closure*/) { PLAJA_ABORT } // NOLINT(readability-convert-member-functions-to-static)

#endif

PA_MATCH_TREE::MtSize PAMatchTree::compute_size() const {
    PA_MATCH_TREE::MtSize mt_size;
    root->compute_size(mt_size, 0);
    return mt_size;
}

/**********************************************************************************************************************/

PA_MATCH_TREE::Traverser::Traverser(const PAMatchTree& match_tree):
    matchTree(&match_tree)
    , currentNode(matchTree->root.get())
    , mtPrefix(PLAJA_GLOBAL::debug or (not PLAJA_GLOBAL::lazyPA and matchTree->predicateRelations) ? new PredicateState(match_tree.get_size_predicates(), 0, &match_tree.get_predicates()) : nullptr)
// CONSTRUCT_IF_LAZY_PA(closure(currentNode->get_closure().cbegin(), currentNode->get_closure().cend()))
#ifndef LAZY_PA
, currentIndex(0)
#endif
{}

PA_MATCH_TREE::Traverser::~Traverser() = default;

#if 0

inline void PA_MATCH_TREE::Traverser::update() {
#ifdef LAZY_PA
    PLAJA_ASSERT(currentNode)

    STMT_IF_DEBUG(closureHistory.push_back(closure.front()))
    closure.pop_front();

    for (auto pred_index: currentNode->get_closure()) {
        PLAJA_ASSERT(std::all_of(closure.begin(), closure.end(), [pred_index](PredicateIndex_type other_index) { return pred_index != other_index; }))
        PLAJA_ASSERT(std::all_of(closureHistory.begin(), closureHistory.end(), [pred_index](PredicateIndex_type other_index) { return pred_index != other_index; }))
        closure.push_back(pred_index);
    }

    if (closure.empty()) { closure.push_back(matchTree->get_size_predicates()); }
#else
    ++currentIndex;
#endif
}

void PA_MATCH_TREE::Traverser::update_entailed() {
    STMT_IF_DEBUG(++entailedClosurePredicates)

#ifdef LAZY_PA

    STMT_IF_DEBUG(closureHistory.push_back(closure.front()))
    closure.pop_front();

    // first predicate in closure extension should not be entailed
    STMT_IF_DEBUG(if (currentNode) {
        for (auto pred_index: currentNode->get_closure()) {
            PLAJA_ASSERT(std::any_of(closure.begin(), closure.end(), [pred_index](PredicateIndex_type other_index) { return pred_index == other_index; }))
        }
    })

    if (closure.empty()) { closure.push_back(matchTree->get_size_predicates()); }

#else
    ++currentIndex;
#endif
}

#endif

void PA_MATCH_TREE::Traverser::next(const PaStateBase& pa_state) {
    PLAJA_ASSERT(not end())
    PLAJA_ASSERT(predicate_index() == PREDICATE::none or predicate_index() < pa_state.get_size_predicates())
    PLAJA_ASSERT(not mtPrefix or pa_state.get_size_predicates() <= mtPrefix->get_size_predicates())

    if (traversed(pa_state)) { currentNode = nullptr; }
    else {

        const auto current_index = predicate_index();
        PLAJA_ASSERT(current_index < pa_state.get_size_predicates())
        const bool truth_value = pa_state.predicate_value(current_index);
        PLAJA_ASSERT(std::all_of(get_prefix().begin(), get_prefix().end(), [current_index](PredicateIndex_type other_index) { return current_index != other_index; }))
        STMT_IF_DEBUG(closurePrefix.push_back(current_index))

        if (PLAJA_GLOBAL::debug or (not PLAJA_GLOBAL::lazyPA and matchTree->predicateRelations)) {
            PLAJA_ASSERT(not mtPrefix->is_entailed(current_index))

            mtPrefix->set(predicate_index(), truth_value);

            if (not PLAJA_GLOBAL::debug or matchTree->get_predicate_relations()) {
                matchTree->get_predicate_relations()->fix_predicate_state_asc_if_non_lazy(*mtPrefix, predicate_index());
            }

        }

        currentNode = currentNode->access_successor(truth_value);

#ifdef LAZY_PA

        // if (currentNode) { update(); }

#else

        if (not currentNode) { return; }

        while (++currentIndex < mtPrefix->get_size_predicates()) {
            PLAJA_ASSERT(not mtPrefix->is_set(currentIndex))
            if (not mtPrefix->is_entailed(currentIndex)) { break; }
        }
        if (currentIndex == mtPrefix->get_size_predicates()) { currentIndex = PREDICATE::none; }

#endif

    }
}

/**********************************************************************************************************************/

#ifndef NDEBUG

bool PA_MATCH_TREE::Traverser::is_final() const {
    // return currentNode and node().get_closure().empty() and closure == std::list<PredicateIndex_type> { static_cast<PredicateIndex_type>(matchTree->get_size_predicates()) };
    return currentNode and predicate_index() == PREDICATE::none;
}

std::size_t PA_MATCH_TREE::Traverser::get_prefix_size() const {

    if (not matchTree->get_predicate_relations()) { return get_prefix().size(); }

    PLAJA_ASSERT(mtPrefix)

    /* additional sanity check */
    PredicatesNumber_type num_set_or_entailed_preds = 0;

    for (auto it = mtPrefix->init_pred_state_iterator(); !it.end(); ++it) { num_set_or_entailed_preds += it.is_defined(); }

    PLAJA_ASSERT(num_set_or_entailed_preds >= get_prefix().size())
    /* */

    return get_prefix().size();
}

std::size_t PA_MATCH_TREE::Traverser::get_path_size() const {
    PLAJA_ASSERT(mtPrefix)

    PredicatesNumber_type num_set_preds = 0;

    for (auto it = mtPrefix->init_pred_state_iterator(); !it.end(); ++it) { num_set_preds += it.is_set(); }

    PLAJA_ASSERT((matchTree->predicateRelations and num_set_preds <= get_prefix().size()) or num_set_preds == get_prefix().size())

    return num_set_preds;
}

std::size_t PA_MATCH_TREE::Traverser::get_size_entailed_closure_predicates() const {

    if (not matchTree->get_predicate_relations()) { return 0; }

    PLAJA_ASSERT(mtPrefix)

    const auto entailed_closure_preds = get_prefix().size() - get_path_size();

    /* additional sanity check */
    PredicatesNumber_type num_entails = 0;

    for (auto it = mtPrefix->init_pred_state_iterator(); !it.end(); ++it) { num_entails += it.is_entailed(); }

    PLAJA_ASSERT(num_entails >= entailed_closure_preds)
    /* */

    return entailed_closure_preds;

}

bool PA_MATCH_TREE::Traverser::matches_prefix(const PaStateBase& pa_state) const {
    PLAJA_ASSERT(mtPrefix)

    PLAJA_ASSERT(is_final()) // expected

    for (const PredicateIndex_type pred: get_prefix()) { // NOLINT(readability-use-anyofallof)

        PLAJA_ASSERT(mtPrefix->is_defined(pred))

        if (not pa_state.is_set(pred)) { return false; }

        if (mtPrefix->is_set(pred)) { if (mtPrefix->predicate_value(pred) != pa_state.predicate_value(pred)) { return false; } }
        else {

            return false;

            // PLAJA_ASSERT(mtPrefix->is_entailed(pred))

            // if (mtPrefix->is_set(pred) and mtPrefix->entailment_value(pred) != pa_state.predicate_value(pred)) { return false; }

        }

    }

    return true;

}

bool PA_MATCH_TREE::Traverser::equals_prefix(const PaStateBase& pa_state) const {

    PLAJA_ASSERT(pa_state.get_size_predicates() <= mtPrefix->get_size_predicates())

    if (not matches_prefix(pa_state)) { return false; }

    for (auto it = pa_state.init_pred_state_it(); !it.end(); ++it) {

        const auto pred_index = it.predicate_index();

        if (it.is_set() and not mtPrefix->is_set(pred_index)) {

            if constexpr (PLAJA_GLOBAL::lazyPA) { return false; }

            if (not mtPrefix->is_entailed(pred_index) or mtPrefix->predicate_value(pred_index) != it.predicate_value()) { return false; }

        }

    }

    return matches_prefix(pa_state);

}

// void PA_MATCH_TREE::Traverser::dump_closure() const { PA_MATCH_TREE::dump_closure(closure); }

void PA_MATCH_TREE::Traverser::dump_predicates_prefix() const { PA_MATCH_TREE::dump_closure(get_prefix()); }

void PA_MATCH_TREE::Traverser::dump_prefix_state() const { if (mtPrefix) { mtPrefix->dump(); } }

#endif

/**********************************************************************************************************************/

PA_MATCH_TREE::TraverserConst::TraverserConst(const PAMatchTree& match_tree):
    Traverser(match_tree) {}

PA_MATCH_TREE::TraverserConst::~TraverserConst() = default;

//

PA_MATCH_TREE::TraverserModifier::TraverserModifier(PAMatchTree& match_tree):
    Traverser(match_tree) {}

PA_MATCH_TREE::TraverserModifier::~TraverserModifier() = default;

//

PA_MATCH_TREE::TraverserConstructor::TraverserConstructor(PAMatchTree& match_tree):
    Traverser(match_tree)
    CONSTRUCT_IF_LAZY_PA(closure(match_tree.get_global_predicates())) {

    if (not PLAJA_GLOBAL::debug and (PLAJA_GLOBAL::lazyPA or not matchTree->get_predicate_relations())) {
        PLAJA_ASSERT(not mtPrefix)
        mtPrefix = std::make_unique<PredicateState>(match_tree.get_size_predicates(), 0, &match_tree.get_predicates());
    }

    STMT_IF_LAZY_PA(update();)
}

PA_MATCH_TREE::TraverserConstructor::~TraverserConstructor() = default;

/**********************************************************************************************************************/


inline void PA_MATCH_TREE::TraverserConstructor::update() {

    PLAJA_ASSERT(currentNode)

#ifdef LAZY_PA

    while (not closure.empty() and mtPrefix->is_defined(closure.front())) { closure.pop_front(); } // global preds may have been set already due to local refinement

    if (not closure.empty() and predicate_index() == PREDICATE::none) {

        currentNode->set_predicate_index(closure.front());
        closure.pop_front();

    }

#else

    while (++currentIndex < mtPrefix->get_size_predicates()) {
        PLAJA_ASSERT(not mtPrefix->is_set(currentIndex))
        if (not mtPrefix->is_entailed(currentIndex)) { break; }
    }

    if (currentIndex == mtPrefix->get_size_predicates()) { currentIndex = PREDICATE::none; }

#endif

}

void PA_MATCH_TREE::TraverserConstructor::next(const PaStateBase& pa_state) {
    PLAJA_ASSERT(not end())
    PLAJA_ASSERT(predicate_index() == PREDICATE::none or predicate_index() < pa_state.get_size_predicates())
    PLAJA_ASSERT(pa_state.get_size_predicates() <= mtPrefix->get_size_predicates())

    if (traversed(pa_state)) { currentNode = nullptr; }
    else {

        const auto current_index = predicate_index();
        PLAJA_ASSERT(current_index < pa_state.get_size_predicates())
        const bool truth_value = pa_state.predicate_value(current_index);
        PLAJA_ASSERT(std::all_of(get_prefix().begin(), get_prefix().end(), [current_index](PredicateIndex_type other_index) { return current_index != other_index; }))
        STMT_IF_DEBUG(closurePrefix.push_back(current_index))

        PLAJA_ASSERT(not mtPrefix->is_entailed(current_index))

        mtPrefix->set(predicate_index(), truth_value);

        if (matchTree->get_predicate_relations()) { matchTree->get_predicate_relations()->fix_predicate_state_asc_if_non_lazy(*mtPrefix, predicate_index()); }

        currentNode = &currentNode->add_successor(truth_value);

        update();

    }

}

/**********************************************************************************************************************/

PAMatchTree::Node& PAMatchTree::add_path(const AbstractState& abstract_state) {

    auto it = PA_MATCH_TREE::TraverserConstructor::init(*this);

    while (not it.traversed(abstract_state)) { it.next(abstract_state); }

    return it.node();

}

PAMatchTree::Node& PAMatchTree::access_node(const AbstractState& abstract_state) {

    auto it = PA_MATCH_TREE::TraverserModifier::init(*this);

    while (not it.traversed(abstract_state)) { it.next(abstract_state); }

    return it.node();

}

const PAMatchTree::Node& PAMatchTree::retrieve_node(const AbstractState& abstract_state) const {

    auto it = PA_MATCH_TREE::TraverserConst::init(*this);

    while (not it.traversed(abstract_state)) { it.next(abstract_state); }

    return it.node();
}
