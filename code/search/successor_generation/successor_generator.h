//
// This file is part of the PlaJA code base.
// Copyright (c) (2019 - 2019) Marcel Vinzent.
// See README.md in the top-level directory for licensing information.
//

#ifndef PLAJA_SUCCESSOR_GENERATOR_H
#define PLAJA_SUCCESSOR_GENERATOR_H

#include <list>
#include <memory>
#include <vector>
#include "action_op_init.h"
#include "base/iterators.h"
#include "base/successor_generator_base.h"

/** constructing action operator instances initialized per source state on-the-fly */
class SuccessorGenerator: public SuccessorGeneratorBase {
private:
    // exploration structures
    std::vector<std::vector<ActionOpInit>> operatorList; // per synchronisation, where i=0 for non-sync operators and i > 0 for sync operators of index i - 1
    std::list<SyncID_type> silentOps; // enabled (silent) action operators per synchronisation
    std::vector<std::list<SyncID_type>> labeledOps; // enabled action operators per synchronisation and per action
    std::vector<unsigned int> enabledSynchronisations; // number of (enabled) ops per sync
    std::vector<std::vector<std::list<PreOpInit>>> enabledEdges; // enabled edges per automaton and per non-silent action

    // exploration sub-routines:
    void reset_exploration_structures();
    void compute_enabled_synchronisations();
    void process_local_op(SyncIndex_type sync_index, std::vector<ActionOpInit>& sync_ops, std::list<PreOpInit>& enabled_edges_loc, const StateBase& state);

public:
    explicit SuccessorGenerator(const PLAJA::Configuration& config, const Model& model);
    ~SuccessorGenerator() override;
    DELETE_CONSTRUCTOR(SuccessorGenerator)

    void explore(const StateBase& state);

    /******************************************************************************************************************/

    using PerSyncIterator = ActionOpIteratorBase<ActionOpInit, ActionOpInit, std::vector>;

    class PerActionIterator {
        friend SuccessorGenerator;

    private:
        const std::vector<std::vector<ActionOpInit>>* operatorList;
        std::list<SyncID_type>::const_iterator it;
        std::list<SyncID_type>::const_iterator itEnd;
        PerSyncIterator itSync;

        explicit PerActionIterator(const std::vector<std::vector<ActionOpInit>>& operator_list, const std::list<SyncID_type>& sync_list):
            operatorList(&operator_list)
            , it(sync_list.cbegin())
            , itEnd(sync_list.cend())
            , itSync(operator_list[*it]) {
            PLAJA_ASSERT(!sync_list.empty())
        }

    public:
        inline void operator++() {
            ++itSync;
            if (itSync.end()) {
                ++it;
                if (!end()) { itSync = PerSyncIterator((*operatorList)[*it]); }
            }
        }

        [[nodiscard]] inline bool end() const { return it == itEnd; }

        [[nodiscard]] inline const ActionOpInit* operator()() const { return itSync(); }

        [[nodiscard]] inline const ActionOpInit* operator->() const { return itSync.operator->(); }

        [[nodiscard]] inline const ActionOpInit& operator*() const { return *itSync; }
    };

    [[nodiscard]] inline PerActionIterator init_silent_action_it_per_explore() const { return PerActionIterator(operatorList, silentOps); }

    [[nodiscard]] inline PerActionIterator init_labeled_action_it_per_explore(ActionLabel_type action_label) const {
        PLAJA_ASSERT(0 <= action_label && action_label <= labeledOps.size())
        return PerActionIterator(operatorList, labeledOps[action_label]);
    }

    class ActionLabelIterator {
        friend SuccessorGenerator;

    private:
        const std::vector<std::vector<ActionOpInit>>& operatorList;
        std::list<std::pair<ActionLabel_type, const std::list<SyncID_type>*>> applicable_actions;
        typename std::list<std::pair<ActionLabel_type, const std::list<SyncID_type>*>>::const_iterator it;
        typename std::list<std::pair<ActionLabel_type, const std::list<SyncID_type>*>>::const_iterator itEnd;

