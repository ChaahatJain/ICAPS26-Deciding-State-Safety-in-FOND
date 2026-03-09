//
// This file is part of the PlaJA code base.
// Copyright (c) (2019 - 2020) Marcel Vinzent.
// See README.md in the top-level directory for licensing information.
//

#include "linear_constraints_checker.h"
#include "../../utils/floating_utils.h"
#include "../../utils/utils.h"
#include "../ast/expression/non_standard/let_expression.h"
#include "../ast/expression/non_standard/state_condition_expression.h"
#include "../ast/expression/special_cases/nary_expression.h"
#include "../ast/expression/special_cases/linear_expression.h"
#include "../ast/expression/array_access_expression.h"
#include "../ast/expression/unary_op_expression.h"
#include "../ast/type/declaration_type.h"

#define LCC_CHECKER_NONE(STR) // PLAJA_LOG_DEBUG(STR) // for debugging

namespace LINEAR_CONSTRAINTS_CHECKER {

    [[nodiscard]] bool is_internally_possible(BinaryOpExpression::BinaryOp binary_op, LinearConstraintsChecker::LinearType requested_type) { // NOLINT(*-no-recursion)

        switch (requested_type) {

            case LinearConstraintsChecker::NONE: { PLAJA_ABORT; }

            case LinearConstraintsChecker::ADDEND: {
                static const std::unordered_set<BinaryOpExpression::BinaryOp> possible_ops { BinaryOpExpression::PLUS, BinaryOpExpression::MINUS, BinaryOpExpression::TIMES };
                return possible_ops.count(binary_op);
            }

            case LinearConstraintsChecker::CON_BOOL: { return binary_op == BinaryOpExpression::AND; }

            case LinearConstraintsChecker::DIS_BOOL: { return binary_op == BinaryOpExpression::OR; }

            case LinearConstraintsChecker::CONSTRAINT: {
                static const std::unordered_set<BinaryOpExpression::BinaryOp> possible_ops { BinaryOpExpression::EQ, BinaryOpExpression::NE, BinaryOpExpression::LT, BinaryOpExpression::LE, BinaryOpExpression::GT, BinaryOpExpression::GE };
                return possible_ops.count(binary_op) or is_internally_possible(binary_op, LinearConstraintsChecker::ADDEND);
            }

            case LinearConstraintsChecker::SUM: { return is_internally_possible(binary_op, LinearConstraintsChecker::ADDEND); }

            case LinearConstraintsChecker::ASSIGNMENT: { return is_internally_possible(binary_op, LinearConstraintsChecker::SUM); }

            case LinearConstraintsChecker::CON:
            case LinearConstraintsChecker::DIS: {
                return binary_op == BinaryOpExpression::AND or binary_op == BinaryOpExpression::OR or is_internally_possible(binary_op, LinearConstraintsChecker::CONSTRAINT);
            }

            case LinearConstraintsChecker::CONDITION: { return true; }

            default: { return false; }
        }

        PLAJA_ABORT

    }

}

/**********************************************************************************************************************/

LinearConstraintsChecker::LinearConstraintsChecker(LinearType requested_type, const LINEAR_CONSTRAINTS_CHECKER::Specification& specification):
    type(NONE)
    , requestedType(requested_type)
    , specification(&specification)
    , isNf(true)
    , seenConsts(0) {}

LinearConstraintsChecker::~LinearConstraintsChecker() = default;

/**********************************************************************************************************************/

bool LinearConstraintsChecker::is(LinearConstraintsChecker::LinearType expr_type, const Expression& expr, LinearConstraintsChecker::LinearType super_type) const { // NOLINT(misc-no-recursion)
    switch (super_type) {
        case NONE: { PLAJA_ABORT; }
        case ADDEND: { return expr_type == ADDEND or (expr_type == VAR and not expr.get_type()->is_boolean_type()); }
        case LITERAL: { return expr_type == LITERAL or expr_type == TRUTH_VALUE or (expr_type == VAR and expr.get_type()->is_boolean_type()); }
        case CON_BOOL: { return expr_type == CON_BOOL or is(expr_type, expr, LITERAL); }
        case DIS_BOOL: { return expr_type == DIS_BOOL or is(expr_type, expr, LITERAL); }
        case CONSTRAINT: { return expr_type == CONSTRAINT; }
        case SUM: { return expr_type == SUM or is(expr_type, expr, ADDEND) or expr_type == SCALAR; }
        case ASSIGNMENT: { return expr_type == ASSIGNMENT or is(expr_type, expr, LITERAL) or is(expr_type, expr, SUM); }
        case CON: { return expr_type == CON or expr_type == DIS or is(expr_type, expr, CONSTRAINT) or is(expr_type, expr, CON_BOOL) or is(expr_type, expr, DIS_BOOL); }
        case DIS: { return expr_type == DIS or expr_type == CON or is(expr_type, expr, CONSTRAINT) or is(expr_type, expr, CON_BOOL) or is(expr_type, expr, DIS_BOOL); }
        case CONDITION: { return expr_type != NONE and not is(expr_type, expr, SUM); }
        default: { return expr_type == super_type; }
    }
    PLAJA_ABORT
}

