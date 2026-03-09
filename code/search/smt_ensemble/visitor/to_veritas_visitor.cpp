//
// This file is part of the PlaJA code base.
// Copyright (c) (2019 - 2023) Marcel Vinzent.
// See README.md in the top-level directory for licensing information.
//

#include "to_veritas_visitor.h"
#include "../../../exception/constructor_exception.h"
#include "../../../parser/ast/expression/non_standard/location_value_expression.h"
#include "../../../parser/ast/expression/non_standard/state_condition_expression.h"
#include "../../../parser/ast/expression/special_cases/nary_expression.h"
#include "../../../parser/ast/expression/special_cases/linear_expression.h"
#include "../../../parser/ast/expression/array_access_expression.h"
#include "../../../parser/ast/expression/bool_value_expression.h"
#include "../../../parser/ast/expression/integer_value_expression.h"
#include "../../../parser/ast/expression/real_value_expression.h"
#include "../../../parser/ast/expression/unary_op_expression.h"
#include "../../../parser/visitor/extern/to_linear_expression.h"
#include "../../../parser/visitor/linear_constraints_checker.h"
#include "../../../parser/visitor/to_normalform.h"
#include "../../../utils/utils.h"
#include "../constraints_in_veritas.h"
#include "../veritas_context.h"
#include "../vars_in_veritas.h"

#include "to_veritas_constraint.h" // TODO keep for now

namespace TO_VERITAS_VISITOR {

    template<class T, typename Target_t>
    void move_rlt_to_target(std::unique_ptr<T>& rlt, Target_t& target, void(T::*fx)(Target_t&)) {
        ((rlt.get())->*fx)(target);
        rlt = nullptr; // nullptr rlt is used as an indicator, e.g., during constructing of disjunctions.
    }

    extern VERITAS_IN_PLAJA::Equation::EquationType invert_op(VERITAS_IN_PLAJA::Equation::EquationType op);

    VERITAS_IN_PLAJA::Equation::EquationType invert_op(VERITAS_IN_PLAJA::Equation::EquationType op) {
        switch (op) {
            case VERITAS_IN_PLAJA::Equation::EquationType::EQ: return VERITAS_IN_PLAJA::Equation::EquationType::EQ;
            case VERITAS_IN_PLAJA::Equation::EquationType::LE: return VERITAS_IN_PLAJA::Equation::EquationType::GE;
            case VERITAS_IN_PLAJA::Equation::EquationType::GE: return VERITAS_IN_PLAJA::Equation::EquationType::LE;
            default: PLAJA_ABORT
        }
    }

    inline std::unique_ptr<VeritasConstraint> to_assignment_constraint(VERITAS_IN_PLAJA::Context& c, VeritasVarIndex_type target_var, VERITAS_IN_PLAJA::Equation::EquationType op, VeritasFloating_type scalar) { // NOLINT(bugprone-easily-swappable-parameters)
        PLAJA_ASSERT(target_var != VERITAS_IN_PLAJA::noneVar)
        return VERITAS_IN_PLAJA::BoundConstraint::construct(c, target_var, op, scalar);
    }

    std::unique_ptr<VeritasConstraint> to_copy_constraint(VERITAS_IN_PLAJA::Context& c, VeritasVarIndex_type target_var, VERITAS_IN_PLAJA::Equation::EquationType op, VeritasVarIndex_type src_var) {
        PLAJA_ASSERT(target_var != VERITAS_IN_PLAJA::noneVar)
        VERITAS_IN_PLAJA::Equation  eq(invert_op(op));
        eq.setScalar(0);
        eq.addAddend(-1, target_var);
        eq.addAddend(1, src_var);
        auto copy_constraint = std::make_unique<VERITAS_IN_PLAJA::EquationConstraint>(c, std::move(eq));
        return copy_constraint;
    }

}

