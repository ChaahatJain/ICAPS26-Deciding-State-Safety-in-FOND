//
// This file is part of the PlaJA code base.
// Copyright (c) (2019 - 2020) Marcel Vinzent.
// See README.md in the top-level directory for licensing information.
//

#ifndef PLAJA_SUCCESSOR_GENERATOR_C_H
#define PLAJA_SUCCESSOR_GENERATOR_C_H

#include <list>
#include <memory>
#include <unordered_set>
#include <vector>
#include "../../assertions.h"
#include "../states/forward_states.h"
#include "../successor_generation/forward_successor_generation.h"
#include "base/iterators.h"
#include "base/successor_generator_base.h"

struct ActionOpStructure;

namespace MARABOU_IN_PLAJA { class SuccessorGenerator; }
namespace VERITAS_IN_PLAJA { class SuccessorGenerator; }


/** caching ActionOps at initialization/construction time */
class SuccessorGeneratorC final: public SuccessorGeneratorBase {
    friend class SuccessorGeneratorAbstract; //
    friend class SuccessorGeneratorPA; //
    friend class SuccessorGeneratorToZ3; //
    friend MARABOU_IN_PLAJA::SuccessorGenerator; //
    friend VERITAS_IN_PLAJA::SuccessorGenerator;

private:
    // action op construction structures
    std::vector<std::unique_ptr<ActionOp>> nonSyncOps;
    std::vector<std::vector<std::unique_ptr<ActionOp>>> syncOps;

    // Actual operator list at exploration time.
    std::list<const ActionOp*> silentOps; // operators with silent label
    std::vector<std::list<const ActionOp*>> labeledOps; // labeled (non-silent) operators per action label

    // Per state exploration.
    const StateBase* cached_exploration_state;
    std::vector<unsigned int> enabledSynchronisations; // number of (enabled) operators per sync
    std::vector<std::vector<std::list<EdgeID_type>>> enabledEdges; // enabled edges per automaton and per non-silent action label

    // Action op construction sub-routines.
    void initialize_action_ops();

    void compute_enabled_synchronisations(); // compute number of enabled operators per sync
    void compute_sync_ops(ActionOpStructure& action_op_structure, SyncIndex_type sync_index, std::size_t condition_index); // recursively compute synchronisation ops

    // Exploration time routines.
    void reset_exploration_structures();
    void compute_enabled_sync_ops(SyncIndex_type sync_index, const std::vector<AutomatonSyncAction>& condition, ActionOpIndex_type action_op_index, std::size_t condition_index); // recursively compute enabled synchronisation ops

    // Internal auxiliaries for extraction.

    [[nodiscard]] const ActionOp& get_action_op(SyncID_type sync_id, ActionOpIndex_type action_op_index) const;
    [[nodiscard]] const ActionOp& explore_action_op(const StateBase& state, SyncID_type sync_id, ActionOpIndex_type action_op_index) const;

public:
    explicit SuccessorGeneratorC(const PLAJA::Configuration& config, const Model& model);
    ~SuccessorGeneratorC() override;
    DELETE_CONSTRUCTOR(SuccessorGeneratorC)

    void explore(const StateBase& state);
    [[nodiscard]] bool is_applicable(ActionLabel_type action_label) const; // with respect to explore

    /** Iterators (static). *******************************************************************************************/

    [[nodiscard]] inline auto init_non_syn_op_it() const { return ActionOpIteratorBase<const ActionOp, std::unique_ptr<ActionOp>, std::vector>(nonSyncOps); }

    [[nodiscard]] inline auto init_syn_op_it(SyncIndex_type syn_index) const {
        PLAJA_ASSERT(PLAJA_UTILS::is_non_negative(syn_index) and syn_index < syncOps.size())
        return ActionOpIteratorBase<const ActionOp, std::unique_ptr<ActionOp>, std::vector>(syncOps[syn_index]);
    }

    [[nodiscard]] inline auto init_silent_action_it_static() const { return ActionLabelPerSyncIteratorBase<const ActionOp, std::unique_ptr<ActionOp>, std::vector, SuccessorGeneratorC>(*this, get_sync_information().synchronisationsPerSilentAction, true); }

