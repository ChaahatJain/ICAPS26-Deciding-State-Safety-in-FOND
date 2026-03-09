//
// This file is part of the PlaJA code base.
// Copyright (c) (2019 - 2021) Marcel Vinzent.
// See README.md in the top-level directory for licensing information.
//

#ifndef PLAJA_PER_ACTIONS_ITERATOR_H
#define PLAJA_PER_ACTIONS_ITERATOR_H

#include "../../../utils/iterator_utils.h"
#include "../../using_search.h"

template<typename ActionOpView_t, typename ActionOpEntry_t, template<typename, typename> typename Container_t>
class ActionOpIteratorBase final: public ContainerIteratorConst<ActionOpView_t, ActionOpEntry_t, Container_t> {

public:
    explicit ActionOpIteratorBase(const Container_t<ActionOpEntry_t, std::allocator<ActionOpEntry_t>>& operator_list):
        ContainerIteratorConst<ActionOpView_t, ActionOpEntry_t, Container_t>(operator_list) {
    }

    ~ActionOpIteratorBase() override = default;
    DEFAULT_CONSTRUCTOR(ActionOpIteratorBase)

#if 0
    /** Routine (for PA) to cache guard enabled value internally. */
    template<typename Solver_t, bool incremental>
    bool guard_enabled(Solver_t& solver) const {
        auto& action_op = **it;
        bool rlt;
        if constexpr (incremental) { rlt = action_op.guard_enabled_inc(solver); } // Does not pop afterwards.
        else { rlt = action_op.guard_enabled(solver); }
        action_op.set_guard_enabled(rlt);
        return rlt;
    }
#endif

};

/**********************************************************************************************************************/

template<typename ActionOpView_t, typename ActionOpEntry_t, template<typename, typename> typename Container_t, typename SuccessorGenerator_t>
class ActionLabelPerSyncIteratorBase final {

private:
    const SuccessorGenerator_t* parent;
    const std::vector<SyncIndex_type>* syncsPerLabel;
    int index;
    ActionOpIteratorBase<ActionOpView_t, ActionOpEntry_t, Container_t> it;

    /* Aux. */
    inline void increment_until_op() {
        if (it.end()) {
            const auto syncs_per_label_size = syncsPerLabel->size();
            for (++index; index < syncs_per_label_size; ++index) {
                it = ActionOpIteratorBase<ActionOpView_t, ActionOpEntry_t, Container_t>(parent->init_syn_op_it((*syncsPerLabel)[index]));
                if (not it.end()) { break; }
            }
        }
    }

public:

    explicit ActionLabelPerSyncIteratorBase(const SuccessorGenerator_t& parent_ref, const std::vector<SyncIndex_type>& syncs_per_label, bool use_non_sync):
        parent(&parent_ref)
        , syncsPerLabel(&syncs_per_label)
        , index(use_non_sync ? -1 : 0)
        , it(use_non_sync ? parent->init_non_syn_op_it() : parent->init_syn_op_it((*syncsPerLabel)[index])) {
        PLAJA_ASSERT(use_non_sync or not syncs_per_label.empty())
        increment_until_op();
    }

    ~ActionLabelPerSyncIteratorBase() = default;
    DEFAULT_CONSTRUCTOR(ActionLabelPerSyncIteratorBase)

    inline void operator++() {
        ++it;
        increment_until_op();
    }

    [[nodiscard]] inline bool end() const { return index >= syncsPerLabel->size(); }

    [[nodiscard]] inline const ActionOpView_t* operator()() const { return it(); }

    [[nodiscard]] inline const ActionOpView_t* operator->() const { return it.operator->(); }

    [[nodiscard]] inline const ActionOpView_t& operator*() const { return *it; }

};

/**********************************************************************************************************************/

template<typename ActionOpView_t, typename ActionOpEntry_t, template<typename, typename> typename Container_t>
class ActionLabelIteratorBase final {

private:
    std::list<std::pair<ActionLabel_type, const Container_t<ActionOpEntry_t, std::allocator<ActionOpEntry_t>>*>> applicableActions;
    typename std::list<std::pair<ActionLabel_type, const Container_t<ActionOpEntry_t, std::allocator<ActionOpEntry_t>>*>>::const_iterator it;
    typename std::list<std::pair<ActionLabel_type, const Container_t<ActionOpEntry_t, std::allocator<ActionOpEntry_t>>*>>::const_iterator itEnd;

public:

    explicit ActionLabelIteratorBase(const Container_t<ActionOpEntry_t, std::allocator<ActionOpEntry_t>>& silent_ops, const std::vector<Container_t<ActionOpEntry_t, std::allocator<ActionOpEntry_t>>>& labeled_ops) {

        /* Silent */
        if (not silent_ops.empty()) { applicableActions.emplace_back(ACTION::silentAction, &silent_ops); }

        /* Labeled. */
        ActionLabel_type action_label = 0;
        for (const auto& labeled_ops_per_action: labeled_ops) {
            if (not labeled_ops_per_action.empty()) { applicableActions.emplace_back(action_label, &labeled_ops_per_action); }
            ++action_label;
        }

        it = applicableActions.cbegin();
        itEnd = applicableActions.cend();
    }

    ~ActionLabelIteratorBase() = default;
    DEFAULT_CONSTRUCTOR(ActionLabelIteratorBase)

    inline void operator++() { ++it; }

    [[nodiscard]] inline bool end() const { return it == itEnd; }

    [[nodiscard]] inline ActionLabel_type action_label() const { return it->first; }

    [[nodiscard]] inline std::size_t num_applicable_ops() const { return it->second->size(); }

    [[nodiscard]] inline auto iterator() const { return ActionOpIteratorBase<ActionOpView_t, ActionOpEntry_t, Container_t>(*it->second); }

};

/**********************************************************************************************************************/

template<typename ActionOpView_t, typename ActionOpEntry_t, template<typename, typename> typename Container_t>
class AllActionOpIteratorBase final {

private:
    int index;
    const std::vector<Container_t<ActionOpEntry_t, std::allocator<ActionOpEntry_t>>>& opsPerIndex;
    ActionOpIteratorBase<ActionOpView_t, ActionOpEntry_t, Container_t> it;

    /* Aux. */
    inline void increment_until_op() {
        if (it.end()) {
            const auto ops_per_index_size = opsPerIndex.size();
            for (++index; index < ops_per_index_size; ++index) {
                it = ActionOpIteratorBase<ActionOpView_t, ActionOpEntry_t, Container_t>(opsPerIndex[index]);
                if (not it.end()) { break; }
            }
        }
    }

public:
    explicit AllActionOpIteratorBase(const Container_t<ActionOpEntry_t, std::allocator<ActionOpEntry_t>>& init_ops, const std::vector<Container_t<ActionOpEntry_t, std::allocator<ActionOpEntry_t>>>& additional_ops):
        index(-1)
        , opsPerIndex(additional_ops)
        , it(init_ops) {
        increment_until_op();
    }

    ~AllActionOpIteratorBase() = default;
    DELETE_CONSTRUCTOR(AllActionOpIteratorBase)

    inline void operator++() {
        ++it;
        increment_until_op();
    }

    [[nodiscard]] inline bool end() const { return index == opsPerIndex.size(); }

    [[nodiscard]] inline const ActionOpView_t* operator()() const { return it(); }

    [[nodiscard]] inline const ActionOpView_t* operator->() const { return it.operator->(); }

    [[nodiscard]] inline const ActionOpView_t& operator*() const { return *it; }

};

#endif //PLAJA_PER_ACTIONS_ITERATOR_H
