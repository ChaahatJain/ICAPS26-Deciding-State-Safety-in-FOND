//
// This file is part of the PlaJA code base.
// Copyright (c) (2019 - 2019) Marcel Vinzent.
// See README.md in the top-level directory for licensing information.
//

#include "destination.h"
#include "../visitor/ast_visitor.h"
#include "../visitor/ast_visitor_const.h"
#include "assignment.h"
#include "automaton.h"
#include "expression/expression.h"

Destination::Destination(const Automaton* automaton):
    automaton(automaton)
    , location(nullptr)
    , probabilityExpression()
    CONSTRUCT_IF_COMMENT_PARSING(probabilityComment(PLAJA_UTILS::emptyString))
    , assignments() {
}

Destination::~Destination() = default;

/* Construction. */

[[maybe_unused]] void Destination::add_sequences(std::size_t num_seq) {
    if (assignmentsPerSeq.size() < num_seq) { assignmentsPerSeq.resize(num_seq); } // only resize if more than existing sequences required
}

void Destination::reserve(std::size_t assignments_cap) { assignments.reserve(assignments_cap); }

void Destination::add_assignment(std::unique_ptr<Assignment>&& assignment) {
    // cache per sequence:
    auto seq_index = assignment->get_index();
    if (assignmentsPerSeq.size() <= seq_index) { assignmentsPerSeq.resize(seq_index + 1); }
    assignmentsPerSeq[seq_index].push_back(assignment.get());
    //
    assignments.push_back(std::move(assignment));
}

/* Setter. */

void Destination::set_probabilityExpression(std::unique_ptr<Expression>&& prob_exp) { probabilityExpression = std::move(prob_exp); }

void Destination::set_assignment(std::unique_ptr<Assignment>&& assignment, std::size_t index) {
    PLAJA_ASSERT(index < assignments.size())
    // cache per sequence:
    auto seq_index = assignment->get_index();
    if (assignmentsPerSeq.size() <= seq_index) { assignmentsPerSeq.resize(seq_index + 1); }
    assignmentsPerSeq[seq_index].push_back(assignment.get());
    //
    std::swap(assignments[index], assignment);
    // de-cache old
    auto& assignments_per_seq = assignmentsPerSeq[assignment->get_index()];
    auto it_end = assignments_per_seq.end();
    for (auto per_seq_it = assignments_per_seq.begin(); per_seq_it != it_end; ++per_seq_it) {
        if (*per_seq_it == assignment.get()) {
            assignments_per_seq.erase(per_seq_it);
            return;
        }
    }
    PLAJA_ABORT // should remove exactly one element above
}

/* Getter. */

VariableIndex_type Destination::get_location_index() const {
    PLAJA_ASSERT(automaton)
    return automaton->get_index();
}

/* */

std::unique_ptr<Destination> Destination::deepCopy() const {
    std::unique_ptr<Destination> copy(new Destination(automaton));
    copy->copy_comment(*this);

    copy->set_location(location);

    if (probabilityExpression) { copy->probabilityExpression = probabilityExpression->deepCopy_Exp(); }
    COPY_IF_COMMENT_PARSING(copy, probabilityComment)

    // assignments
    copy->reserve(get_number_assignments());
    copy->add_sequences(get_number_sequences());
    for (const auto& assignment: assignments) { copy->add_assignment(assignment->deep_copy()); }

    return copy;
}

void Destination::evaluate(const StateBase* current, StateBase* target, SequenceIndex_type seq_index) const {
    PLAJA_ASSERT(seq_index < get_number_sequences())
    for (const auto& assignment: assignmentsPerSeq[seq_index]) {
        PLAJA_ASSERT(assignment->get_index() == seq_index)
        assignment->evaluate(current, target);
    }
}

/* Override. */

void Destination::accept(AstVisitor* ast_visitor) { ast_visitor->visit(this); }

void Destination::accept(AstVisitorConst* ast_visitor) const { ast_visitor->visit(this); }