        explicit ActionLabelIterator(const std::vector<std::vector<ActionOpInit>>& operator_list, const std::list<SyncID_type>& silent_ops, const std::vector<std::list<SyncID_type>>& labeled_ops):
            operatorList(operator_list) {
            // by invariant each element in std::list<SyncID_type> is non-empty,
            // thus there is at least one action op (under the action label) iff list is non-empty
            // silent
            if (not silent_ops.empty()) { applicable_actions.emplace_back(ACTION::silentAction, &silent_ops); }
            // labeled
            ActionLabel_type action_label = 0;
            for (const auto& labeled_ops_per_action: labeled_ops) {
                if (not labeled_ops_per_action.empty()) { applicable_actions.emplace_back(action_label, &labeled_ops_per_action); }
                ++action_label;
            }

            it = applicable_actions.cbegin();
            itEnd = applicable_actions.cend();
        }

    public:
        inline void operator++() { ++it; }

        [[nodiscard]] inline bool end() const { return it == itEnd; }

        [[nodiscard]] inline ActionLabel_type action_label() const { return it->first; }

        [[nodiscard]] inline PerActionIterator iterator() const { return PerActionIterator(operatorList, *it->second); }

    };

    [[nodiscard]] inline ActionLabelIterator init_action_it_per_explore() const { return ActionLabelIterator(operatorList, silentOps, labeledOps); }

    /*
    class ActionOpIterator {
        friend SuccessorGenerator;

    private:
        ActionLabelIterator itLabel;
        PerActionIterator itOp;

        explicit ActionOpIterator(const std::vector<std::vector<ActionOpInit>>& operator_list, const std::list<SyncID_type>& silent_ops, const std::vector<std::list<SyncID_type>>& labeled_ops):
                itLabel(operator_list, silent_ops, labeled_ops),
                itOp(itLabel.end() ? PerActionIterator() : itLabel.iterator()) {
            PLAJA_ASSERT(itLabel.end() == it.end()) // sanity assert
        }

    public:
        inline void operator++() {
            ++itOp;
            if (itOp.end()) {
                ++itLabel;
                if (!itLabel.end()) { itOp = itLabel.iterator(); PLAJA_ASSERT(!it.end()) } // sanity assert
            }
        }
        [[nodiscard]] inline bool end() const { return itLabel.end(); }
        [[nodiscard]] inline const ActionOpInit* operator()() const { return itOp(); }
        [[nodiscard]] inline const ActionOpInit* operator->() const { return itOp.operator->(); }
        [[nodiscard]] inline const ActionOpInit& operator*() const { return *itOp; }

    };

    [[nodiscard]] inline ActionOpIterator actionOpIterator() const { return ActionOpIterator(operatorList, silentOps, labeledOps); }
     */

    /******************************************************************************************************************/

    class ActionOpIterator {
        friend SuccessorGenerator;
    private:
        const std::vector<std::vector<ActionOpInit>>& operatorList;
        unsigned int ref_syn;
        unsigned int ref_op;

        explicit ActionOpIterator(const std::vector<std::vector<ActionOpInit>>& operator_list):
            operatorList(operator_list)
            , ref_syn(0)
            , ref_op(0) {
            while (ref_syn < operatorList.size()) {
                if (ref_op < operatorList[ref_syn].size()) { break; }
                ++ref_syn;
            }
        }

    public:
        [[nodiscard]] inline bool end() const { return ref_syn >= operatorList.size(); }

        inline void operator++() {
            if (++ref_op >= operatorList[ref_syn].size()) {
                ++ref_syn;
                ref_op = 0;
                // next syn list may be empty, then have to skip ...
                while (!end()) {
                    if (ref_op < operatorList[ref_syn].size()) { break; }
                    ++ref_syn;
                }
            }
        }

        [[nodiscard]] inline const ActionOpInit* operator()() const { return &operatorList[ref_syn][ref_op]; }

        [[nodiscard]] inline const ActionOpInit* operator->() const { return &operatorList[ref_syn][ref_op]; }

        [[nodiscard]] inline const ActionOpInit& operator*() const { return operatorList[ref_syn][ref_op]; }
    };

    [[nodiscard]] inline ActionOpIterator init_action_op_it_per_explore() const { return ActionOpIterator(operatorList); }

};

#endif //PLAJA_SUCCESSOR_GENERATOR_H