std::unique_ptr<VeritasConstraint> ToVeritasVisitor::to_veritas_constraint(const Expression& expr, const StateIndexesInVeritas& state_indexes) {
    if (not LinearConstraintsChecker::is_linear(expr, LINEAR_CONSTRAINTS_CHECKER::Specification::set_special(LINEAR_CONSTRAINTS_CHECKER::SpecialAstRequest::NONE))) { throw ConstructorException(expr.to_string(), PLAJA_EXCEPTION::nonLinear); }
    ToVeritasVisitor to_veritas_visitor(state_indexes);
    expr.accept(&to_veritas_visitor);
    PLAJA_ASSERT(to_veritas_visitor.rlt)
    return std::move(to_veritas_visitor.rlt);
}

std::unique_ptr<VeritasConstraint> ToVeritasVisitor::to_assignment_constraint(VeritasVarIndex_type target_var, VERITAS_IN_PLAJA::Equation::EquationType assignment_type, const Expression& assignment_sum, const StateIndexesInVeritas& state_indexes) {
    PLAJA_ASSERT(target_var != VERITAS_IN_PLAJA::noneVar)

    if (not LinearConstraintsChecker::is_linear_assignment(assignment_sum, LINEAR_CONSTRAINTS_CHECKER::Specification::set_special(LINEAR_CONSTRAINTS_CHECKER::SpecialAstRequest::NONE))) { throw ConstructorException(assignment_sum.to_string(), PLAJA_EXCEPTION::nonLinear); }

    ToVeritasVisitor to_veritas_visitor(state_indexes);
    to_veritas_visitor.assignmentTarget = target_var;
    to_veritas_visitor.assignmentType = assignment_type;

    assignment_sum.accept(&to_veritas_visitor);
    PLAJA_ASSERT(to_veritas_visitor.rlt)
    return std::move(to_veritas_visitor.rlt);
}

/**********************************************************************************************************************/

ToVeritasVisitor::ToVeritasVisitor(const StateIndexesInVeritas& state_indexes_in_veritas):
    context(state_indexes_in_veritas._context())
    , stateIndexes(state_indexes_in_veritas)
    , assignmentTarget(VERITAS_IN_PLAJA::noneVar)
    , assignmentType(VERITAS_IN_PLAJA::Equation::EquationType::EQ) {
}

ToVeritasVisitor::~ToVeritasVisitor() = default;

/**********************************************************************************************************************/

void ToVeritasVisitor::visit(const ArrayAccessExpression* exp) {
    PLAJA_ASSERT(exp->get_index()->is_constant())

    if (assignmentTarget == VERITAS_IN_PLAJA::noneVar) {

        PLAJA_ASSERT(exp->is_boolean())
        rlt = std::make_unique<VERITAS_IN_PLAJA::BoundConstraint>(context, stateIndexes.to_veritas(exp->get_variable_index()), VERITAS_IN_PLAJA::Tightening::LB, 1);

    } else { // Assignment constraint.

        rlt = TO_VERITAS_VISITOR::to_copy_constraint(context, assignmentTarget, assignmentType, stateIndexes.to_veritas(exp->get_variable_index()));

    }
}

void ToVeritasVisitor::visit(const BoolValueExpression* exp) {
    PLAJA_ASSERT_EXPECT(assignmentType == VERITAS_IN_PLAJA::Equation::EquationType::EQ)
    rlt = TO_VERITAS_VISITOR::to_assignment_constraint(context, assignmentTarget, assignmentType, exp->evaluate_integer_const());
}

void ToVeritasVisitor::visit(const IntegerValueExpression* exp) {
    rlt = TO_VERITAS_VISITOR::to_assignment_constraint(context, assignmentTarget, assignmentType, exp->evaluate_integer_const());
}

void ToVeritasVisitor::visit(const RealValueExpression* exp) {
    rlt = TO_VERITAS_VISITOR::to_assignment_constraint(context, assignmentTarget, assignmentType, exp->evaluate_floating_const());
}

