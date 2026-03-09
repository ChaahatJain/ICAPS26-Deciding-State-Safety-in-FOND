//
// This file is part of the PlaJA code base.
// Copyright (c) (2019 - 2019) Marcel Vinzent.
// See README.md in the top-level directory for licensing information.
//

#include "edge.h"
#include "../visitor/ast_visitor.h"
#include "../visitor/ast_visitor_const.h"
#include "expression/expression.h"
#include "automaton.h"
#include "destination.h"

Edge::Edge(const Automaton* automaton):
    id(EDGE::nullEdge)
    , automaton(automaton)
    , location(nullptr)
    , action(nullptr)
    , rateExpression()
    CONSTRUCT_IF_COMMENT_PARSING(rateComment(PLAJA_UTILS::emptyString))
    , guardExpression()
    CONSTRUCT_IF_COMMENT_PARSING(guardComment(PLAJA_UTILS::emptyString))
    , destinations() {
}

Edge::~Edge() = default;

/* Construction. */

void Edge::reserve(std::size_t dest_cap) { destinations.reserve(dest_cap); }

void Edge::add_destination(std::unique_ptr<Destination>&& dest) { destinations.push_back(std::move(dest)); }

/* Setter. */

void Edge::set_rateExpression(std::unique_ptr<Expression>&& rate_exp) { rateExpression = std::move(rate_exp); }

void Edge::set_guardExpression(std::unique_ptr<Expression>&& guard_exp) { guardExpression = std::move(guard_exp); }

void Edge::set_destination(std::unique_ptr<Destination>&& dest, std::size_t index) {
    PLAJA_ASSERT(index < destinations.size())
    destinations[index] = std::move(dest);
}

void Edge::set_destinations(std::vector<std::unique_ptr<Destination>>&& destinations_) { destinations = std::move(destinations_); }

/* Getter. */

VariableIndex_type Edge::get_location_index() const {
    PLAJA_ASSERT(automaton)
    return automaton->get_index();
}

/* */

std::unique_ptr<Edge> Edge::deepCopy() const {
    std::unique_ptr<Edge> copy(new Edge(automaton));
    copy->copy_comment(*this);

    copy->id = id;
    copy->set_location(location);
    copy->set_action(action);

    if (rateExpression) { copy->rateExpression = rateExpression->deepCopy_Exp(); }
    COPY_IF_COMMENT_PARSING(copy, rateComment)

    if (guardExpression) { copy->guardExpression = guardExpression->deepCopy_Exp(); }
    COPY_IF_COMMENT_PARSING(copy, guardComment)

    copy->destinations.reserve(destinations.size());
    for (const auto& dest: destinations) { copy->destinations.emplace_back(dest->deepCopy()); }

    return copy;
}

/* override */

void Edge::accept(AstVisitor* ast_visitor) { ast_visitor->visit(this); }

void Edge::accept(AstVisitorConst* ast_visitor) const { ast_visitor->visit(this); }
