//
// This file is part of the PlaJA code base.
// Copyright (c) (2019 - 2020) Marcel Vinzent.
// See README.md in the top-level directory for licensing information.
//

#ifndef PLAJA_ACTION_OP_H
#define PLAJA_ACTION_OP_H

#include "../../parser/ast/expression/forward_expression.h"
#include "base/action_op_base.h"

class ActionOp final: public ActionOpBase {
    friend class SuccessorGeneratorC; //
    friend SUCCESSOR_GENERATION::ActionOpConstruction<Edge, ActionOp>;  //
    friend class ActionOpInMarabou;

public:

    struct ProbabilityStructure { // TODO may also cache probabilities on a per destination base with pointers to in multiplication structure
    private:
        std::vector<const Expression*> multiplication;
    public:
        ProbabilityStructure();
        explicit ProbabilityStructure(std::vector<const Destination*>& destinations);
        ~ProbabilityStructure();
        DEFAULT_CONSTRUCTOR_ONLY(ProbabilityStructure)
        DELETE_ASSIGNMENT(ProbabilityStructure)

        PLAJA::Prob_type probability; // Initialised probability.
        void add_factor(const Expression* factor);

        PLAJA::Prob_type compute(const StateBase* state) const;

        inline void initialise(const StateBase* state) { probability = compute(state); }
    };

private:
    std::vector<ProbabilityStructure> probabilityStructures;

    void initialise(const StateBase* source_state);

    /**
     * Merge this operator with an edge in terms of guard and updates,
     * i.e., ad guard and recombine probability structures and updates.
     * Assumptions: the automaton of the edge is not one of this operator.
     */
    void product_with(const Edge* edge);
    void product_with(const Edge* edge, ActionOpID_type additive_id);

    ActionOp(const Edge* edge, ActionOpID_type additive_id, const Model& model, SyncID_type sync_id = SYNCHRONISATION::nonSyncID);

    void compute_updates(std::vector<const Destination*>& destinations, std::size_t index, const Model& model);

    ActionOp(std::vector<const Edge*> edges, SyncID_type sync_id, ActionOpID_type op_id, const Model& model);
public:
    ~ActionOp() override;
    DEFAULT_CONSTRUCTOR_ONLY(ActionOp)
    DELETE_ASSIGNMENT(ActionOp)

    static std::unique_ptr<ActionOp> construct_operator(const ActionOpStructure& action_op_structure, const Model& model);
    static std::unique_ptr<ActionOp> construct_operator(std::vector<const Edge*> edges, SyncID_type sync_id, const Model& model);

    [[nodiscard]] inline PLAJA::Prob_type get_cached_probability(UpdateIndex_type index) const {
        PLAJA_ASSERT(index < probabilityStructures.size())
        return probabilityStructures[index].probability;
    }

    [[nodiscard]] inline PLAJA::Prob_type has_zero_probability(UpdateIndex_type index) const;

    [[nodiscard]] bool is_enabled(const StateBase& state) const;
    [[nodiscard]] PLAJA::Prob_type evaluate_probability(UpdateIndex_type index, const StateBase& state) const;
    [[nodiscard]] bool has_zero_probability(UpdateIndex_type index, const StateBase& state) const;

    /** iterators *****************************************************************************************************/

    template<bool initialized = false>
    class UpdateIteratorBase final: public ActionOpBase::UpdateIteratorBase {
        friend ActionOp;
    private:
        const std::vector<ProbabilityStructure>* probabilityStructures;

        explicit UpdateIteratorBase(const std::vector<ProbabilityStructure>& probability_structures, const std::vector<Update>& updates_ref):
            ActionOpBase::UpdateIteratorBase(updates_ref)
            , probabilityStructures(&probability_structures) {
            if constexpr (initialized) { while (not end() and (*probabilityStructures)[refIndex].probability == 0) { ++refIndex; } } // Skip 0 probabilities.
        }

    public:
        ~UpdateIteratorBase() = default;
        DELETE_CONSTRUCTOR(UpdateIteratorBase)

        inline void operator++() {
            ++refIndex;
            if constexpr (initialized) { while (!end() and (*probabilityStructures)[refIndex].probability == 0) { ++refIndex; } } // Skip 0 probabilities.
        }

        [[nodiscard]] inline PLAJA::Prob_type prob(const StateBase& state) const { return (*probabilityStructures)[refIndex].compute(&state); }

        [[nodiscard]] inline PLAJA::Prob_type prob() const { return (*probabilityStructures)[refIndex].probability; }

    };

    using UpdateIterator = UpdateIteratorBase<false>;
    using TransitionIterator = UpdateIteratorBase<true>;

    [[nodiscard]] inline UpdateIterator updateIterator() const { return UpdateIterator(probabilityStructures, updates); }

    [[nodiscard]] inline TransitionIterator transitionIterator() const { return TransitionIterator(probabilityStructures, updates); }

    [[nodiscard]] inline TransitionIterator transitionIterator(const StateBase& state) {
        initialise(&state);
        return TransitionIterator(probabilityStructures, updates);
    }

};

#endif //PLAJA_ACTION_OP_H