bool LinearConstraintsChecker::is_internally_possible(LinearConstraintsChecker::LinearType expr_type, LinearConstraintsChecker::LinearType requested_type) { // NOLINT(*-no-recursion)
    switch (requested_type) {

        case NONE: { PLAJA_ABORT; }

        case ADDEND: {
            static const std::unordered_set<LinearType> possible_types { ADDEND, SCALAR, VAR };
            return possible_types.count(expr_type);
        }

        case LITERAL: {
            static const std::unordered_set<LinearType> possible_types { LITERAL, TRUTH_VALUE, VAR };
            return possible_types.count(expr_type);
        }

        case CON_BOOL: { return expr_type == CON_BOOL or is_internally_possible(expr_type, LITERAL); }

        case DIS_BOOL: { return expr_type == DIS_BOOL or is_internally_possible(expr_type, LITERAL); }

        case CONSTRAINT: { return expr_type == CONSTRAINT or is_internally_possible(expr_type, SUM); }

        case SUM: { return expr_type == SUM or is_internally_possible(expr_type, ADDEND); }

        case ASSIGNMENT: { return expr_type == ASSIGNMENT or is_internally_possible(expr_type, LITERAL) or is_internally_possible(expr_type, SUM); }

        case CON:
        case DIS: {
            static const std::unordered_set<LinearType> possible_types { CON, DIS, CON_BOOL, DIS_BOOL, CONSTRAINT, SUM };
            return possible_types.count(expr_type) or is_internally_possible(expr_type, ADDEND) or is_internally_possible(expr_type, LITERAL);
        }

        case CONDITION: { return true; }

        default: { return expr_type == requested_type; }
    }

    PLAJA_ABORT
}

/**********************************************************************************************************************/

void LinearConstraintsChecker::visit(const ArrayAccessExpression* exp) {

    if (not specification->allowArrayAccess
        or (not specification->allowBools and exp->is_boolean())) {
        LCC_CHECKER_NONE(__PRETTY_FUNCTION__)
        type = NONE;
        return;
    }

    const auto* index = exp->get_index();

    if (not index->is_constant()) {
        type = NONE;
        return;
    }

    add_to_seen_vars(exp->get_variable_index());
    type = VAR;
}

void LinearConstraintsChecker::visit(const ArrayConstructorExpression*) {
    LCC_CHECKER_NONE(__PRETTY_FUNCTION__)
    type = NONE;
}

void LinearConstraintsChecker::visit(const ArrayValueExpression*) {
    LCC_CHECKER_NONE(__PRETTY_FUNCTION__)
    type = NONE;
}

void LinearConstraintsChecker::visit(const BoolValueExpression* /*exp*/) {
    if (not specification->allowBools) {
        LCC_CHECKER_NONE(__PRETTY_FUNCTION__)
        type = NONE;
    } else { type = TRUTH_VALUE; }
    inc_seen_constants();
}

void LinearConstraintsChecker::visit(const DistributionSamplingExpression*) {
    LCC_CHECKER_NONE(__PRETTY_FUNCTION__)
    type = NONE;
}

