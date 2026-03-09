//
// This file is part of the PlaJA code base.
// Copyright (c) (2019 - 2019) Marcel Vinzent.
// See README.md in the top-level directory for licensing information.
//

#include "assignment.h"
#include "../../search/states/state_base.h"
#include "../../utils/floating_utils.h"
#include "../../utils/rng.h"
#include "../../utils/utils.h"
#include "../../search/information/model_information.h"
#include "../../globals.h"
#include "../visitor/ast_visitor.h"
#include "../visitor/ast_visitor_const.h"
#include "expression/expression_utils.h"
#include "expression/variable_expression.h"
#include "model.h"

#ifdef RUNTIME_CHECKS

#include "../../exception/runtime_exception.h"

#endif

Assignment::Assignment():
    ref()
    , value(nullptr)
    , lowerBound(nullptr)
    , upperBound(nullptr)
    , index(0) {}

Assignment::~Assignment() = default;

/* setter */

void Assignment::set_ref(std::unique_ptr<LValueExpression>&& ref_) { ref = std::move(ref_); }

void Assignment::set_value(std::unique_ptr<Expression>&& val) {
    JANI_ASSERT(not lowerBound and not upperBound)
    value = std::move(val);
}

void Assignment::set_lower_bound(std::unique_ptr<Expression>&& lb) {
    JANI_ASSERT(not value)
    lowerBound = std::move(lb);
}

void Assignment::set_upper_bound(std::unique_ptr<Expression>&& ub) {
    JANI_ASSERT(not value)
    upperBound = std::move(ub);
}

/* */

std::unique_ptr<Assignment> Assignment::deep_copy() const {
    std::unique_ptr<Assignment> copy(new Assignment());

    copy->copy_comment(*this);

    if (ref) { copy->set_ref(ref->deep_copy_lval_exp()); }

    if (value) { copy->set_value(value->deepCopy_Exp()); }
    if (lowerBound) { copy->set_lower_bound(lowerBound->deepCopy_Exp()); }
    if (upperBound) { copy->set_upper_bound(upperBound->deepCopy_Exp()); }

    copy->set_index(index);

    return copy;
}

void Assignment::evaluate(const StateBase* current, StateBase* target) const {

    PLAJA_ASSERT(PLAJA_GLOBAL::currentModel) // Quick fix, we need lower and upper bounds when evaluating non-det. assignments.

    if (value) { ref->assign(*target, current, value.get()); }
    else {
        PLAJA_ASSERT(lowerBound and upperBound)

        const auto& model_info = PLAJA_GLOBAL::currentModel->get_model_information();

        STMT_IF_DEBUG(
            static bool non_det_warning(true);
            if (PLAJA_GLOBAL::randomizeNonDetEval and non_det_warning) {
                PLAJA_LOG_DEBUG(PLAJA_UTILS::to_red_string("Evaluating non-deterministic assignment."))
                non_det_warning = false;
            }
        )

        const auto state_index = ref->get_variable_index(current);

        if (PLAJA_GLOBAL::randomizeNonDetEval) {


            // Sample assignment value:

            if (ref->is_floating()) {

                const auto lb = std::max(lowerBound->evaluateFloating(current), model_info.get_lower_bound_float(state_index));
                const auto ub = std::min(upperBound->evaluateFloating(current), model_info.get_upper_bound_float(state_index));
                RUNTIME_ASSERT(PLAJA_FLOATS::lte(lb, ub, PLAJA::floatingPrecision), this->to_string())

                target->assign_float<true>(state_index, PLAJA_GLOBAL::rng->sample_float(lb, ub));

            } else {

                const auto lb = std::max(lowerBound->evaluateInteger(current), model_info.get_lower_bound_int(state_index));
                const auto ub = std::min(upperBound->evaluateInteger(current), model_info.get_upper_bound_int(state_index));
                RUNTIME_ASSERT(lb <= ub, this->to_string())

                target->assign_int<true>(state_index, PLAJA_GLOBAL::rng->sample_signed(lb, ub));

            }

        } else {

            if (ref->is_floating()) {

                const auto lb = std::max(lowerBound->evaluateFloating(current), model_info.get_lower_bound_float(state_index));
                target->assign_float<true>(state_index, lb);

                STMT_IF_RUNTIME_CHECKS(
                    const auto ub = std::min(upperBound->evaluateFloating(current), model_info.get_upper_bound_float(state_index));
                    RUNTIME_ASSERT(PLAJA_FLOATS::lte(lb, ub, PLAJA::floatingPrecision), this->to_string())
                )

            } else {

                const auto lb = std::max(lowerBound->evaluateInteger(current), model_info.get_lower_bound_int(state_index));
                target->assign_int<true>(state_index, lb);

                STMT_IF_RUNTIME_CHECKS(
                    const auto ub = std::min(upperBound->evaluateInteger(current), model_info.get_upper_bound_int(state_index));
                    RUNTIME_ASSERT(lb <= ub, this->to_string())
                )

            }

        }

    }

}