void ToVeritasVisitor::visit(const BinaryOpExpression* exp) {

    /* catch cases currently encoded via old structure:. */
    if (assignmentTarget == VERITAS_IN_PLAJA::noneVar) {

        if (LinearConstraintsChecker::is_conjunction_bool(*exp)) {

            rlt = TO_VERITAS_CONSTRAINT::to_veritas_conjunction_bool(*exp, stateIndexes);
            return;

        } else if (LinearConstraintsChecker::is_disjunction_bool(*exp)) {

            rlt = TO_VERITAS_CONSTRAINT::to_veritas_disjunction_bool(*exp, stateIndexes);
            return;

        } else if (LinearConstraintsChecker::is_linear_constraint(*exp, LINEAR_CONSTRAINTS_CHECKER::Specification::set_special(LINEAR_CONSTRAINTS_CHECKER::SpecialAstRequest::FALSE))) {

            rlt = TO_VERITAS_CONSTRAINT::to_veritas_linear_constraint(*exp, stateIndexes);
            return;

        }

    } else {
        PLAJA_ASSERT(LinearConstraintsChecker::is_linear_sum(*exp))
        rlt = TO_VERITAS_CONSTRAINT::to_veritas_linear_sum(assignmentTarget, assignmentType, *exp, stateIndexes);
        return;
    }

    PLAJA_ASSERT(assignmentTarget == VERITAS_IN_PLAJA::noneVar)

    /*
     * Constructing disjunctions recursively via "move_to" is inefficient due to internal vector structure.
     * Instead, one can immediately add the disjuncts to the "global" parent disjunction,
     * which is accessed as static field.
     * Still, usage of static field is over-complex. Better to just preprocess to NaryExpression (see below).
     */

// #define OPTIMIZE_DIS_TO_VERITAS

#ifdef OPTIMIZE_DIS_TO_VERITAS
    static std::list<std::unique_ptr<DisjunctionInVeritas>> parent_dis;
    constexpr bool optimize_dis_to_veritas(true);
#else
    constexpr bool optimize_dis_to_veritas(false);
#endif

    switch (exp->get_op()) {

        case BinaryOpExpression::AND: {

#ifdef OPTIMIZE_DIS_TO_VERITAS
            parent_dis.push_back(nullptr); // Conjunction opens new scope for disjunctions.
#endif

            std::unique_ptr<ConjunctionInVeritas> rlt_tmp(new ConjunctionInVeritas(context));

            /* Left. */
            exp->get_left()->accept(this);
            PLAJA_ASSERT(rlt)
            TO_VERITAS_VISITOR::move_rlt_to_target(rlt, *rlt_tmp, &VeritasConstraint::move_to_conjunction);

            /* Right. */
            exp->get_right()->accept(this);
            PLAJA_ASSERT(rlt)
            TO_VERITAS_VISITOR::move_rlt_to_target(rlt, *rlt_tmp, &VeritasConstraint::move_to_conjunction);

            rlt = std::move(rlt_tmp);

#ifdef OPTIMIZE_DIS_TO_VERITAS
            parent_dis.pop_back();
#endif

            break;
        }

        case BinaryOpExpression::OR: {

            STMT_IF_DEBUG(static bool print(true);)
            STMT_IF_DEBUG(const bool is_multi_ary = print and TO_NORMALFORM::is_multi_ary(*exp);)
            PLAJA_LOG_DEBUG_IF(print and is_multi_ary, PLAJA_UTILS::to_red_string("Warning: Constructing Veritas disjunction of arity > 3 from BinaryOpExpression."))
            PLAJA_LOG_DEBUG_IF(print and is_multi_ary, PLAJA_UTILS::to_red_string("More scalable performance can be obtained by applying TO_NORMALFORM::to_nary a priori."))
            STMT_IF_DEBUG(if (is_multi_ary) { print = false; })

#ifdef OPTIMIZE_DIS_TO_VERITAS
            const bool construct_parent_dis = parent_dis.empty() or not parent_dis.back();
            if (construct_parent_dis) { parent_dis.push_back(std::make_unique<DisjunctionInVeritas>(context)); }
            auto* rlt_tmp = parent_dis.back().get();
            PLAJA_ASSERT(rlt_tmp)
#else
            std::unique_ptr<DisjunctionInVeritas> rlt_tmp(new DisjunctionInVeritas(context));
#endif

            /* Left. */
            exp->get_left()->accept(this);
            if (optimize_dis_to_veritas) { if (rlt) { TO_VERITAS_VISITOR::move_rlt_to_target(rlt, *rlt_tmp, &VeritasConstraint::move_to_disjunction); } }
            else { TO_VERITAS_VISITOR::move_rlt_to_target(rlt, *rlt_tmp, &VeritasConstraint::move_to_disjunction); }

            /* Right. */
            exp->get_right()->accept(this);
            if (optimize_dis_to_veritas) { if (rlt) { TO_VERITAS_VISITOR::move_rlt_to_target(rlt, *rlt_tmp, &VeritasConstraint::move_to_disjunction); } }
            else { TO_VERITAS_VISITOR::move_rlt_to_target(rlt, *rlt_tmp, &VeritasConstraint::move_to_disjunction); }

#ifdef OPTIMIZE_DIS_TO_VERITAS
            if (construct_parent_dis) {
                rlt = std::move(parent_dis.back());
                parent_dis.pop_back();
            }
#else
            rlt = std::move(rlt_tmp);
#endif

            break;
        }

        default: { throw ConstructorException(exp->to_string(), PLAJA_EXCEPTION::nonLinear); }
    }

}

