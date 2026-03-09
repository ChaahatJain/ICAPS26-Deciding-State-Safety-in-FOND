//
// This file is part of the PlaJA code base.
// Copyright (c) (2019 - 2019) Marcel Vinzent.
// See README.md in the top-level directory for licensing information.
//

#ifndef PLAJA_DESTINATION_H
#define PLAJA_DESTINATION_H

#include <list>
#include "../../search/states/forward_states.h"
#include "../using_parser.h"
#include "expression/forward_expression.h"
#include "iterators/ast_iterator.h"
#include "commentable.h"
#include "forward_ast.h"
#include "location.h"

class Destination final: public AstElement, public Commentable {
private:
    const Automaton* automaton;
    const Location* location;

    // probability
    std::unique_ptr<Expression> probabilityExpression;
    FIELD_IF_COMMENT_PARSING(std::string probabilityComment;)

    // assignments
    std::vector<std::unique_ptr<Assignment>> assignments;
    std::vector<std::list<const Assignment*>> assignmentsPerSeq;

public:
    explicit Destination(const Automaton* automaton);
    ~Destination() override;
    DELETE_CONSTRUCTOR(Destination)

    /* Construction. */

    /**
     * No effect, if num_seq is smaller-equal get_number_assignments.
     * @param num_seq, the maximal number of required sequences.
     */
    [[maybe_unused]] void add_sequences(std::size_t num_seq);

    void reserve(std::size_t assignments_cap);

    void add_assignment(std::unique_ptr<Assignment>&& assignment);

    /* Setter. */

    inline void set_automaton(const Automaton* automaton_ptr) { automaton = automaton_ptr; }

    inline void set_location(const Location* loc) { location = loc; }

    void set_probabilityExpression(std::unique_ptr<Expression>&& prob_exp);

    inline void set_probabilityComment(const std::string& prob_comment) { SET_IF_COMMENT_PARSING(probabilityComment, prob_comment) }

    void set_assignment(std::unique_ptr<Assignment>&& assignment, std::size_t index);

    /* Getter. */

    [[nodiscard]] inline const std::string& get_location_name() const {
        PLAJA_ASSERT(location)
        return location->get_name();
    }

    [[nodiscard]] VariableIndex_type get_location_index() const;

    [[nodiscard]] inline PLAJA::integer get_location_value() const {
        PLAJA_ASSERT(location)
        return location->get_locationValue();
    }

    [[nodiscard]] inline Expression* get_probabilityExpression() { return probabilityExpression.get(); }

    [[nodiscard]] inline const Expression* get_probabilityExpression() const { return probabilityExpression.get(); }

    [[nodiscard]] const std::string& get_probabilityComment() const { return GET_IF_COMMENT_PARSING(probabilityComment); }

    [[nodiscard]] inline std::size_t get_number_assignments() const { return assignments.size(); }

    [[nodiscard]] inline Assignment* get_assignment(std::size_t index) {
        PLAJA_ASSERT(index < assignments.size())
        return assignments[index].get();
    }

    [[nodiscard]] inline const Assignment* get_assignment(std::size_t index) const {
        PLAJA_ASSERT(index < assignments.size())
        return assignments[index].get();
    }

    [[nodiscard]] inline std::size_t get_number_sequences() const { return assignmentsPerSeq.size(); }

    [[nodiscard]] inline std::size_t get_number_assignments_per_seq(SequenceIndex_type index) const {
        PLAJA_ASSERT(index < assignmentsPerSeq.size())
        return assignmentsPerSeq[index].size();
    }

    [[nodiscard]] inline const std::list<const Assignment*>& get_assignments_per_seq(SequenceIndex_type seq_index) const {
        PLAJA_ASSERT(seq_index < assignmentsPerSeq.size())
        return assignmentsPerSeq[seq_index];
    }

    /* */

    /** Deep copy of a destination. */
    [[nodiscard]] std::unique_ptr<Destination> deepCopy() const;

    void evaluate(const StateBase* current, StateBase* target, SequenceIndex_type seq_index) const;

    /* Override. */
    void accept(AstVisitor* ast_visitor) override;
    void accept(AstVisitorConst* ast_visitor) const override;

    /* Iterators. */

    [[nodiscard]] inline AstConstIterator<Assignment> assignmentIterator() const { return AstConstIterator(assignments); }

    /* */

    class AssignmentIterator {
    private:
        Destination& destination;
        std::size_t index;

    public:
        explicit AssignmentIterator(Destination& destination_):
            destination(destination_)
            , index(0) {}

        inline void operator++() { ++index; }

        [[nodiscard]] inline bool end() const { return index == destination.get_number_assignments(); }

        [[nodiscard]] inline Assignment* operator()() const { return destination.get_assignment(index); }

        [[nodiscard]] inline Assignment* operator->() const { return destination.get_assignment(index); }

        [[nodiscard]] inline Assignment& operator*() const { return *destination.get_assignment(index); }

        inline void set(std::unique_ptr<Assignment>&& assignment) const { destination.set_assignment(std::move(assignment), index); }

    };

    [[nodiscard]] inline AssignmentIterator assignmentIterator() { return AssignmentIterator(*this); }

    /* */

    class AssignmentIteratorPerSequence {
        friend Destination;
    private:
        const std::list<const Assignment*>& assignments;
        std::list<const Assignment*>::const_iterator it;
        const std::list<const Assignment*>::const_iterator itEnd;

        explicit AssignmentIteratorPerSequence(const std::list<const Assignment*>& assignments_):
            assignments(assignments_)
            , it(assignments.cbegin())
            , itEnd(assignments.end()) {
        }

    public:
        inline void operator++() { ++it; }

        [[nodiscard]] inline bool end() const { return it == itEnd; }

        inline const Assignment* operator->() { return *it; }
    };

    [[nodiscard]] inline AssignmentIteratorPerSequence assignmentIterator(SequenceIndex_type seq_index) const {
        return AssignmentIteratorPerSequence(get_assignments_per_seq(seq_index));
    }
};

#endif //PLAJA_DESTINATION_H