bool Assignment::agrees(const StateBase& source, const StateBase& target) const {

    PLAJA_ASSERT(source.is_valid())
    PLAJA_ASSERT(target.is_valid())

    if (value) {

        if (ref->is_floating()) {

            const bool rlt = PLAJA_FLOATS::equal(ref->access_floating(target, &source), value->evaluate_floating(source), PLAJA::floatingPrecision);

            PLAJA_LOG_DEBUG_IF(PLAJA_GLOBAL::do_additional_outputs() and not rlt, PLAJA_UTILS::string_f("Target (%.15f) disagrees with update assignment (%.15f).", ref->access_floating(target, &source), value->evaluate_floating(source)))

            STMT_IF_DEBUG(if (PLAJA_GLOBAL::do_additional_outputs() and not rlt) {
                source.dump(true);
                dump();
                target.dump(true);
            })

            return rlt;

        } else { return ref->access_integer(target, &source) == value->evaluate_integer(source); }

    } else {
        PLAJA_ASSERT(lowerBound and upperBound)

        if (ref->is_floating()) {

            auto lb = lowerBound->evaluate_floating(source);
            auto ub = upperBound->evaluate_floating(source);
            RUNTIME_ASSERT(PLAJA_FLOATS::lte(lb, ub, PLAJA::floatingPrecision), this->to_string())

            auto target_value = ref->access_floating(target, &source);

            STMT_IF_DEBUG(
                if (PLAJA_GLOBAL::do_additional_outputs() and not PLAJA_FLOATS::lte(lb, target_value, PLAJA::floatingPrecision)) {
                    PLAJA_LOG_DEBUG(PLAJA_UTILS::string_f("Target (%.15f) disagrees with update lower bound (%.15f)", target_value, lb))
                    source.dump(true);
                    dump();
                    target.dump(true);
                }
            )

            STMT_IF_DEBUG(
                if (PLAJA_GLOBAL::do_additional_outputs() and not PLAJA_FLOATS::lte(target_value, ub, PLAJA::floatingPrecision)) {
                    PLAJA_LOG_DEBUG(PLAJA_UTILS::string_f("Target (%.15f) disagrees with update upper bound (%.15f)", target_value, ub))
                    source.dump(true);
                    dump();
                    target.dump(true);
                }
            )

            return PLAJA_FLOATS::lte(lb, target_value, PLAJA::floatingPrecision) and PLAJA_FLOATS::lte(target_value, ub, PLAJA::floatingPrecision);

        } else {

            auto lb = lowerBound->evaluate_integer(source);
            auto ub = upperBound->evaluate_integer(source);
            RUNTIME_ASSERT(lb <= ub, this->to_string())

            auto target_value = ref->access_integer(target, &source);

            return lb <= target_value and target_value <= ub;

        }

    }

}

void Assignment::clip(const StateBase* source, StateBase& target) const {

    PLAJA_ASSERT(is_non_deterministic())
    PLAJA_ASSERT(target.is_valid())

    PLAJA_ASSERT(lowerBound and upperBound)

    const auto state_index = ref->get_variable_index(source);

    if (ref->is_floating()) {

        const auto lb = lowerBound->evaluateFloating(source);
        const auto ub = upperBound->evaluateFloating(source);
        RUNTIME_ASSERT(PLAJA_FLOATS::lte(lb, ub, PLAJA::floatingPrecision), this->to_string(true))

        const auto target_value = target.get_float(state_index);

        if (PLAJA_FLOATS::lt(target_value, lb, PLAJA::floatingPrecision)) { target.assign_float<false>(state_index, lb); }
        else if (PLAJA_FLOATS::lt(ub, target_value, PLAJA::floatingPrecision)) { target.assign_float<false>(state_index, ub); }

    } else {

        const auto lb = lowerBound->evaluateInteger(source);
        const auto ub = upperBound->evaluateInteger(source);
        RUNTIME_ASSERT(lb <= ub, this->to_string(true))

        const auto target_value = target.get_int(state_index);

        if (target_value < lb) { target.assign_int<false>(state_index, lb); }
        else if (ub < target_value) { target.assign_int<false>(state_index, ub); }

    }

}

/* lvalue expression shortcuts */

VariableID_type Assignment::get_updated_var_id() const { return ref->get_variable_id(); }

const Expression* Assignment::get_array_index() const { return ref->get_array_index(); }

VariableIndex_type Assignment::get_variable_index(const StateBase* source) const { return ref->get_variable_index(source); }

/* override */

void Assignment::accept(AstVisitor* ast_visitor) { ast_visitor->visit(this); }

void Assignment::accept(AstVisitorConst* ast_visitor) const { ast_visitor->visit(this); }