void ToVeritasVisitor::visit(const UnaryOpExpression* exp) {

    switch (exp->get_op()) {

        case UnaryOpExpression::NOT: {
            PLAJA_ASSERT(LinearConstraintsChecker::is_state_variable_index(*exp->get_operand()))

            const auto state_index = static_cast<const LValueExpression*>(exp->get_operand())->get_variable_index(); // NOLINT(cppcoreguidelines-pro-type-static-cast-downcast)

            if (assignmentTarget == VERITAS_IN_PLAJA::noneVar) {

                rlt = std::make_unique<VERITAS_IN_PLAJA::BoundConstraint>(context, stateIndexes.to_veritas(state_index), VERITAS_IN_PLAJA::Tightening::UB, 0); // not v_bool becomes v_veritas == 0

            } else {

                PLAJA_ASSERT_EXPECT(assignmentType == VERITAS_IN_PLAJA::Equation::EquationType::EQ)
                // t_b = not s_b becomes s_veritas + t_veritas = 1
                VERITAS_IN_PLAJA::Equation  eq(VERITAS_IN_PLAJA::Equation::EquationType::EQ);
                eq.setCoefficient(assignmentTarget, 1);
                eq.setCoefficient(stateIndexes.to_veritas(state_index), 1);
                eq.setScalar(1);
                rlt = std::make_unique<VERITAS_IN_PLAJA::EquationConstraint>(context, std::move(eq));

            }

            return;
        }

        default: { PLAJA_ABORT }

    }

    PLAJA_ABORT
}

void ToVeritasVisitor::visit(const VariableExpression* exp) {
    if (assignmentTarget == VERITAS_IN_PLAJA::noneVar) {

        PLAJA_ASSERT(exp->is_boolean())
        rlt = std::make_unique<VERITAS_IN_PLAJA::BoundConstraint>(context, stateIndexes.to_veritas(exp->get_index()), VERITAS_IN_PLAJA::Tightening::LB, 1);

    } else { // Assignment constraint.

        rlt = TO_VERITAS_VISITOR::to_copy_constraint(context, assignmentTarget, assignmentType, stateIndexes.to_veritas(exp->get_index()));

    }
}

/* non-standard *******************************************************************************************************/

