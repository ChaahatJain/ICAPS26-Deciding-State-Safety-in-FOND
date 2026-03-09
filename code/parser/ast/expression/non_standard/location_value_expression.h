//
// This file is part of the PlaJA code base.
// Copyright (c) (2019 - 2023) Marcel Vinzent.
// See README.md in the top-level directory for licensing information.
//

#ifndef PLAJA_LOCATION_VALUE_EXPRESSION_H
#define PLAJA_LOCATION_VALUE_EXPRESSION_H

#include "../../forward_ast.h"
#include "../expression.h"

class LocationValueExpression final: public Expression {

private:
    const Automaton* automaton; // Automaton instance.
    const Location* location;

public:
    LocationValueExpression(const Automaton& automaton, const Location* location);
    ~LocationValueExpression() override;
    DELETE_CONSTRUCTOR(LocationValueExpression)

    /* setter */

    /* getter */

    [[nodiscard]] inline const Automaton* get_automaton() const { return automaton; }

    [[nodiscard]] inline const Location* get_location() const { return location; }

    [[nodiscard]] const std::string& get_automaton_name() const;

    [[nodiscard]] const std::string& get_location_name() const;

    [[nodiscard]] AutomatonIndex_type get_location_index() const;

    [[nodiscard]] PLAJA::integer get_location_value() const;

    /* override */

    PLAJA::integer evaluateInteger(const StateBase* state) const override;

    bool equals(const Expression* exp) const override;

    [[nodiscard]] std::size_t hash() const override;

    void accept(AstVisitor* ast_visitor) override;

    void accept(AstVisitorConst* ast_visitor) const override;

    [[nodiscard]] std::unique_ptr<Expression> deepCopy_Exp() const override;

    [[nodiscard]] std::unique_ptr<LocationValueExpression> deep_copy() const;

};

#endif //PLAJA_LOCATION_VALUE_EXPRESSION_H
