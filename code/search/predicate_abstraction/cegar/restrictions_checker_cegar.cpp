//
// This file is part of the PlaJA code base.
// Copyright (c) (2019 - 2020) Marcel Vinzent.
// See README.md in the top-level directory for licensing information.
//

#include <iostream>
#include "restrictions_checker_cegar.h"
#include "../../../exception/not_supported_exception.h"
#include "../../../parser/ast/expression/special_cases/linear_expression.h"
#include "../../../parser/ast/expression/variable_expression.h"
#include "../../../parser/ast/assignment.h"
#include "../../../parser/ast/edge.h"
#include "../../../parser/visitor/extern/to_linear_expression.h"
#include "../../../parser/visitor/extern/to_normalform.h"
#include "../../../parser/visitor/extern/variables_of.h"
#include "../../../parser/visitor/linear_constraints_checker.h"
#include "../../../utils/utils.h"

#ifndef NDEBUG

#include "../../../parser/ast/expression/constant_value_expression.h"
#include "../../../utils/floating_utils.h"

#endif

RestrictionsCheckerCegar::RestrictionsCheckerCegar() = default;

RestrictionsCheckerCegar::~RestrictionsCheckerCegar() = default;

bool RestrictionsCheckerCegar::check_restrictions(const AstElement* ast_element, bool catch_exception) {
    RestrictionsCheckerCegar restrictions_checker;

    restrictions_checker.nonDetUpdatedVars = VARIABLES_OF::non_det_updated_vars_of(ast_element);

    if (catch_exception) { // branch outside of try-catch to keep copy constructor of exception deleted
        try { ast_element->accept(&restrictions_checker); }
        catch (NotSupportedException& e) {
            PLAJA_LOG(e.what());
            return false;
        }
    } else { ast_element->accept(&restrictions_checker); }

    return true;
}

bool RestrictionsCheckerCegar::check_restrictions(const Model* model, bool catch_exception) { return check_restrictions(cast_model(model), catch_exception); }

void RestrictionsCheckerCegar::visit(const Assignment* assignment) {

    RestrictionsSMTChecker::visit(assignment);

    std::list<const Expression*> linear_sums;

    if (assignment->get_value()) { linear_sums.push_back(assignment->get_value()); }
    else {
        JANI_ASSERT(assignment->get_lower_bound() and assignment->get_upper_bound())
        linear_sums.push_back(assignment->get_lower_bound());
        linear_sums.push_back(assignment->get_upper_bound());
    }

    for (const auto* lin_sum: linear_sums) {

        if (not VARIABLES_OF::contains(lin_sum, this->nonDetUpdatedVars, false)) { continue; }

        const LinearExpression* lin_sum_ptr; // NOLINT(*-init-variables)
        std::unique_ptr<LinearExpression> lin_sum_convert(nullptr);

        if (TO_LINEAR_EXP::is_linear_sum(*lin_sum)) { lin_sum_ptr = static_cast<const LinearExpression*>(lin_sum); } // NOLINT(*-pro-type-static-cast-downcast)
        else if (TO_LINEAR_EXP::as_linear_exp(*lin_sum)) {
            lin_sum_convert = TO_LINEAR_EXP::to_linear_sum(*lin_sum);
            lin_sum_ptr = lin_sum_convert.get();
        } else { throw NotSupportedException(assignment->to_string()); }

        if (not lin_sum_ptr->is_linear_sum() or lin_sum_ptr->get_number_addends() > 1) { throw NotSupportedException(assignment->to_string()); }

    }

}

void RestrictionsCheckerCegar::visit(const Edge* edge) {
    AstVisitorConst::visit(edge);

    const auto* guard = edge->get_guardExpression();

    if (not guard or not VARIABLES_OF::contains(guard, this->nonDetUpdatedVars, false)) { return; }

    // guard
    auto guard_standardized = TO_NORMALFORM::normalize_and_split(*guard, true);

    for (const auto& guard_atom: guard_standardized) {

        PLAJA_ASSERT(TO_LINEAR_EXP::as_linear_exp(*guard_atom))

        auto linear_atom = TO_LINEAR_EXP::to_linear_constraint(*guard_atom);

        if (not linear_atom->is_linear_constraint() or linear_atom->get_number_addends() > 1) { throw NotSupportedException(guard->to_string()); }

        PLAJA_ASSERT(not linear_atom->addendIterator().var()->is_integer() or
                     (PLAJA_FLOATS::equals_integer(linear_atom->addendIterator().factor()->evaluate_floating_const(), 1, PLAJA::integerPrecision)
                      and linear_atom->get_scalar()->is_integer())
        )

    }

}