void ToVeritasVisitor::visit(const StateConditionExpression* exp) {

    std::unique_ptr<ConjunctionInVeritas> rlt_tmp(new ConjunctionInVeritas(context));

    if (exp->get_constraint()) {
        exp->get_constraint()->accept(this);
        PLAJA_ASSERT(rlt)
        TO_VERITAS_VISITOR::move_rlt_to_target(rlt, *rlt_tmp, &VeritasConstraint::move_to_conjunction);
    }

    for (auto it = exp->init_loc_value_iterator(); !it.end(); ++it) {
        auto loc_value = it();
        if (stateIndexes.exists(loc_value->get_location_index())) { rlt_tmp->add_equality_constraint(stateIndexes.to_veritas(loc_value->get_location_index()), loc_value->get_location_value()); }
    }

    rlt = std::move(rlt_tmp);

}

void ToVeritasVisitor::visit(const NaryExpression* exp) {

    switch (exp->get_op()) {
        case BinaryOpExpression::AND: {
            std::unique_ptr<ConjunctionInVeritas> rlt_tmp(new ConjunctionInVeritas(context));

            for (auto it = exp->iterator(); !it.end(); ++it) {
                it->accept(this);
                rlt->move_to_conjunction(*rlt_tmp);
            }

            rlt = std::move(rlt_tmp);
            break;
        }
        case BinaryOpExpression::OR: {
            std::unique_ptr<DisjunctionInVeritas> rlt_tmp(new DisjunctionInVeritas(context));

            for (auto it = exp->iterator(); !it.end(); ++it) {
                it->accept(this);
                rlt->move_to_disjunction(*rlt_tmp);
            }

            rlt = std::move(rlt_tmp);
            break;
        }
        default: { PLAJA_ABORT }
    }

}

void ToVeritasVisitor::visit(const LinearExpression* exp) {

    /* Scalar. */
    auto scalar = exp->get_scalar()->evaluate_floating_const();

    /* Op. */
    VERITAS_IN_PLAJA::Equation::EquationType op; // NOLINT(cppcoreguidelines-init-variables)

    switch (exp->get_op()) {

        case BinaryOpExpression::LT: {
            op = VERITAS_IN_PLAJA::Equation::EquationType::LE;
            scalar -= VERITAS_IN_PLAJA::strictnessPrecision;
            break;
        }

        case BinaryOpExpression::LE: {
            op = VERITAS_IN_PLAJA::Equation::EquationType::LE;
            break;
        }

        case BinaryOpExpression::EQ: {
            op = VERITAS_IN_PLAJA::Equation::EquationType::EQ;
            break;
        }

        case BinaryOpExpression::GE: {
            op = VERITAS_IN_PLAJA::Equation::EquationType::GE;
            break;
        }

        case BinaryOpExpression::GT: {
            op = VERITAS_IN_PLAJA::Equation::EquationType::GE;
            scalar += VERITAS_IN_PLAJA::strictnessPrecision;
            break;
        }

        case BinaryOpExpression::NE: { // TODO copy as quick fix.

#ifndef NDEBUG
            static bool print(true);
            if (print) {
                PLAJA_LOG_DEBUG(PLAJA_UTILS::to_red_string("Linear non-equal constraint to Veritas."));
                print = false;
            }
#endif

            auto copy = exp->deep_copy();

            auto disjunction = std::make_unique<DisjunctionInVeritas>(context);

            /* LT. */
            copy->set_op(BinaryOpExpression::LT);
            copy->accept(this);
            rlt->move_to_disjunction(*disjunction);

            /* GT. */
            copy->set_op(BinaryOpExpression::GT);
            copy->accept(this);
            rlt->move_to_disjunction(*disjunction);

            rlt = std::move(disjunction);

            return;
        }

        case BinaryOpExpression::PLUS: {
            PLAJA_ASSERT(assignmentTarget != VERITAS_IN_PLAJA::noneVar)

            if (exp->get_number_addends() == 0) {

                rlt = VERITAS_IN_PLAJA::BoundConstraint::construct(context, assignmentTarget, assignmentType, scalar);

            } else {

                VERITAS_IN_PLAJA::Equation  eq(TO_VERITAS_VISITOR::invert_op(assignmentType));
                eq.setScalar(-scalar);
                eq.setCoefficient(assignmentTarget, -1);

                for (auto it = exp->addendIterator(); !it.end(); ++it) {
                    eq.setCoefficient(stateIndexes.to_veritas(it.var_index()), it.factor()->evaluate_floating_const());
                }

                rlt = std::make_unique<VERITAS_IN_PLAJA::EquationConstraint>(context, std::move(eq));
            }

            return;
        }

        default: { PLAJA_ABORT }
    }

    if (exp->is_bound()) {

        rlt = VERITAS_IN_PLAJA::BoundConstraint::construct(context, stateIndexes.to_veritas(exp->addendIterator().var_index()), op, scalar);

    } else {

        VERITAS_IN_PLAJA::Equation  eq;
        eq.setScalar(scalar);
        eq.setType(op);

        for (auto it = exp->addendIterator(); !it.end(); ++it) {
            eq.setCoefficient(stateIndexes.to_veritas(it.var_index()), it.factor()->evaluate_floating_const());
        }

        rlt = std::make_unique<VERITAS_IN_PLAJA::EquationConstraint>(context, std::move(eq));

    }

}