void LinearConstraintsChecker::visit(const BinaryOpExpression* exp) {

    /* Fast check. */
    if (not LINEAR_CONSTRAINTS_CHECKER::is_internally_possible(exp->get_op(), requestedType)) {
        type = NONE;
        return;
    }

    type = NONE;
    exp->get_left()->accept(this);
    if (type == NONE or not is_internally_possible(type, requestedType)) { return; }
    const auto type_l = type;
    type = NONE;
    exp->get_right()->accept(this);
    if (type == NONE or not is_internally_possible(type, requestedType)) { return; }
    const auto type_r = type;

    const auto op = exp->get_op();

    switch (op) {

        case BinaryOpExpression::EQ:
        case BinaryOpExpression::NE:
        case BinaryOpExpression::LT:
        case BinaryOpExpression::LE:
        case BinaryOpExpression::GT:
        case BinaryOpExpression::GE: {
            // normal linear constraint of the form o {<,<=,=,>=,>}
            if (specification->specialRequest == LINEAR_CONSTRAINTS_CHECKER::SpecialAstRequest::TRUE) {
                LCC_CHECKER_NONE("CONSTRAINT - NOT SPECIAL")
                type = NONE;
                return;
            }

            // op check
            if (not(op == BinaryOpExpression::LE or op == BinaryOpExpression::GE
                    or (specification->allowEquality and (op == BinaryOpExpression::EQ or op == BinaryOpExpression::NE) and not specification->asPredicate) // allow equality (but default no for predicates)
                    or (specification->allowStrictOp and (op == BinaryOpExpression::LT or op == BinaryOpExpression::GT)))) { // allow strict op
                LCC_CHECKER_NONE("STRICT OP")
                type = NONE;
                return;
            }

            // main check
            if (not(is(type_l, *exp->get_left(), SUM) and is(type_r, *exp->get_right(), SUM) and has_addend())) {
                LCC_CHECKER_NONE("SUM - MAIN")
                type = NONE;
                return;
            }

            type = CONSTRAINT;

            if (op == BinaryOpExpression::LT or op == BinaryOpExpression::GT) { isNf = false; }; // strict op is not standard however

            if (not is(type_r, *exp->get_right(), SCALAR)) { isNf = false; } // standard form has right-hand scalar

            if (specification->asPredicate and op != BinaryOpExpression::GE) { isNf = false; } // normal form predicate is of form sum >= scalar

            return;
        }

        case BinaryOpExpression::MINUS:
        case BinaryOpExpression::PLUS: {

            if (specification->specialRequest == LINEAR_CONSTRAINTS_CHECKER::SpecialAstRequest::TRUE) {
                LCC_CHECKER_NONE("SUM - NOT SPECIAL")
                type = NONE;
                return;
            }

            if (is(type_l, *exp->get_left(), SUM) and is(type_r, *exp->get_right(), SUM)) { type = SUM; }
            else {
                LCC_CHECKER_NONE("SUM")
                type = NONE;
            }

            return;
        }

        case BinaryOpExpression::TIMES: {

            if (specification->specialRequest == LINEAR_CONSTRAINTS_CHECKER::SpecialAstRequest::TRUE) {
                LCC_CHECKER_NONE("MULTIPLICATION - NOT SPECIAL")
                type = NONE;
                return;
            }

            PLAJA_ASSERT(exp->get_left()->get_type()->is_numeric_type())
            PLAJA_ASSERT(exp->get_right()->get_type()->is_numeric_type())

            // ADDEND
            if ((is(type_l, *exp->get_left(), VAR) and is_factor(*exp->get_right()))
                or (is(type_r, *exp->get_right(), VAR) and is_factor(*exp->get_left()))) { type = ADDEND; }
                // SUM
            else if ((is(type_l, *exp->get_left(), SUM) and is_factor(*exp->get_right()))
                     or (is(type_r, *exp->get_right(), SUM) and is_factor(*exp->get_left()))) {
                isNf = false;
                type = SUM;
            } else {
                LCC_CHECKER_NONE("MULTIPLICATION")
                type = NONE;
            }

            return;
        }

        case BinaryOpExpression::OR: {
            if (is(type_l, *exp->get_left(), DIS_BOOL) and is(type_r, *exp->get_right(), DIS_BOOL)) { type = DIS_BOOL; }
            else if (is(type_l, *exp->get_left(), DIS) and is(type_r, *exp->get_right(), DIS)) {
                // additional check for linear disjunction (since may use same vars in both branches)
                if (specification->nfRequest != LINEAR_CONSTRAINTS_CHECKER::NFRequest::NONE) {
                    auto spec_intern = *specification;
                    spec_intern.nfRequest = LINEAR_CONSTRAINTS_CHECKER::NFRequest::TRUE;
                    seenConsts = 0;
                    seenAddends.clear();
                    isNf = isNf and is_linear(*exp->get_left(), spec_intern) and is_linear(*exp->get_right(), spec_intern);
                }
                type = DIS;
            } else {
                LCC_CHECKER_NONE("OR")
                type = NONE;
            }
            return;
        }

        case BinaryOpExpression::AND: {

            if (is(type_l, *exp->get_left(), CON_BOOL) and is(type_r, *exp->get_right(), CON_BOOL)) { type = CON_BOOL; }
            else if (is(type_l, *exp->get_left(), CON) and is(type_r, *exp->get_right(), CON)) {
                // additional check for linear conjunction (since may use same vars in both branches)
                if (specification->nfRequest != LINEAR_CONSTRAINTS_CHECKER::NFRequest::NONE) {
                    auto spec_intern = *specification;
                    spec_intern.nfRequest = LINEAR_CONSTRAINTS_CHECKER::NFRequest::TRUE;
                    seenConsts = 0;
                    seenAddends.clear();
                    isNf = isNf and is_linear(*exp->get_left(), spec_intern) and is_linear(*exp->get_right(), spec_intern);
                }
                type = CON;
            } else {
                LCC_CHECKER_NONE("AND")
                type = NONE;
            }

            return;
        }

        default: {
            LCC_CHECKER_NONE("BINARY - DEFAULT")
            type = NONE;
            return;
        }
    }

    PLAJA_ABORT
}

