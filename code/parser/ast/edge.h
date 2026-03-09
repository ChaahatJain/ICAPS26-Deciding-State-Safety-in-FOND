//
// This file is part of the PlaJA code base.
// Copyright (c) (2019 - 2019) Marcel Vinzent.
// See README.md in the top-level directory for licensing information.
//

#ifndef PLAJA_EDGE_H
#define PLAJA_EDGE_H

#include "../using_parser.h"
#include "expression/forward_expression.h"
#include "iterators/ast_iterator.h"
#include "action.h"
#include "commentable.h"
#include "forward_ast.h"
#include "location.h"

class Edge final: public AstElement, public Commentable {

private:
    EdgeID_type id; // id for this edge (under edges in an automaton); defaults to EDGE::nullEdge, which is invalid for edge instances
    const Automaton* automaton;
    const Location* location;
    const Action* action;

    // rate
    std::unique_ptr<Expression> rateExpression;
    FIELD_IF_COMMENT_PARSING(std::string rateComment;)

    // guard
    std::unique_ptr<Expression> guardExpression;
    FIELD_IF_COMMENT_PARSING(std::string guardComment;)

    // destinations
    std::vector<std::unique_ptr<Destination>> destinations;

public:
    explicit Edge(const Automaton* automaton);
    ~Edge() override;
    DELETE_CONSTRUCTOR(Edge)

    /* Construction. */

    void reserve(std::size_t dest_cap);

    void add_destination(std::unique_ptr<Destination>&& dest);

    /* Setter. */

    inline void set_id(EdgeID_type edge_id) { id = edge_id; }

    inline void set_location(const Location* loc) { location = loc; }

    inline void set_action(const Action* action_ptr) { action = action_ptr; }

    void set_rateExpression(std::unique_ptr<Expression>&& rate_exp);

    inline void set_rateComment(const std::string& rate_comment) { SET_IF_COMMENT_PARSING(rateComment, rate_comment) }

    void set_guardExpression(std::unique_ptr<Expression>&& guard_exp);

    void set_guardComment(const std::string& guard_comment) { SET_IF_COMMENT_PARSING(guardComment, guard_comment) }

    void set_destination(std::unique_ptr<Destination>&& dest, std::size_t index);

    void set_destinations(std::vector<std::unique_ptr<Destination>>&& destinations);

    /* Getter. */

    [[nodiscard]] inline EdgeID_type get_id() const { return id; }

    [[nodiscard]] inline const std::string& get_location_name() const {
        PLAJA_ASSERT(location)
        return location->get_name();
    }

    [[nodiscard]] VariableIndex_type get_location_index() const;

    [[nodiscard]] inline PLAJA::integer get_location_value() const {
        PLAJA_ASSERT(location)
        return location->get_locationValue();
    }

    [[nodiscard]] inline const std::string& get_action_name() const { return action ? action->get_name() : ACTION::silentName; }

    [[nodiscard]] inline const Action* get_action() const { return action; }

    [[nodiscard]] inline ActionID_type get_action_id_labeled() const {
        PLAJA_ASSERT(action)
        PLAJA_ASSERT(action->get_id() != ACTION::nullAction)
        return action->get_id();
    }

    [[nodiscard]] inline ActionID_type get_action_id() const { return action ? get_action_id_labeled() : ACTION::silentAction; }

    [[nodiscard]] inline Expression* get_rateExpression() { return rateExpression.get(); }

    [[nodiscard]] inline const Expression* get_rateExpression() const { return rateExpression.get(); }

    [[nodiscard]] inline const std::string& get_rateComment() const { return GET_IF_COMMENT_PARSING(rateComment); }

    [[nodiscard]] inline Expression* get_guardExpression() { return guardExpression.get(); }

    [[nodiscard]] inline const Expression* get_guardExpression() const { return guardExpression.get(); }

    [[nodiscard]] inline const std::string& get_guardComment() const { return GET_IF_COMMENT_PARSING(guardComment); }

    [[nodiscard]] inline std::size_t get_number_destinations() const { return destinations.size(); }

    [[nodiscard]] inline Destination* get_destination(std::size_t index) {
        PLAJA_ASSERT(index < destinations.size())
        return destinations[index].get();
    }

    [[nodiscard]] inline const Destination* get_destination(std::size_t index) const {
        PLAJA_ASSERT(index < destinations.size())
        return destinations[index].get();
    }

    [[nodiscard]] inline AstConstIterator<Destination> destinationIterator() const { return AstConstIterator(destinations); }

    [[nodiscard]] inline AstIterator<Destination> destinationIterator() { return AstIterator(destinations); }

    /** Deep copy of an edge. */
    [[nodiscard]] std::unique_ptr<Edge> deepCopy() const;

    /* Override. */
    void accept(AstVisitor* ast_visitor) override;
    void accept(AstVisitorConst* ast_visitor) const override;

};

#endif //PLAJA_EDGE_H