/**********************************************************************************************************************/

namespace VERITAS_IN_PLAJA {

    std::unique_ptr<VeritasConstraint> to_veritas_constraint(const Expression& exp, const StateIndexesInVeritas& state_indexes) { return ToVeritasVisitor::to_veritas_constraint(exp, state_indexes); }

    std::unique_ptr<VeritasConstraint> to_assignment_constraint(VeritasVarIndex_type target_var, const Expression& assignment_sum, const StateIndexesInVeritas& state_indexes) { return ToVeritasVisitor::to_assignment_constraint(target_var, VERITAS_IN_PLAJA::Equation::EquationType::EQ, assignment_sum, state_indexes); }

    std::unique_ptr<VeritasConstraint> to_lb_constraint(VeritasVarIndex_type target_var, const Expression& lb_sum, const StateIndexesInVeritas& state_indexes) { return ToVeritasVisitor::to_assignment_constraint(target_var, VERITAS_IN_PLAJA::Equation::EquationType::GE, lb_sum, state_indexes); }

    std::unique_ptr<VeritasConstraint> to_ub_constraint(VeritasVarIndex_type target_var, const Expression& ub_sum, const StateIndexesInVeritas& state_indexes) { return ToVeritasVisitor::to_assignment_constraint(target_var, VERITAS_IN_PLAJA::Equation::EquationType::LE, ub_sum, state_indexes); }

    std::unique_ptr<PredicateConstraint> to_predicate_constraint(const Expression& constraint, const StateIndexesInVeritas& state_indexes) {
        PLAJA_ASSERT(LinearConstraintsChecker::is_linear(constraint, LINEAR_CONSTRAINTS_CHECKER::Specification::set_special_and_predicate(LINEAR_CONSTRAINTS_CHECKER::SpecialAstRequest::NONE, true)))
        return std::make_unique<PredicateConstraint>(ToVeritasVisitor::to_veritas_constraint(constraint, state_indexes));
    }

    std::unique_ptr<VeritasConstraint> to_assignment_constraint(VERITAS_IN_PLAJA::Context& c, VeritasVarIndex_type target_var, VeritasFloating_type scalar) { // NOLINT(bugprone-easily-swappable-parameters)
        return TO_VERITAS_VISITOR::to_assignment_constraint(c, target_var, VERITAS_IN_PLAJA::Equation::EquationType::EQ, scalar);
    }

    std::unique_ptr<VeritasConstraint> to_copy_constraint(VERITAS_IN_PLAJA::Context& c, VeritasVarIndex_type target_var, VeritasVarIndex_type src_var) {
        return TO_VERITAS_VISITOR::to_copy_constraint(c, target_var, VERITAS_IN_PLAJA::Equation::EquationType::EQ, src_var);
    }

}