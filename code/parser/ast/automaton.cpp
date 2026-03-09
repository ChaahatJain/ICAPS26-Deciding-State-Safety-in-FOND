//
// This file is part of the PlaJA code base.
// Copyright (c) (2019 - 2019) Marcel Vinzent.
// See README.md in the top-level directory for licensing information.
//

#include "automaton.h"
#include "../../include/ct_config_const.h"
#include "../visitor/ast_visitor.h"
#include "../visitor/ast_visitor_const.h"
#include "edge.h"
#include "expression/expression.h"
#include "location.h"
#include "variable_declaration.h"

Automaton::Automaton():
    name(PLAJA_UTILS::emptyString)
    , automatonIndex(AUTOMATON::invalid)
    , variables()
    , restrictInitialExpression()
    CONSTRUCT_IF_COMMENT_PARSING(restrictInitialComment(PLAJA_UTILS::emptyString))
    , locations()
    , initialLocations()
    , edges() {
}

Automaton::~Automaton() = default;

/* construction */

void Automaton::reserve(std::size_t vars_cap, std::size_t locs_cap, std::size_t init_locs_cap, std::size_t edges_cap) {
    variables.reserve(vars_cap);
    locations.reserve(locs_cap);
    initialLocations.reserve(init_locs_cap);
    edges.reserve(edges_cap);
}

void Automaton::add_variable(std::unique_ptr<VariableDeclaration>&& var) { variables.push_back(std::move(var)); }

void Automaton::add_location(std::unique_ptr<Location>&& loc) { locations.push_back(std::move(loc)); }

void Automaton::add_initialLocation(PLAJA::integer init_loc) {
    PLAJA_ASSERT(0 <= init_loc && init_loc < locations.size())
    initialLocations.push_back(init_loc);
}

void Automaton::add_edge(std::unique_ptr<Edge>&& edge) {
    PLAJA_ASSERT(0 <= edge->get_location_value() and edge->get_location_value() < locations.size())
    edges.push_back(std::move(edge));
}

/* setter */

void Automaton::set_variable(std::unique_ptr<VariableDeclaration> var, std::size_t index) { variables[index] = std::move(var); }

void Automaton::set_restrictInitialExpression(std::unique_ptr<Expression>&& restrict_init_exp) { restrictInitialExpression = std::move(restrict_init_exp); }

[[maybe_unused]] void Automaton::set_location(std::unique_ptr<Location> loc, std::size_t index) {
    PLAJA_ASSERT(index < locations.size())
    locations[index] = std::move(loc);
}

void Automaton::set_initialLocation(PLAJA::integer init_loc, std::size_t index) {
    PLAJA_ASSERT(index < initialLocations.size())
    PLAJA_ASSERT(0 <= init_loc && init_loc < locations.size())
    initialLocations[index] = init_loc;
}

void Automaton::set_edge(std::unique_ptr<Edge>&& edge, std::size_t index) {
    PLAJA_ASSERT(index < edges.size())
    edges[index] = std::move(edge);
}

void Automaton::set_edges(std::vector<std::unique_ptr<Edge>>&& edges_) { edges = std::move(edges_); }

/* auxiliaries */

const std::string& Automaton::get_location_name(std::size_t index) const { return get_location(index)->get_name(); }

const Location* Automaton::get_location_by_name(const std::string& location_name) const {
    for (const auto& location: locations) { if (location->get_name() == location_name) { return location.get(); } }
    return nullptr;
}

std::size_t Automaton::get_edge_index_by_id(EdgeID_type edge_id) const {
    const auto num_edges = edges.size();
    for (std::size_t index = 0; index < num_edges; ++index) { if (get_edge(index)->get_id() == edge_id) { return index; } }
    PLAJA_ABORT
}

const Edge* Automaton::get_edge_by_id(EdgeID_type edge_id) const {
    for (const auto& edge: edges) { if (edge->get_id() == edge_id) { return edge.get(); } }
    PLAJA_ABORT
}

/**/

std::unique_ptr<VariableDeclaration> Automaton::remove_variable(std::size_t index) {
    PLAJA_ASSERT(get_variable(index))
    auto it = variables.begin() + PLAJA_UTILS::cast_numeric<long>(index);
    PLAJA_ASSERT(it->get() == get_variable(index))
    std::unique_ptr<VariableDeclaration> var_removed = std::move(*it);
    variables.erase(it);
    return var_removed;
}

/**/

std::unique_ptr<Automaton> Automaton::deepCopy() const {
    PLAJA_LOG_DEBUG(PLAJA_UTILS::to_red_string("Warning: Deep copying automaton might invalidate child-to-parent references."))
    PLAJA_ABORT_IF(not PLAJA_GLOBAL::debug) // Probably not used anyway. Crash for now.

    std::unique_ptr<Automaton> copy(new Automaton());
    copy->copy_comment(*this);
    copy->name = name;
    copy->automatonIndex = automatonIndex;

    copy->variables.reserve(variables.size());
    for (const auto& var: variables) { copy->variables.emplace_back(var->deep_copy()); }

    if (restrictInitialExpression) { copy->restrictInitialExpression = restrictInitialExpression->deepCopy_Exp(); }
    COPY_IF_COMMENT_PARSING(copy, restrictInitialComment)

    copy->locations.reserve(locations.size());
    for (const auto& loc: locations) { copy->locations.emplace_back(loc->deepCopy()); }

    copy->initialLocations = initialLocations;

    copy->edges.reserve(edges.size());
    for (const auto& edge: edges) { copy->edges.emplace_back(edge->deepCopy()); }

    return copy;
}

/* override */

void Automaton::accept(AstVisitor* ast_visitor) { ast_visitor->visit(this); }

void Automaton::accept(AstVisitorConst* ast_visitor) const { ast_visitor->visit(this); }






