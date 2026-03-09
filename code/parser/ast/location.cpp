//
// This file is part of the PlaJA code base.
// Copyright (c) (2019 - 2019) Marcel Vinzent.
// See README.md in the top-level directory for licensing information.
//

#include "location.h"
#include "../visitor/ast_visitor.h"
#include "../visitor/ast_visitor_const.h"
#include "expression/expression.h"
#include "transient_value.h"

Location::Location():
    name(PLAJA_UTILS::emptyString)
    , locationValue(AUTOMATON::invalidLocation)
    , timeProgressCondition()
    CONSTRUCT_IF_COMMENT_PARSING(timeProgressComment(PLAJA_UTILS::emptyString))
    , transientValues() {
}

Location::~Location() = default;

/* construction */

void Location::reserve(std::size_t tval_cap) { transientValues.reserve(tval_cap); }

void Location::add_transient_value(std::unique_ptr<TransientValue>&& trans_val) { transientValues.push_back(std::move(trans_val)); }

/* setter */

void Location::set_timeProgressExpression(std::unique_ptr<Expression>&& exp) { timeProgressCondition = std::move(exp); }

[[maybe_unused]] void Location::set_transientValue(std::unique_ptr<TransientValue>&& trans_val, std::size_t index) { transientValues[index] = std::move(trans_val); }

//

std::unique_ptr<Location> Location::deepCopy() const {
    std::unique_ptr<Location> copy(new Location());
    copy->copy_comment(*this);

    copy->name = name;

    copy->locationValue = locationValue;

    if (timeProgressCondition) { copy->timeProgressCondition = timeProgressCondition->deepCopy_Exp(); }
    COPY_IF_COMMENT_PARSING(copy, timeProgressComment)

    copy->transientValues.reserve(transientValues.size());
    for (const auto& trans_val: transientValues) { copy->transientValues.emplace_back(trans_val->deepCopy()); }

    return copy;
}

/* override */

void Location::accept(AstVisitor* ast_visitor) { ast_visitor->visit(this); }

void Location::accept(AstVisitorConst* ast_visitor) const { ast_visitor->visit(this); }