void LinearConstraintsChecker::visit(const IntegerValueExpression* /*exp*/) {
    inc_seen_constants();
    type = SCALAR;
}

void LinearConstraintsChecker::visit(const ITE_Expression*) {
    LCC_CHECKER_NONE(__PRETTY_FUNCTION__)
    type = NONE;
}

void LinearConstraintsChecker::visit(const RealValueExpression* /*exp*/) {
    inc_seen_constants();
    type = SCALAR;
}

void LinearConstraintsChecker::visit(const UnaryOpExpression* exp) {
    switch (exp->get_op()) {
        case UnaryOpExpression::NOT: {
            PLAJA_ASSERT(exp->get_type()->is_boolean_type())
            type = NONE;
            exp->get_operand()->accept(this);
            if (type != TRUTH_VALUE and type != VAR) {  // LITERAL: TRUE OR FALSE or variable
                LCC_CHECKER_NONE("UNARY")
                type = NONE;
            } else { type = LITERAL; }
            return;
        }
        default: {
            LCC_CHECKER_NONE("UNARY - DEFAULT")
            type = NONE;
            return;
        }
    }
    PLAJA_ABORT
}

void LinearConstraintsChecker::visit(const VariableExpression* exp) {

    if (not specification->allowBools and exp->is_boolean()) {
        LCC_CHECKER_NONE(__PRETTY_FUNCTION__)
        type = NONE;
        return;
    }

    add_to_seen_vars(exp->get_index());
    type = VAR;

}

// non-standard

void LinearConstraintsChecker::visit(const ConstantArrayAccessExpression*) {
    LCC_CHECKER_NONE(__PRETTY_FUNCTION__)
    type = NONE; // If index is constant, we can as well evaluate expression upfront.
}

void LinearConstraintsChecker::visit(const LocationValueExpression*) {
    LCC_CHECKER_NONE(__PRETTY_FUNCTION__)
    type = NONE;
}

void LinearConstraintsChecker::visit(const LetExpression* exp) {

    if (exp->get_number_of_free_variables() != 0) { // allow for let expression to occur due to legacy considerations, but no free variable support
        LCC_CHECKER_NONE("LET - FREE VARS")
        type = NONE;
        return;
    }

    type = NONE;
    AST_CONST_ACCEPT_IF(exp->get_expression(), Expression)
}

void LinearConstraintsChecker::visit(const PredicatesExpression*) {
    LCC_CHECKER_NONE(__PRETTY_FUNCTION__)
    type = NONE;
}

void LinearConstraintsChecker::visit(const StateConditionExpression* exp) {
    const auto* constraint = exp->get_constraint();

    if (constraint) {
        type = NONE;

        constraint->accept(this);

        if (not is(type, *constraint, CONDITION)) {

            LCC_CHECKER_NONE(__PRETTY_FUNCTION__)
            type = NONE;
            return;

        }

        if (exp->get_size_loc_values() == 0) { return; } // no locs, just keep type of state var constraint

    }

    type = CON;
}

void LinearConstraintsChecker::visit(const StateValuesExpression*) {
    LCC_CHECKER_NONE(__PRETTY_FUNCTION__)
    type = NONE;
}

void LinearConstraintsChecker::visit(const StatesValuesExpression*) {
    LCC_CHECKER_NONE(__PRETTY_FUNCTION__)
    type = NONE;
}

void LinearConstraintsChecker::visit(const VariableValueExpression*) {
    LCC_CHECKER_NONE(__PRETTY_FUNCTION__)
    type = NONE;
}

// special:

