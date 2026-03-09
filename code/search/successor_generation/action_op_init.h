//
// This file is part of the PlaJA code base.
// Copyright (c) (2019 - 2019) Marcel Vinzent.
// See README.md in the top-level directory for licensing information.
//

#ifndef PLAJA_ACTION_OP_INIT_H
#define PLAJA_ACTION_OP_INIT_H

#include "base/action_op_base.h"

/** Pre-structure of ActionOpInit for single edge; to compute probabilities and thereby enabled destinations. */
class PreOpInit final {
    friend SuccessorGenerator; //
    friend ActionOpInit;

private:
    /* Action op id info. */
    const Edge* edgePtr;
    const AutomatonIndex_type automatonIndex;
    const Model* model;

    std::vector<PLAJA::Prob_type> probabilities;
    std::vector<const Destination*> destinations;
    bool initialised;

    [[nodiscard]] const Edge& get_edge() const;

    [[nodiscard]] inline bool is_initialised() const { return initialised; }

    /**
     * To be called before pre-op is added to actual source op.
     * Computes probabilities, checks probability sum and removes 0-probability destinations.
     * @param state
     */
    void initialise(const StateBase& state);

    [[nodiscard]] ActionOpID_type compute_additive_action_op_id(SyncIndex_type sync_index) const;

public:
    explicit PreOpInit(const Edge& edge, AutomatonIndex_type automaton_index, const Model& model); // Note, making constructor private does not work, as SuccessorGenerator uses "emplace_back".
    ~PreOpInit();
    DELETE_CONSTRUCTOR(PreOpInit)

};

/** Class to represent an action operator initialized for a fixed source state. */
class ActionOpInit final: public ActionOpBase {
    friend SuccessorGenerator;

private:
    const SyncID_type syncId;
    std::vector<PLAJA::Prob_type> probabilities;

    /**
     * Merge this operator with a pre-pre_op in terms of guard and updates,
     * i.e., add guard and recombine destinations.
     * Assumptions: the automaton of the edge in the pre-pre_op is not one of this operator.
     * @param pre_op
     */
    void product_with(const PreOpInit& pre_op);

public:
    /* Note, making constructors private does not work, as SuccessorGenerator uses "emplace_back". */

    ActionOpInit(const Edge& edge, const StateBase& state, ActionOpID_type action_op_id, const Model& model); // for non-sync

    /**
     * @param additive_id_sync, the additive impact of the sync ID to the action op ID.
     */
    explicit ActionOpInit(const PreOpInit& pre_op, ActionOpID_type additive_id_sync, SyncIndex_type sync_index); // for sync

    ~ActionOpInit() override;
    DEFAULT_CONSTRUCTOR_ONLY(ActionOpInit)
    DELETE_ASSIGNMENT(ActionOpInit)

    [[maybe_unused]] [[nodiscard]] inline PLAJA::Prob_type get_probability(TransitionIndex_type index) const {
        PLAJA_ASSERT(index < probabilities.size())
        return probabilities[index];
    }

    [[maybe_unused]] [[nodiscard]] inline const Update& get_transition(TransitionIndex_type index) const { return get_update(index); }

    class TransitionIterator final: public UpdateIteratorBase {
        friend ActionOpInit;
    private:
        const std::vector<PLAJA::Prob_type>* probabilities;

        TransitionIterator(const std::vector<PLAJA::Prob_type>& probabilities_ref, const std::vector<Update>& updates_ref):
            UpdateIteratorBase(updates_ref)
            , probabilities(&probabilities_ref) {
        }

    public:

        inline void operator++() { ++refIndex; }

        [[nodiscard]] inline PLAJA::Prob_type prob() const { return (*probabilities)[refIndex]; }

        [[nodiscard]] inline const Update& transition() const { return update(); }

        [[nodiscard]] inline TransitionIndex_type transition_index() const { return update_index(); }
    };

    [[nodiscard]] inline TransitionIterator transitionIterator() const { return TransitionIterator(probabilities, updates); } // NOLINT(modernize-return-braced-init-list)

};

#endif //PLAJA_ACTION_OP_INIT_H
