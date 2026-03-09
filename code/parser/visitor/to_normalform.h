//
// This file is part of the PlaJA code base.
// Copyright (c) (2019 - 2022) Marcel Vinzent.
// See README.md in the top-level directory for licensing information.
//

#ifndef PLAJA_TO_NORMALFORM_H
#define PLAJA_TO_NORMALFORM_H

#include <list>
#include <memory>
#include "ast_visitor.h"

namespace TO_NORMALFORM {

    extern void negate(std::unique_ptr<Expression>& expr);

    extern void push_down_negation(std::unique_ptr<Expression>& expr);
    extern void negate_and_push_down(std::unique_ptr<Expression>& expr);

    extern void distribute_or_over_and(std::unique_ptr<Expression>& expr);
    extern void distribute_and_over_or(std::unique_ptr<Expression>& expr);

    /******************************************************************************************************************/

    extern void to_cnf(std::unique_ptr<Expression>& expr);
    extern void to_dnf(std::unique_ptr<Expression>& expr);

    extern void negation_to_cnf(std::unique_ptr<Expression>& expr);
    extern void negation_to_dnf(std::unique_ptr<Expression>& expr);

    /******************************************************************************************************************/

    extern void split_equality(std::unique_ptr<Expression>& expr);
    extern void split_equality_only(std::unique_ptr<Expression>& expr);
    extern void split_inequality_only(std::unique_ptr<Expression>& expr);

    /******************************************************************************************************************/

    FCT_IF_DEBUG([[nodiscard]] extern bool is_multi_ary(const Expression& expr);)
    extern void to_nary(std::unique_ptr<Expression>& expr);

    extern std::list<std::unique_ptr<Expression>> split_conjunction(std::unique_ptr<Expression>&& expr, bool nf);
    extern std::list<std::unique_ptr<Expression>> split_conjunction(const Expression& expr, bool nf);
    extern std::list<std::unique_ptr<Expression>> split_disjunction(std::unique_ptr<Expression>&& expr, bool nf);
    extern std::list<std::unique_ptr<Expression>> split_disjunction(const Expression& expr, bool nf);

    extern std::unique_ptr<Expression> construct_conjunction(std::list<std::unique_ptr<Expression>>&& sub_expressions);
    extern std::unique_ptr<Expression> construct_disjunction(std::list<std::unique_ptr<Expression>>&& sub_expressions);

    /******************************************************************************************************************/

    extern void normalize(std::unique_ptr<Expression>& expr);
    extern std::unique_ptr<Expression> normalize(std::unique_ptr<Expression>&& expr);
    extern std::unique_ptr<Expression> normalize(const Expression& expr);

    extern std::list<std::unique_ptr<Expression>> normalize_and_split(std::unique_ptr<Expression>&& expr, bool as_predicate);
    extern std::list<std::unique_ptr<Expression>> normalize_and_split(const Expression& expr, bool as_predicate);

    [[nodiscard]] extern bool is_states_values(const Expression& expr);
    extern std::unique_ptr<StateConditionExpression> split_on_locations(std::unique_ptr<Expression>&& expr);

    /******************************************************************************************************************/

    extern void specialize(std::unique_ptr<Expression>& expr);
    extern void standardize(std::unique_ptr<Expression>& expr);

}

#endif //PLAJA_TO_NORMALFORM_H