void LinearConstraintsChecker::visit(const LinearExpression* exp) {

    if (specification->specialRequest == LINEAR_CONSTRAINTS_CHECKER::SpecialAstRequest::FALSE) {
        LCC_CHECKER_NONE("SPECIAL")
        type = NONE;
        return;
    }

    if (exp->is_linear_sum()) { type = (specification->needsAddends and exp->get_number_addends() == 0) ? NONE : SUM; }
    else {
        PLAJA_ASSERT(exp->is_linear_constraint())
        PLAJA_ASSERT(exp->get_number_addends() > 0)

        const auto op = exp->get_op();

        // check op
        if (not(op == BinaryOpExpression::LE or op == BinaryOpExpression::GE
                or (specification->allowEquality and (op == BinaryOpExpression::EQ or op == BinaryOpExpression::NE) and not specification->asPredicate) // allow equality (but default no for predicates)
                or (specification->allowStrictOp and (op == BinaryOpExpression::LT or op == BinaryOpExpression::GT)))) { // allow strict op
            LCC_CHECKER_NONE("SPECIAL - STRICT OP")
            type = NONE;
            return;
        }

        type = CONSTRAINT;

        if (op == BinaryOpExpression::LT or op == BinaryOpExpression::GT) { isNf = false; } // strict op is not standard however

        if (specification->asPredicate and op != BinaryOpExpression::GE) { isNf = false; }

    }
}

void LinearConstraintsChecker::visit(const NaryExpression* exp) {

    if (specification->specialRequest == LINEAR_CONSTRAINTS_CHECKER::SpecialAstRequest::FALSE) {
        LCC_CHECKER_NONE("JUNCTION - SPECIAL")
        type = NONE;
        return;
    }

    switch (exp->get_op()) {

        case BinaryOpExpression::OR: {

            bool dis_bool = true;

            for (auto it = exp->iterator(); !it.end(); ++it) {

                type = NONE;

                it->accept(this);

                if (not is(type, *it, DIS_BOOL)) {
                    dis_bool = false;
                }

                if (not is(type, *it, DIS)) {
                    LCC_CHECKER_NONE("JUNCTION - OR")
                    type = NONE;
                    return;
                }

            }

            type = dis_bool ? DIS_BOOL : DIS;

            // additional check for linear disjunction (since may use same vars in both branches)
            if (specification->nfRequest != LINEAR_CONSTRAINTS_CHECKER::NFRequest::NONE and not dis_bool) {

                auto spec_intern = *specification;
                spec_intern.nfRequest = LINEAR_CONSTRAINTS_CHECKER::NFRequest::TRUE;
                seenConsts = 0;
                seenAddends.clear();

                for (auto it = exp->iterator(); !it.end(); ++it) {
                    if (not is_linear(*it, spec_intern)) {
                        isNf = false;
                        break;
                    }
                }

            }

            return;
        }

        case BinaryOpExpression::AND: {

            bool con_bool = true;

            for (auto it = exp->iterator(); !it.end(); ++it) {

                type = NONE;

                it->accept(this);

                if (not is(type, *it, CON_BOOL)) {
                    con_bool = false;
                }

                if (not is(type, *it, CON)) {
                    LCC_CHECKER_NONE("JUNCTION - OR")
                    type = NONE;
                    return;
                }

            }

            type = con_bool ? CON_BOOL : CON;

            // additional check for linear conjunction (since may use same vars in both branches)
            if (specification->nfRequest != LINEAR_CONSTRAINTS_CHECKER::NFRequest::NONE and not con_bool) {

                auto spec_intern = *specification;
                spec_intern.nfRequest = LINEAR_CONSTRAINTS_CHECKER::NFRequest::TRUE;
                seenConsts = 0;
                seenAddends.clear();

                for (auto it = exp->iterator(); !it.end(); ++it) {
                    if (not is_linear(*it, spec_intern)) {
                        isNf = false;
                        break;
                    }
                }

            }

            return;

        }

        default: { PLAJA_ABORT }
    }

    PLAJA_ABORT
}

//

bool LinearConstraintsChecker::is_factor(const Expression& expr) { return is_scalar(expr) and not PLAJA_FLOATS::is_zero(expr.evaluate_floating_const()); }

