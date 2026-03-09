//
// This file is part of the PlaJA code base.
// Copyright (c) (2019 - 2019) Marcel Vinzent.
// See README.md in the top-level directory for licensing information.
//

#ifndef PLAJA_LOCATION_H
#define PLAJA_LOCATION_H

#include "../using_parser.h"
#include "iterators/ast_iterator.h"
#include "expression/forward_expression.h"
#include "ast_element.h"
#include "commentable.h"
#include "forward_ast.h"

class Location final: public AstElement, public Commentable {

private:
    std::string name;
    PLAJA::integer locationValue; // value of the location, unique value from [0,#automaton locations] of corresponding automaton (or -1 if invalid)

    // time progress
    std::unique_ptr<Expression> timeProgressCondition;
    FIELD_IF_COMMENT_PARSING(std::string timeProgressComment;)

    std::vector<std::unique_ptr<TransientValue>> transientValues;

public:
    Location();
    ~Location() override;
    DELETE_CONSTRUCTOR(Location)

    /* construction */

    void reserve(std::size_t tval_cap);

    void add_transient_value(std::unique_ptr<TransientValue>&& trans_val);

    /* setter */

    inline void set_name(std::string name_) { name = std::move(name_); }

    inline void set_locationValue(PLAJA::integer loc_val) { locationValue = loc_val; }

    void set_timeProgressExpression(std::unique_ptr<Expression>&& exp);

    inline void set_timeProgressComment(const std::string& tp_comment) { SET_IF_COMMENT_PARSING(timeProgressComment, tp_comment) }

    [[maybe_unused]] void set_transientValue(std::unique_ptr<TransientValue>&& trans_val, std::size_t index);

    /* getter */

    [[nodiscard]] inline const std::string& get_name() const { return name; }

    [[nodiscard]] inline PLAJA::integer get_locationValue() const { return locationValue; }

    [[nodiscard]] inline Expression* get_timeProgressCondition() { return timeProgressCondition.get(); }

    [[nodiscard]] inline const Expression* get_timeProgressCondition() const { return timeProgressCondition.get(); }

    [[nodiscard]] const std::string& get_timeProgressComment() const { return GET_IF_COMMENT_PARSING(timeProgressComment); }

    [[nodiscard]] inline std::size_t get_number_transientValues() const { return transientValues.size(); }

    [[nodiscard]] inline TransientValue* get_transientValue(std::size_t index) { return transientValues[index].get(); }

    [[nodiscard]] inline const TransientValue* get_transientValue(std::size_t index) const { return transientValues[index].get(); }

    [[nodiscard]] inline AstConstIterator<TransientValue> transientValueIterator() const { return AstConstIterator(transientValues); }

    [[nodiscard]] inline AstIterator<TransientValue> transientValueIterator() { return AstIterator(transientValues); }

    /** Deep copy of a location. */
    [[nodiscard]] std::unique_ptr<Location> deepCopy() const;

    /* override */
    void accept(AstVisitor* ast_visitor) override;
    void accept(AstVisitorConst* ast_visitor) const override;

};

#endif //PLAJA_LOCATION_H
