//
// This file is part of the PlaJA code base.
// Copyright (c) (2019 - 2023) Marcel Vinzent.
// See README.md in the top-level directory for licensing information.
//


#ifndef PLAJA_PREDICATE_TRAVERSER_H
#define PLAJA_PREDICATE_TRAVERSER_H

#include "../../../include/ct_config_const.h"
#include "../../../utils/default_constructors.h"
#include "../pa_states/pa_state_base.h"
#include "../using_predicate_abstraction.h"

#ifdef LAZY_PA

static_assert(PLAJA_GLOBAL::useIncMt);

#ifdef USE_MATCH_TREE

#include "pa_match_tree.h"

// TODO ideally we would like to maintain only one PA stack.
//  Currently: one inside traverser and one for the PA state under construction.
//  Issue: The latter involves entailments not due binary predicate relations, i.e., not generally true for the predicate state but specific to the transition problem at hand.
//  Solution: PredicateState might introduce two entailment values "unconditionally" and "conditionally".

class PredicateTraverser final {

    const PAMatchTree* matchTree;
    //
    PAMatchTree::Node* currentNode;
    //
    std::unique_ptr<PredicateState> mtPrefix;
    //
    std::list<PredicateIndex_type> closure;

    std::list<PAMatchTree::Node*> nodeStack;
    std::list<std::list<PredicateIndex_type>> closureStack;

    // void update();
    void push();
    void pop();

    explicit PredicateTraverser(const PAMatchTree& mt);
public:
    ~PredicateTraverser();
    DELETE_CONSTRUCTOR(PredicateTraverser)

    [[nodiscard]] inline static PredicateTraverser init(const PAMatchTree& mt) { return PredicateTraverser(mt); }

    void next(const PaStateBase& pa_state);
    void previous();

    [[nodiscard]] bool has_node() const;

    [[nodiscard]] inline bool end() const { return closure.empty() and (not currentNode or currentNode->get_predicate_index() == PREDICATE::none); }

    [[nodiscard]] inline bool traversed(const PaStateBase& pa_state) const { return predicate_index() >= pa_state.get_size_predicates() or (PLAJA_GLOBAL::lazyPA and not pa_state.is_set(predicate_index())); }

    [[nodiscard]] inline PredicateIndex_type predicate_index() const {
        if (currentNode) {
            const auto pred_index = currentNode->get_predicate_index();
            if (pred_index != PREDICATE::none) { return pred_index; }
        }

        PLAJA_ASSERT(not closure.empty())
        return closure.front();

    }

    // FCT_IF_DEBUG([[nodiscard]] std::size_t get_closure_size();)

    FCT_IF_DEBUG([[nodiscard]] static bool is_closed(const PAMatchTree& mt, const PaStateBase& pa_state, bool minimal);)

    FCT_IF_DEBUG([[nodiscard]] bool is_closed(const PaStateBase& pa_state, bool minimal) const { return is_closed(*matchTree, pa_state, minimal); })

};

#endif

#else

class PredicateTraverser final {

    PredicatesNumber_type numPreds;
    PredicateIndex_type predIndex;

    explicit PredicateTraverser(const PredicatesNumber_type num_preds):
        numPreds(num_preds)
        , predIndex(0) {}

public:
    ~PredicateTraverser() = default;
    DELETE_CONSTRUCTOR(PredicateTraverser)

    [[nodiscard]] inline static PredicateTraverser init(const PredicatesNumber_type num_preds) { return PredicateTraverser(num_preds); }

    inline void next(const PaStateBase& /*pa_state*/) { ++predIndex; }

    inline void previous() { --predIndex; }

    [[nodiscard]] inline bool end() const { return predIndex >= numPreds; }

    [[nodiscard]] inline bool traversed(const PaStateBase& pa_state) const { return predicate_index() >= pa_state.get_size_predicates(); }

    [[nodiscard]] inline PredicateIndex_type predicate_index() const { return predIndex; }

};

#endif

#endif //PLAJA_PREDICATE_TRAVERSER_H