bool LinearConstraintsChecker::is_linear(const Expression& expr, const LINEAR_CONSTRAINTS_CHECKER::Specification& specification, LinearType type) {
    PLAJA_ASSERT(type != NONE)
    PLAJA_ASSERT(type != SUM or not specification.asPredicate)
    LinearConstraintsChecker checker(type, specification);
    expr.accept(&checker);

    if (not checker.is(checker.type, expr, type)) { return false; }

    if (specification.asPredicate and type == CONDITION) { // special handling for preds
        if (not checker.is(checker.type, expr, CONSTRAINT) and not checker.is(checker.type, expr, CON_BOOL) and not checker.is(checker.type, expr, DIS_BOOL)) { return false; }
    }

    if (specification.needsAddends and checker.type == SUM and not checker.has_addend()) { return false; } // special handling for sum

    // normal form request: is the linear constraint requested to be in normal form (or even requested to be non-normal)?
    switch (specification.nfRequest) {
        case LINEAR_CONSTRAINTS_CHECKER::NFRequest::NONE: { return true; }
        case LINEAR_CONSTRAINTS_CHECKER::NFRequest::TRUE: { return checker.is_nf(); }
        case LINEAR_CONSTRAINTS_CHECKER::NFRequest::FALSE: { return not checker.is_nf(); }
    }

    PLAJA_ABORT
}

#if 0

// Deprecated:

bool LinearConstraintsChecker::is_state_variable_index_internal(const Expression& exp) {

    {   // basic variable
        const auto* var = PLAJA_UTILS::cast_ptr<VariableExpression>(&exp);
        if (var) {
            add_to_seen_vars(var->get_variable_index(true));
            return specification->allowBools or not var->is_boolean_type();
        }
    }

    if (specification->allowArrayAccess) {   // constant array access
        auto* array_access = PLAJA_UTILS::cast_ptr<ArrayAccessExpression>(&exp);
        if (array_access) {
            const auto* index = array_access->get_index();
            if (index->is_constant()) {
                add_to_seen_vars(array_access->get_variable_index(true));
                return specification->allowBools or not array_access->is_boolean_type();
            }
        }
    }

    return false;

}

bool LinearConstraintsChecker::is_literal(const Expression& expr, const LINEAR_CONSTRAINTS_CHECKER::Specification& specification) {
    if ((is_state_variable_index(expr, specification) or expr.is_constant()) and expr.get_type()->is_boolean_type()) { return true; }
    else {
        // polarity = false; // either polarity is false or the expression is invalid
        const auto* unary_exp = PLAJA_UTILS::cast_ptr<UnaryOpExpression>(&expr);
        return unary_exp and unary_exp->get_op() == UnaryOpExpression::NOT and is_state_variable_index(*unary_exp->get_operand(), specification);
    }
}

bool LinearConstraintsChecker::is_conjunction_bool(const Expression& expr, const LINEAR_CONSTRAINTS_CHECKER::Specification& specification) { // NOLINT(misc-no-recursion)
    if (is_literal(expr, specification)) { return true; }
    // else:
    const auto* binary_exp = PLAJA_UTILS::cast_ptr<BinaryOpExpression>(&expr);
    if (not binary_exp or binary_exp->get_op() != BinaryOpExpression::AND) { return false; }
    return is_conjunction_bool(*binary_exp->get_left(), specification) and is_conjunction_bool(*binary_exp->get_right(), specification);
}

bool LinearConstraintsChecker::is_disjunction_bool(const Expression& expr, const LINEAR_CONSTRAINTS_CHECKER::Specification& specification) { // NOLINT(misc-no-recursion)
    if (is_literal(expr, specification)) { return true; }
    // else:
    const auto* binary_exp = PLAJA_UTILS::cast_ptr<BinaryOpExpression>(&expr);
    if (not binary_exp or binary_exp->get_op() != BinaryOpExpression::OR) { return false; }
    return is_disjunction_bool(*binary_exp->get_left(), specification) and is_disjunction_bool(*binary_exp->get_right(), specification);
}

bool LinearConstraintsChecker::is_scalar(const Expression& expr) { return expr.is_constant() and ( expr.get_type()->is_integer_type() or expr.get_type()->is_floating_type() ); }

bool LinearConstraintsChecker::is_addend_internal(const Expression& expr) {
    if (is_state_variable_index_internal(expr)) { return true; }
    else {
        const auto* binary_exp = PLAJA_UTILS::cast_ptr<BinaryOpExpression>(&expr);
        if (not binary_exp or binary_exp->get_op() != BinaryOpExpression::TIMES) { return false; }
        if (is_state_variable_index_internal(*binary_exp->get_left())) { return is_factor(*binary_exp->get_right()); }
        if (is_state_variable_index_internal(*binary_exp->get_right())) { return is_factor(*binary_exp->get_left()); }
    }
    return false;
}