    [[nodiscard]] inline auto init_label_action_it_static(ActionLabel_type action_label) const {
        PLAJA_ASSERT(0 <= action_label and action_label < get_sync_information().synchronisationsPerAction.size())
        return ActionLabelPerSyncIteratorBase<const ActionOp, std::unique_ptr<ActionOp>, std::vector, SuccessorGeneratorC>(*this, get_sync_information().synchronisationsPerAction[action_label], false);
    }

    [[nodiscard]] inline auto init_action_it_static(ActionLabel_type action_label) const { return action_label == ACTION::silentLabel ? init_silent_action_it_static() : init_label_action_it_static(action_label); }

    [[nodiscard]] inline auto init_action_op_it_static() const { return AllActionOpIteratorBase<const ActionOp, std::unique_ptr<ActionOp>, std::vector>(nonSyncOps, syncOps); }

    /** Iterators per explore. ****************************************************************************************/

    [[nodiscard]] inline auto init_silent_action_it_per_explore() const { return ActionOpIteratorBase<const ActionOp, const ActionOp*, std::list>(silentOps); }

    [[nodiscard]] inline auto init_label_action_it_per_explore(ActionLabel_type action_label) const {
        PLAJA_ASSERT(0 <= action_label and action_label < labeledOps.size())
        return ActionOpIteratorBase<const ActionOp, const ActionOp*, std::list>(labeledOps[action_label]);
    }

    [[nodiscard]] inline auto init_action_it_per_explore() const { return ActionLabelIteratorBase<const ActionOp, const ActionOp*, std::list>(silentOps, labeledOps); }

    [[nodiscard]] inline auto init_action_op_it_per_explore() const { return AllActionOpIteratorBase<const ActionOp, const ActionOp*, std::list>(silentOps, labeledOps); }

    /** Extraction (invalidates iterators if non-const): */

    [[nodiscard]] const ActionOp& get_action_op(const ActionOpStructure& action_op_structure) const;
    [[nodiscard]] const ActionOp& get_action_op(ActionOpID_type action_op_id) const;
    [[nodiscard]] std::list<const ActionOp*> extract_ops_per_label(ActionLabel_type action_label) const;

    [[nodiscard]] ActionLabel_type get_action_label(const ActionOpStructure& action_op_structure) const;
    [[nodiscard]] ActionLabel_type get_action_label(ActionOpID_type action_op_id) const;

    [[nodiscard]] const ActionOp& explore_action_op(const StateBase& state, const ActionOpStructure& action_op_structure);
    [[nodiscard]] const ActionOp& explore_action_op(const StateBase& state, ActionOpID_type action_op_id);

    [[nodiscard]] bool is_applicable(const StateBase& state, const ActionOp& action_op) const;

    using SuccessorGeneratorBase::compute_successor;
    [[nodiscard]] std::unique_ptr<State> compute_successor(const State& source, const ActionOp& action_op, UpdateIndex_type update_index) const;
    [[nodiscard]] std::unique_ptr<State> compute_successor(const State& source, ActionOpID_type action_op_id, UpdateIndex_type update_index) const;
    [[nodiscard]] std::unique_ptr<State> compute_successor(const State& source, UpdateOpID_type update_op_id) const;

    [[nodiscard]] bool is_applicable(const StateBase& state, ActionLabel_type action_label) const;
    [[nodiscard]] std::list<const ActionOp*> get_action(const StateBase& state, ActionLabel_type action_label) const;
    [[nodiscard]] std::list<const ActionOp*> explore_action(const StateBase& state, ActionLabel_type action_label);

};

/**********************************************************************************************************************/

// extern for interfacing with Marabou-related structures:
namespace SUCCESSOR_GENERATOR_C {
    [[nodiscard]] extern std::list<const ActionOp*> extract_ops_per_label(const SuccessorGeneratorC& successor_generator, ActionLabel_type action_label);
}

#endif //PLAJA_SUCCESSOR_GENERATOR_C_H
