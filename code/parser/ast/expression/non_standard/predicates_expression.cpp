//
// This file is part of the PlaJA code base.
// Copyright (c) (2019 - 2022) Marcel Vinzent.
// See README.md in the top-level directory for licensing information.
//

#include "predicates_expression.h"
#include "../../../../parser/visitor/retrieve_variable_bound.h"
#include "../../../../search/states/state_values.h"
#include "../../../visitor/ast_visitor.h"
#include "../../../visitor/ast_visitor_const.h"
#include "../../type/bool_type.h"
#include "../binary_op_expression.h"
#include "../variable_expression.h"
#include "../unary_op_expression.h"

PredicatesExpression::PredicatesExpression() = default;

PredicatesExpression::~PredicatesExpression() = default;

/* construction */

void PredicatesExpression::reserve(std::size_t predicates_cap) { predicates.reserve(predicates_cap); }

void PredicatesExpression::add_predicate(std::unique_ptr<Expression>&& predicate) { predicates.push_back(std::move(predicate)); }

/* override */

bool PredicatesExpression::equals(const Expression* exp) const {
    auto* other = PLAJA_UTILS::cast_ptr_if<PredicatesExpression>(exp);
    if (not other) { return false; }

    const auto num_preds = predicates.size();
    if (num_preds != other->get_number_predicates()) { return false; }

    for (std::size_t i = 0; i < num_preds; ++i) {
        if (*get_predicate(i) != *other->get_predicate(i)) { return false; }
    }

    return true;
}

void PredicatesExpression::accept(AstVisitor* ast_visitor) { ast_visitor->visit(this); }

void PredicatesExpression::accept(AstVisitorConst* ast_visitor) const { ast_visitor->visit(this); }

std::unique_ptr<Expression> PredicatesExpression::deepCopy_Exp() const { return deepCopy(); }

std::unique_ptr<PredicatesExpression> PredicatesExpression::deepCopy() const {
    std::unique_ptr<PredicatesExpression> copy(new PredicatesExpression());
    copy->predicates.reserve(predicates.size());
    for (const auto& predicate: predicates) { copy->predicates.push_back(predicate->deepCopy_Exp()); }
    return copy;
}

// auxiliary methods for bound check

bool PredicatesExpression::predicate_is_bound(const Expression& predicate) {
    return RetrieveVariableBound::retrieve_bound(predicate).get();
}