bool LinearConstraintsChecker::is_linear_sum_internal(const Expression& expr) { // NOLINT(misc-no-recursion)
    if (is_scalar(expr)) { inc_seen_scalars(); return true; }
    if (is_addend_internal(expr)) { hasAddend = true; return true; }
    // else:
    const auto* binary_exp = PLAJA_UTILS::cast_ptr<BinaryOpExpression>(&expr);
    if (not binary_exp) { return false; }

    switch (binary_exp->get_op()) {
        case BinaryOpExpression::PLUS: { return is_linear_sum_internal(*binary_exp->get_left()) && is_linear_sum_internal(*binary_exp->get_right()); }
        case BinaryOpExpression::TIMES: {
            isStandard = false; // as of the form c ( c1 * x1 + ... + cn * xn )
            return ( is_factor(*binary_exp->get_left()) && is_linear_sum_internal(*binary_exp->get_right()) ) or
                   ( is_linear_sum_internal(*binary_exp->get_left()) && is_factor(*binary_exp->get_right()) );
        }
        default: { return false; }
    }
}

bool LinearConstraintsChecker::is_linear_sum_special(const Expression& expr, LINEAR_CONSTRAINTS_CHECKER::Specification specification, bool needs_addend) {
    PLAJA_ASSERT(specification.specialRequest != LINEAR_CONSTRAINTS_CHECKER::SpecialAstRequest::NONE)
    const auto* lin_expr = PLAJA_UTILS::cast_ptr<LinearExpression>(&expr);
    return lin_expr and lin_expr->is_linear_sum() and (not needs_addend or lin_expr->get_number_addends() > 0);
}


bool LinearConstraintsChecker::is_linear_constraint(const Expression& expr, const LINEAR_CONSTRAINTS_CHECKER::Specification& specification) {
    if (specification.specialRequest == LINEAR_CONSTRAINTS_CHECKER::SpecialAstRequest::ONLY) { return is_linear_constraint_special(expr, specification); } // is special structure?
    else if (specification.specialRequest == LINEAR_CONSTRAINTS_CHECKER::SpecialAstRequest::BOTH) { if (is_linear_constraint_special(expr, specification)) { return true; } }
    else { PLAJA_ASSERT(specification.specialRequest == LINEAR_CONSTRAINTS_CHECKER::SpecialAstRequest::NONE); }

    auto* binary_exp = PLAJA_UTILS::cast_ptr<BinaryOpExpression>(&expr);
    if (not binary_exp) { return false; }
    // else:
    const BinaryOpExpression::BinaryOp op = binary_exp->get_op();
    // predicate request (deprecated)
    // if (!allow_equality && op == BinaryOpExpression::EQ) { throw NotSupportedException(exp->to_string(), binaryOpToString[BinaryOpExpression::EQ]); }
    // normal linear constraint of the form o {<,<=,=,>=,>}
    if (op == BinaryOpExpression::LE or op == BinaryOpExpression::GE
        or (specification.allowEquality and op == BinaryOpExpression::EQ) // allow equality
        or (specification.allowStrictOp and (op == BinaryOpExpression::LT or op == BinaryOpExpression::GT) ) ) { // allow strict op

        // main check
        LINEAR_CONSTRAINTS_CHECKER::Specification spec_internal(specification);
        spec_internal.allowBools = false;
        LinearConstraintsChecker linear_constraints_checker(spec_internal);
        const bool rlt = linear_constraints_checker.is_linear_sum_internal(*binary_exp->get_left()) &&
                linear_constraints_checker.is_linear_sum_internal(*binary_exp->get_right()) &&
                linear_constraints_checker.has_addend();

        if (op == BinaryOpExpression::LT or op == BinaryOpExpression::GT) { linear_constraints_checker.isStandard = false; } // strict op is not standard however
        if (not is_scalar(*binary_exp->get_right())) { linear_constraints_checker.isStandard = false; } // standard form has right-hand scalar
        // standard request: is the linear constraint requested to be standard (or even requested to be non-standard)?
        switch (specification.standardRequest) {
            case LINEAR_CONSTRAINTS_CHECKER::StandardRequest::NONE: { return rlt; }
            case LINEAR_CONSTRAINTS_CHECKER::StandardRequest::IS_STANDARD: { return rlt and linear_constraints_checker.is_standard(); }
            case LINEAR_CONSTRAINTS_CHECKER::StandardRequest::IS_NON_STANDARD: { return rlt and not linear_constraints_checker.is_standard(); }
            case LINEAR_CONSTRAINTS_CHECKER::StandardRequest::IS_STANDARD_PREDICATE: { return rlt and linear_constraints_checker.is_standard() and op == BinaryOpExpression::GE; } // standard predicate is of form sum >= scalar
            case LINEAR_CONSTRAINTS_CHECKER::StandardRequest::IS_NON_STANDARD_PREDICATE: { return rlt && not (linear_constraints_checker.is_standard() and op == BinaryOpExpression::GE); }
        }

    }

    return false;

}

