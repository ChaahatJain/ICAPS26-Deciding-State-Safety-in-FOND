//
// This file is part of the PlaJA code base.
// Copyright (c) (2019 - 2019) Marcel Vinzent.
// See README.md in the top-level directory for licensing information.
//

#include <unordered_map>
#include "state_predicate_expression.h"
#include "../../visitor/ast_visitor.h"
#include "../../visitor/ast_visitor_const.h"
#include "../type/bool_type.h"

StatePredicateExpression::StatePredicateExpression(StatePredicate sp_op):
    op(sp_op) {
    resultType = std::make_unique<BoolType>();
}

StatePredicateExpression::~StatePredicateExpression() = default;

// static:

namespace STATE_PREDICATE_EXPRESSION {
    const std::string statePredicateToStr[] { "initial", "deadlock", "timelock" }; // NOLINT(cert-err58-cpp)
    const std::unordered_map<std::string, StatePredicateExpression::StatePredicate> strToStatePredicate { { "initial",  StatePredicateExpression::INITIAL },
                                                                                                          { "deadlock", StatePredicateExpression::DEADLOCK },
                                                                                                          { "timelock", StatePredicateExpression::TIMELOCK } }; // NOLINT(cert-err58-cpp)
}

const std::string& StatePredicateExpression::op_to_str(StatePredicateExpression::StatePredicate op) { return STATE_PREDICATE_EXPRESSION::statePredicateToStr[op]; }

std::unique_ptr<StatePredicateExpression::StatePredicate> StatePredicateExpression::str_to_op(const std::string& op_str) {
    auto it = STATE_PREDICATE_EXPRESSION::strToStatePredicate.find(op_str);
    if (it == STATE_PREDICATE_EXPRESSION::strToStatePredicate.end()) { return nullptr; }
    else { return std::make_unique<StatePredicate>(it->second); }
}

// override:

const DeclarationType* StatePredicateExpression::determine_type() {
    PLAJA_ASSERT(resultType)
    return resultType.get();
}

void StatePredicateExpression::accept(AstVisitor* ast_visitor) { ast_visitor->visit(this); }

void StatePredicateExpression::accept(AstVisitorConst* ast_visitor) const { ast_visitor->visit(this); }

std::unique_ptr<PropertyExpression> StatePredicateExpression::deepCopy_PropExp() const {
    std::unique_ptr<StatePredicateExpression> copy(new StatePredicateExpression(op));
    if (resultType) { copy->resultType = resultType->deep_copy_decl_type(); }
    return copy;
}







