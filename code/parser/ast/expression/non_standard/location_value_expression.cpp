//
// This file is part of the PlaJA code base.
// Copyright (c) (2019 - 2023) Marcel Vinzent.
// See README.md in the top-level directory for licensing information.
//

#include "location_value_expression.h"
#include "../../../../search/states/state_base.h"
#include "../../../visitor/ast_visitor.h"
#include "../../../visitor/ast_visitor_const.h"
#include "../../type/bool_type.h"
#include "../../automaton.h"
#include "../../location.h"

LocationValueExpression::LocationValueExpression(const Automaton& automaton, const Location* location):
    automaton(&automaton)
    , location(location) {
    resultType = std::make_unique<BoolType>();
}

LocationValueExpression::~LocationValueExpression() = default;

/* Setter. */

/* Getter. */

const std::string& LocationValueExpression::get_automaton_name() const { return automaton->get_name(); }

const std::string& LocationValueExpression::get_location_name() const { return location->get_name(); }

AutomatonIndex_type LocationValueExpression::get_location_index() const { return automaton->get_index(); }

PLAJA::integer LocationValueExpression::get_location_value() const { return location->get_locationValue(); }

/* Override. */

PLAJA::integer LocationValueExpression::evaluateInteger(const StateBase* state) const {
    PLAJA_ASSERT(state)
    return state->get_int(get_location_index()) == get_location_value();
}

bool LocationValueExpression::equals(const Expression* exp) const {
    auto* other = PLAJA_UTILS::cast_ptr_if<LocationValueExpression>(exp);
    if (not other) { return false; }
    // Sanity: Referenced parent structures should be same for all instances.
    PLAJA_ASSERT(other->automaton == automaton or other->get_location_index() != get_location_index())
    PLAJA_ASSERT(other->automaton != automaton or other->location == location or other->get_location_value() != get_location_value())
    //
    return other->automaton == automaton and other->location == location;
}

std::size_t LocationValueExpression::hash() const {
    constexpr unsigned prime = 31;
    std::size_t result = 1;
    result = prime * result + get_location_index();
    result = prime * result + get_location_value();
    return result;
}

void LocationValueExpression::accept(AstVisitor* ast_visitor) { ast_visitor->visit(this); }

void LocationValueExpression::accept(AstVisitorConst* ast_visitor) const { ast_visitor->visit(this); }

std::unique_ptr<Expression> LocationValueExpression::deepCopy_Exp() const { return deep_copy(); }

//

std::unique_ptr<LocationValueExpression> LocationValueExpression::deep_copy() const { return std::make_unique<LocationValueExpression>(*automaton, location); }