bool LinearConstraintsChecker::is_linear_constraint_special(const Expression& expr, const LINEAR_CONSTRAINTS_CHECKER::Specification& specification) {
    PLAJA_ASSERT(specification.specialRequest != LINEAR_CONSTRAINTS_CHECKER::SpecialAstRequest::NONE)

    const auto* lin_expr = PLAJA_UTILS::cast_ptr<LinearExpression>(&expr);

    if (not lin_expr or not lin_expr->is_linear_constraint()) { return false; }

    const auto op = lin_expr->get_op();

    // normal linear constraint of the form o {<,<=,=,>=,>}
    if (op == BinaryOpExpression::LE or op == BinaryOpExpression::GE
        or (specification.allowEquality and op == BinaryOpExpression::EQ) // allow equality
        or (specification.allowStrictOp and (op == BinaryOpExpression::LT or op == BinaryOpExpression::GT) ) ) { // allow strict op

        const bool standard = not specification.allowStrictOp or not (op == BinaryOpExpression::LT or op == BinaryOpExpression::GT); // strict op is not standard however

        // standard request: is the linear constraint requested to be standard (or even requested to be non-standard)?
        switch (specification.standardRequest) {
            case LINEAR_CONSTRAINTS_CHECKER::StandardRequest::NONE: { return true; }
            case LINEAR_CONSTRAINTS_CHECKER::StandardRequest::IS_STANDARD: { return standard; }
            case LINEAR_CONSTRAINTS_CHECKER::StandardRequest::IS_NON_STANDARD: { return not standard; }
            case LINEAR_CONSTRAINTS_CHECKER::StandardRequest::IS_STANDARD_PREDICATE: { return standard and op == BinaryOpExpression::GE; } // standard predicate is of form sum >= scalar
            case LINEAR_CONSTRAINTS_CHECKER::StandardRequest::IS_NON_STANDARD_PREDICATE: { return not (standard and op == BinaryOpExpression::GE); }
        }

    }

    return false;

}

bool LinearConstraintsChecker::is_linear_conjunction(const Expression& expr, const LINEAR_CONSTRAINTS_CHECKER::Specification& specification) { // NOLINT(misc-no-recursion)
    if (is_linear_constraint(expr, specification) or is_conjunction_bool(expr, specification) or is_disjunction_bool(expr, specification)) { return true; }
    // else: must be conjunction
    auto* binary_exp = PLAJA_UTILS::cast_ptr<BinaryOpExpression>(&expr);
    if (not binary_exp) { return false; }
    // else:
    return binary_exp->get_op() == BinaryOpExpression::AND and
            is_linear_conjunction(*binary_exp->get_left(), specification) and is_linear_conjunction(*binary_exp->get_right(), specification);
}

bool LinearConstraintsChecker::is_linear_disjunction(const Expression& expr, const LINEAR_CONSTRAINTS_CHECKER::Specification& specification) { // NOLINT(misc-no-recursion)
    if (is_linear_constraint(expr, specification) or is_conjunction_bool(expr, specification) or is_disjunction_bool(expr, specification)) { return true; }
    // else: must be conjunction
    auto* binary_exp = PLAJA_UTILS::cast_ptr<BinaryOpExpression>(&expr);
    if (not binary_exp) { return false; }
    // else:
    return binary_exp->get_op() == BinaryOpExpression::OR and
           is_linear_disjunction(*binary_exp->get_left(), specification) and is_linear_disjunction(*binary_exp->get_right(), specification);
}

bool LinearConstraintsChecker::is_linear(const Expression& expr, const LINEAR_CONSTRAINTS_CHECKER::Specification& specification) {
    if (specification.asPredicate) { // as predicate only linear inequation supported
        auto spec_internal = specification;
        spec_internal.allowEquality = false;
        return is_linear_constraint(expr, spec_internal) or is_conjunction_bool(expr, specification) or is_disjunction_bool(expr, specification);
    } else {
        return is_linear_conjunction(expr, specification) or is_linear_disjunction(expr, specification);
    }
}

#endif