//
// This file is part of the PlaJA code base.
// Copyright (c) (2019 - 2019) Marcel Vinzent.
// See README.md in the top-level directory for licensing information.
//

#ifndef PLAJA_AUTOMATON_H
#define PLAJA_AUTOMATON_H

#include <memory>
#include <vector>
#include "ast_element.h"
#include "commentable.h"
#include "../using_parser.h"
#include "expression/forward_expression.h"
#include "iterators/ast_iterator.h"
#include "forward_ast.h"

class Automaton final: public AstElement, public Commentable {
private:
    std::string name;
    AutomatonIndex_type automatonIndex;
    std::vector<std::unique_ptr<VariableDeclaration>> variables;

    std::unique_ptr<Expression> restrictInitialExpression;
    FIELD_IF_COMMENT_PARSING(std::string restrictInitialComment;)

    std::vector<std::unique_ptr<Location>> locations;
    std::vector<PLAJA::integer> initialLocations; // initial locations by the value in the corresponding automaton's location's range
    std::vector<std::unique_ptr<Edge>> edges;

public:
    Automaton();
    ~Automaton() override;
    DELETE_CONSTRUCTOR(Automaton)

    /* construction */

    void reserve(size_t vars_cap, size_t locs_cap, size_t init_locs_cap, size_t edges_cap);

    void add_variable(std::unique_ptr<VariableDeclaration>&& var);

    void add_location(std::unique_ptr<Location>&& loc);

    void add_initialLocation(PLAJA::integer init_loc);

    void add_edge(std::unique_ptr<Edge>&& edge);

    /* setter */

    inline void set_name(std::string name_) { name = std::move(name_); }

    inline void set_index(AutomatonIndex_type automaton_index) { automatonIndex = automaton_index; }

    void set_variable(std::unique_ptr<VariableDeclaration> var, std::size_t index);

    void set_restrictInitialExpression(std::unique_ptr<Expression>&& restrict_init_exp);

    inline void set_restrictInitialComment(const std::string& restrict_init_comment) { SET_IF_COMMENT_PARSING(restrictInitialComment, restrict_init_comment); }

    [[maybe_unused]] void set_location(std::unique_ptr<Location> loc, std::size_t index);

    void set_initialLocation(PLAJA::integer init_loc, std::size_t index);

    void set_edge(std::unique_ptr<Edge>&& dest, std::size_t index);

    void set_edges(std::vector<std::unique_ptr<Edge>>&& edges_);

    /* getter */

    [[nodiscard]] inline const std::string& get_name() const { return name; }

    [[nodiscard]] inline AutomatonIndex_type get_index() const { return automatonIndex; }

    [[nodiscard]] inline std::size_t get_number_variables() const { return variables.size(); }

    [[nodiscard]] inline VariableDeclaration* get_variable(std::size_t index) {
        PLAJA_ASSERT(index < variables.size())
        return variables[index].get();
    }

    [[nodiscard]] inline const VariableDeclaration* get_variable(std::size_t index) const {
        PLAJA_ASSERT(index < variables.size())
        return variables[index].get();
    }

    [[nodiscard]] inline Expression* get_restrictInitialExpression() { return restrictInitialExpression.get(); }

    [[nodiscard]] inline const Expression* get_restrictInitialExpression() const { return restrictInitialExpression.get(); }

    [[nodiscard]] inline const std::string& get_restrictInitialComment() const { return GET_IF_COMMENT_PARSING(restrictInitialComment); }

    [[nodiscard]] inline std::size_t get_number_locations() const { return locations.size(); }

    [[nodiscard]] inline Location* get_location(std::size_t index) {
        PLAJA_ASSERT(index < locations.size())
        return locations[index].get();
    }

    [[nodiscard]] inline const Location* get_location(std::size_t index) const {
        PLAJA_ASSERT(index < locations.size())
        return locations[index].get();
    }

    [[nodiscard]] inline std::size_t get_number_initialLocations() const { return initialLocations.size(); }

    [[nodiscard]] inline PLAJA::integer get_initialLocation(std::size_t index) const {
        PLAJA_ASSERT(index < initialLocations.size())
        return initialLocations[index];
    }

    [[nodiscard]] inline const std::vector<PLAJA::integer>& _initial_locations() const { return initialLocations; }

    [[nodiscard]] inline std::size_t get_number_edges() const { return edges.size(); }

    [[nodiscard]] inline Edge* get_edge(std::size_t index) {
        PLAJA_ASSERT(index < edges.size())
        return edges[index].get();
    }

    [[nodiscard]] inline const Edge* get_edge(std::size_t index) const {
        PLAJA_ASSERT(index < edges.size())
        return edges[index].get();
    }

    /* iterators */

    [[nodiscard]] inline AstConstIterator<VariableDeclaration> variableIterator() const { return AstConstIterator(variables); }

    [[nodiscard]] inline AstIterator<VariableDeclaration> variableIterator() { return AstIterator(variables); }

    [[nodiscard]] inline AstConstIterator<Location> locationIterator() const { return AstConstIterator(locations); }

    [[nodiscard]] inline AstIterator<Location> locationIterator() { return AstIterator(locations); }

    [[nodiscard]] inline AstConstIterator<Edge> edgeIterator() const { return AstConstIterator(edges); }

    [[nodiscard]] inline AstIterator<Edge> edgeIterator() { return AstIterator(edges); }

    /* auxiliaries */

    [[nodiscard]] const std::string& get_location_name(std::size_t index) const;
    [[nodiscard]] const Location* get_location_by_name(const std::string& location_name) const;

    /** Currently linear in the number of edges. */
    [[nodiscard]] std::size_t get_edge_index_by_id(EdgeID_type edge_id) const;
    [[nodiscard]] const Edge* get_edge_by_id(EdgeID_type edge_id) const;

    /**
     * Remove local variable.
     * Linear in the number of variables.
     */
    std::unique_ptr<VariableDeclaration> remove_variable(std::size_t index);

    /**
     * Deep copy of an automaton.
     * References to globals (variables, actions ...) remain unchanged.
     */
    [[nodiscard]] std::unique_ptr<Automaton> deepCopy() const;

    /* override */
    void accept(AstVisitor* ast_visitor) override;
    void accept(AstVisitorConst* ast_visitor) const override;

};

#endif //PLAJA_AUTOMATON_H
