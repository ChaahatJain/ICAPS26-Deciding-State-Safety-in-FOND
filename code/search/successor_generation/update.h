//
// This file is part of the PlaJA code base.
// Copyright (c) (2019 - 2019) Marcel Vinzent.
// See README.md in the top-level directory for licensing information.
//

#ifndef PLAJA_UPDATE_H
#define PLAJA_UPDATE_H

#include <list>
#include <memory>
#include <unordered_map>
#include <unordered_set>
#include <vector>
#include "../../parser/ast/expression/forward_expression.h"
#include "../../parser/ast/forward_ast.h"
#include "../../parser/using_parser.h"
#include "../../assertions.h"
#include "../states/forward_states.h"

/* NOTE: It is not checked whether the same variable is assigned several times during the same sequence, this is done by writtenTo in state.h for runtime assertions. */

class Update { // A state operator, used by ActionOp as well as ActionOpInit.
    friend class SuccessorGeneratorBase; //
    friend struct SuccessorGeneratorEvaluator; //
    friend struct UpdateEvaluator; //
    friend class ActionOp; //
    friend class ActionOpInit; //
    friend class UpdateAbstract; //
    friend struct UpdateInMarabou;
    friend struct FaultAnalysis;

private:
    const Model* model;
    std::size_t numSequences; // num sequence index, i.e., the highest occurring index + 1
    std::vector<const Destination*> destinations;

    void add_destination(const Destination* destination);
    // void merge(const Update& update);

    /* Auxiliary (see evaluate below). */
    void evaluate_locations(StateValues* target) const;  // (Also see UpdateAbstract.)
    void evaluate_locations(State* target) const;

    /**
     * Intended to be encapsulated by successor generation.
     * @param target, must be copy of source to evaluate on.
     */
    void evaluate(State* target) const;
    void evaluate(StateValues* target) const;

public:
    explicit Update(const Model& model, std::vector<const Destination*> destinations);
    explicit Update(const Model& model, const Destination* destination);
    Update(const Update& update);
    Update(Update&& update) noexcept;
    DELETE_ASSIGNMENT(Update)
    ~Update();

    [[nodiscard]] inline std::size_t get_num_sequences() const { return numSequences; }

    [[nodiscard]] std::unique_ptr<State> evaluate(const State& source) const;
    [[nodiscard]] StateValues evaluate(const StateValues& source) const;

    /** Update agrees with the given source-target combination. */
    [[nodiscard]] bool agrees(const StateBase& source, const StateBase& target) const;

    /* Auxiliaries. */

    [[nodiscard]] std::unordered_set<VariableIndex_type> collect_updated_locs() const;
    std::unordered_set<VariableID_type> collect_updated_vars(SequenceIndex_type sequence_index) const; // NOLINT(modernize-use-nodiscard)
    [[nodiscard]] std::unordered_set<VariableIndex_type> collect_updated_state_indexes(SequenceIndex_type sequence_index, bool include_locs = false, const StateBase* source = nullptr) const;
    [[nodiscard]] std::unordered_set<VariableID_type> collect_updating_vars(SequenceIndex_type sequence_index) const;
    [[nodiscard]] std::unordered_set<VariableIndex_type> collect_updating_state_indexes(SequenceIndex_type sequence_index) const;
    std::unordered_map<VariableID_type, std::vector<const Expression*>> collect_updated_array_indexes(SequenceIndex_type sequence_index) const; // NOLINT(modernize-use-nodiscard)
    /**/
    std::unordered_set<VariableIndex_type> infer_non_updated_locs(const std::unordered_set<VariableIndex_type>* updated_locs = nullptr) const;
    std::unordered_set<VariableID_type> infer_non_updated_vars(SequenceIndex_type sequence_index, const std::unordered_set<VariableID_type>* updated_vars = nullptr) const;
    std::unordered_set<VariableIndex_type> infer_non_updated_state_indexes(SequenceIndex_type sequence_index, bool include_locs = false, const std::unordered_set<VariableIndex_type>* updated_state_indexes = nullptr) const;

    [[nodiscard]] bool is_non_deterministic(SequenceIndex_type sequence_index) const;
    [[nodiscard]] bool is_non_deterministic() const;

    /******************************************************************************************************************/

    class LocationIterator {
        friend Update;

    private:
        std::vector<const Destination*>::const_iterator it;
        std::vector<const Destination*>::const_iterator itEnd;
        explicit LocationIterator(const std::vector<const Destination*>& destinations);

    public:
        ~LocationIterator() = default;
        DELETE_CONSTRUCTOR(LocationIterator)

        inline void operator++() { ++it; }

        [[nodiscard]] inline bool end() const { return it == itEnd; }

        [[nodiscard]] VariableIndex_type loc() const;

        [[nodiscard]] PLAJA::integer dest() const;

    };

    [[nodiscard]] inline LocationIterator locationIterator() const { return LocationIterator(destinations); }

    class AssignmentIterator {
        friend Update;

    private:
        const SequenceIndex_type sequenceIndex; // Sequence index of interest.
        const std::vector<const Destination*>* destinations;
        std::size_t refDest;
        std::list<const Assignment*>::const_iterator it;
        std::list<const Assignment*>::const_iterator itEnd;

        [[nodiscard]] const Destination& get_destination(std::size_t index) const;

        explicit AssignmentIterator(const std::vector<const Destination*>& destinations, SequenceIndex_type seq_index);
    public:
        ~AssignmentIterator();
        DELETE_CONSTRUCTOR(AssignmentIterator)

        void operator++();

        [[nodiscard]] bool end() const;

        const Assignment* operator()() const;

        const Assignment* operator->() const;

        const Assignment& operator*() const;

        /* short-cuts */

        [[nodiscard]] const LValueExpression* ref() const;

        [[nodiscard]] VariableID_type var() const;

        [[nodiscard]] const Expression* array_index() const;

        [[nodiscard]] VariableIndex_type variable_index(const StateBase* state = nullptr) const;

        [[nodiscard]] const Expression* value() const;

        /** Should be used for deterministic assignments only. */
        void evaluate(const StateBase* current, StateBase& target) const;

        [[nodiscard]] bool is_non_deterministic() const;

        [[nodiscard]] const Expression* lower_bound() const;

        [[nodiscard]] const Expression* upper_bound() const;

        /** To be used for non-deterministic assignments. */
        void clip(const StateBase* current, StateBase& target) const;

    };

    [[nodiscard]] inline AssignmentIterator assignmentIterator(SequenceIndex_type seq_index) const { return AssignmentIterator(destinations, seq_index); }

};

#endif //PLAJA_UPDATE_H
