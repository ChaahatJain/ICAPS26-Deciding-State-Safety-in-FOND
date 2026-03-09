//
// This file is part of the PlaJA code base.
// Copyright (c) (2019 - 2023) Marcel Vinzent.
// See README.md in the top-level directory for licensing information.
//

#include "to_marabou_visitor.h"
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
#include "../constraints_in_marabou.h"
#include "../marabou_context.h"
#include "../vars_in_marabou.h"

#include "to_marabou_constraint.h" // TODO keep for now

namespace TO_MARABOU_VISITOR {

    template<class T, typename Target_t>
    void move_rlt_to_target(std::unique_ptr<T>& rlt, Target_t& target, void(T::*fx)(Target_t&)) {
        ((rlt.get())->*fx)(target);
        rlt = nullptr; // nullptr rlt is used as an indicator, e.g., during constructing of disjunctions.
    }

    extern Equation::EquationType invert_op(Equation::EquationType op);

    Equation::EquationType invert_op(Equation::EquationType op) {
        switch (op) {
            case Equation::EquationType::EQ: return Equation::EquationType::EQ;
            case Equation::EquationType::LE: return Equation::EquationType::GE;
            case Equation::EquationType::GE: return Equation::EquationType::LE;
            default: PLAJA_ABORT
        }
    }

    inline std::unique_ptr<MarabouConstraint> to_assignment_constraint(MARABOU_IN_PLAJA::Context& c, MarabouVarIndex_type target_var, Equation::EquationType op, MarabouFloating_type scalar) { // NOLINT(bugprone-easily-swappable-parameters)
        PLAJA_ASSERT(target_var != MARABOU_IN_PLAJA::noneVar)
        return BoundConstraint::construct(c, target_var, op, scalar);
    }

    std::unique_ptr<MarabouConstraint> to_copy_constraint(MARABOU_IN_PLAJA::Context& c, MarabouVarIndex_type target_var, Equation::EquationType op, MarabouVarIndex_type src_var) {
        PLAJA_ASSERT(target_var != MARABOU_IN_PLAJA::noneVar)
        Equation eq(invert_op(op));
        eq.setScalar(0);
        eq.addAddend(-1, target_var);
        eq.addAddend(1, src_var);
        auto copy_constraint = std::make_unique<EquationConstraint>(c, std::move(eq));
        return copy_constraint;
    }

}

std::unique_ptr<MarabouConstraint> ToMarabouVisitor::to_marabou_constraint(const Expression& expr, const StateIndexesInMarabou& state_indexes) {
    if (not LinearConstraintsChecker::is_linear(expr, LINEAR_CONSTRAINTS_CHECKER::Specification::set_special(LINEAR_CONSTRAINTS_CHECKER::SpecialAstRequest::NONE))) { throw ConstructorException(expr.to_string(), PLAJA_EXCEPTION::nonLinear); }
    ToMarabouVisitor to_marabou_visitor(state_indexes);
    expr.accept(&to_marabou_visitor);
    PLAJA_ASSERT(to_marabou_visitor.rlt)
    return std::move(to_marabou_visitor.rlt);
}

std::unique_ptr<MarabouConstraint> ToMarabouVisitor::to_assignment_constraint(MarabouVarIndex_type target_var, Equation::EquationType assignment_type, const Expression& assignment_sum, const StateIndexesInMarabou& state_indexes) {
    PLAJA_ASSERT(target_var != MARABOU_IN_PLAJA::noneVar)

    if (not LinearConstraintsChecker::is_linear_assignment(assignment_sum, LINEAR_CONSTRAINTS_CHECKER::Specification::set_special(LINEAR_CONSTRAINTS_CHECKER::SpecialAstRequest::NONE))) { throw ConstructorException(assignment_sum.to_string(), PLAJA_EXCEPTION::nonLinear); }

    ToMarabouVisitor to_marabou_visitor(state_indexes);
    to_marabou_visitor.assignmentTarget = target_var;
    to_marabou_visitor.assignmentType = assignment_type;

    assignment_sum.accept(&to_marabou_visitor);
    PLAJA_ASSERT(to_marabou_visitor.rlt)
    return std::move(to_marabou_visitor.rlt);
}

/**********************************************************************************************************************/

ToMarabouVisitor::ToMarabouVisitor(const StateIndexesInMarabou& state_indexes_in_marabou):
    context(state_indexes_in_marabou.get_context())
    , stateIndexes(state_indexes_in_marabou)
    , assignmentTarget(MARABOU_IN_PLAJA::noneVar)
    , assignmentType(Equation::EquationType::EQ) {
}

ToMarabouVisitor::~ToMarabouVisitor() = default;

/**********************************************************************************************************************/

void ToMarabouVisitor::visit(const ArrayAccessExpression* exp) {
    PLAJA_ASSERT(exp->get_index()->is_constant())

    if (assignmentTarget == MARABOU_IN_PLAJA::noneVar) {

        PLAJA_ASSERT(exp->is_boolean())
        rlt = std::make_unique<BoundConstraint>(context, stateIndexes.to_marabou(exp->get_variable_index()), Tightening::LB, 1);

    } else { // Assignment constraint.

        rlt = TO_MARABOU_VISITOR::to_copy_constraint(context, assignmentTarget, assignmentType, stateIndexes.to_marabou(exp->get_variable_index()));

    }
}

void ToMarabouVisitor::visit(const BoolValueExpression* exp) {
    PLAJA_ASSERT_EXPECT(assignmentType == Equation::EQ)
    rlt = TO_MARABOU_VISITOR::to_assignment_constraint(context, assignmentTarget, assignmentType, exp->evaluate_integer_const());
}

void ToMarabouVisitor::visit(const IntegerValueExpression* exp) {
    rlt = TO_MARABOU_VISITOR::to_assignment_constraint(context, assignmentTarget, assignmentType, exp->evaluate_integer_const());
}

void ToMarabouVisitor::visit(const RealValueExpression* exp) {
    rlt = TO_MARABOU_VISITOR::to_assignment_constraint(context, assignmentTarget, assignmentType, exp->evaluate_floating_const());
}

void ToMarabouVisitor::visit(const BinaryOpExpression* exp) {

    /* catch cases currently encoded via old structure:. */
    if (assignmentTarget == MARABOU_IN_PLAJA::noneVar) {

        if (LinearConstraintsChecker::is_conjunction_bool(*exp)) {

            rlt = TO_MARABOU_CONSTRAINT::to_marabou_conjunction_bool(*exp, stateIndexes);
            return;

        } else if (LinearConstraintsChecker::is_disjunction_bool(*exp)) {

            rlt = TO_MARABOU_CONSTRAINT::to_marabou_disjunction_bool(*exp, stateIndexes);
            return;

        } else if (LinearConstraintsChecker::is_linear_constraint(*exp, LINEAR_CONSTRAINTS_CHECKER::Specification::set_special(LINEAR_CONSTRAINTS_CHECKER::SpecialAstRequest::FALSE))) {

            rlt = TO_MARABOU_CONSTRAINT::to_marabou_linear_constraint(*exp, stateIndexes);
            return;

        }

    } else {
        PLAJA_ASSERT(LinearConstraintsChecker::is_linear_sum(*exp))
        rlt = TO_MARABOU_CONSTRAINT::to_marabou_linear_sum(assignmentTarget, assignmentType, *exp, stateIndexes);
        return;
    }

    PLAJA_ASSERT(assignmentTarget == MARABOU_IN_PLAJA::noneVar)

    /*
     * Constructing disjunctions recursively via "move_to" is inefficient due to internal vector structure.
     * Instead, one can immediately add the disjuncts to the "global" parent disjunction,
     * which is accessed as static field.
     * Still, usage of static field is over-complex. Better to just preprocess to NaryExpression (see below).
     */

// #define OPTIMIZE_DIS_TO_MARABOU

#ifdef OPTIMIZE_DIS_TO_MARABOU
    static std::list<std::unique_ptr<DisjunctionInMarabou>> parent_dis;
    constexpr bool optimize_dis_to_marabou(true);
#else
    constexpr bool optimize_dis_to_marabou(false);
#endif

    switch (exp->get_op()) {

        case BinaryOpExpression::AND: {

#ifdef OPTIMIZE_DIS_TO_MARABOU
            parent_dis.push_back(nullptr); // Conjunction opens new scope for disjunctions.
#endif

            std::unique_ptr<ConjunctionInMarabou> rlt_tmp(new ConjunctionInMarabou(context));

            /* Left. */
            exp->get_left()->accept(this);
            PLAJA_ASSERT(rlt)
            TO_MARABOU_VISITOR::move_rlt_to_target(rlt, *rlt_tmp, &MarabouConstraint::move_to_conjunction);

            /* Right. */
            exp->get_right()->accept(this);
            PLAJA_ASSERT(rlt)
            TO_MARABOU_VISITOR::move_rlt_to_target(rlt, *rlt_tmp, &MarabouConstraint::move_to_conjunction);

            rlt = std::move(rlt_tmp);

#ifdef OPTIMIZE_DIS_TO_MARABOU
            parent_dis.pop_back();
#endif

            break;
        }

        case BinaryOpExpression::OR: {

            STMT_IF_DEBUG(static bool print(true);)
            STMT_IF_DEBUG(const bool is_multi_ary = print and TO_NORMALFORM::is_multi_ary(*exp);)
            PLAJA_LOG_DEBUG_IF(print and is_multi_ary, PLAJA_UTILS::to_red_string("Warning: Constructing Marabou disjunction of arity > 3 from BinaryOpExpression."))
            PLAJA_LOG_DEBUG_IF(print and is_multi_ary, PLAJA_UTILS::to_red_string("More scalable performance can be obtained by applying TO_NORMALFORM::to_nary a priori."))
            STMT_IF_DEBUG(if (is_multi_ary) { print = false; })

#ifdef OPTIMIZE_DIS_TO_MARABOU
            const bool construct_parent_dis = parent_dis.empty() or not parent_dis.back();
            if (construct_parent_dis) { parent_dis.push_back(std::make_unique<DisjunctionInMarabou>(context)); }
            auto* rlt_tmp = parent_dis.back().get();
            PLAJA_ASSERT(rlt_tmp)
#else
            std::unique_ptr<DisjunctionInMarabou> rlt_tmp(new DisjunctionInMarabou(context));
#endif

            /* Left. */
            exp->get_left()->accept(this);
            if (optimize_dis_to_marabou) { if (rlt) { TO_MARABOU_VISITOR::move_rlt_to_target(rlt, *rlt_tmp, &MarabouConstraint::move_to_disjunction); } }
            else { TO_MARABOU_VISITOR::move_rlt_to_target(rlt, *rlt_tmp, &MarabouConstraint::move_to_disjunction); }

            /* Right. */
            exp->get_right()->accept(this);
            if (optimize_dis_to_marabou) { if (rlt) { TO_MARABOU_VISITOR::move_rlt_to_target(rlt, *rlt_tmp, &MarabouConstraint::move_to_disjunction); } }
            else { TO_MARABOU_VISITOR::move_rlt_to_target(rlt, *rlt_tmp, &MarabouConstraint::move_to_disjunction); }

#ifdef OPTIMIZE_DIS_TO_MARABOU
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

void ToMarabouVisitor::visit(const UnaryOpExpression* exp) {

    switch (exp->get_op()) {

        case UnaryOpExpression::NOT: {
            PLAJA_ASSERT(LinearConstraintsChecker::is_state_variable_index(*exp->get_operand()))

            const auto state_index = static_cast<const LValueExpression*>(exp->get_operand())->get_variable_index(); // NOLINT(cppcoreguidelines-pro-type-static-cast-downcast)

            if (assignmentTarget == MARABOU_IN_PLAJA::noneVar) {

                rlt = std::make_unique<BoundConstraint>(context, stateIndexes.to_marabou(state_index), Tightening::UB, 0); // not v_bool becomes v_marabou == 0

            } else {

                PLAJA_ASSERT_EXPECT(assignmentType == Equation::EQ)
                // t_b = not s_b becomes s_marabou + t_marabou = 1
                Equation eq(Equation::EQ);
                eq.setCoefficient(assignmentTarget, 1);
                eq.setCoefficient(stateIndexes.to_marabou(state_index), 1);
                eq.setScalar(1);
                rlt = std::make_unique<EquationConstraint>(context, std::move(eq));

            }

            return;
        }

        default: { PLAJA_ABORT }

    }

    PLAJA_ABORT
}

void ToMarabouVisitor::visit(const VariableExpression* exp) {
    if (assignmentTarget == MARABOU_IN_PLAJA::noneVar) {

        PLAJA_ASSERT(exp->is_boolean())
        rlt = std::make_unique<BoundConstraint>(context, stateIndexes.to_marabou(exp->get_index()), Tightening::LB, 1);

    } else { // Assignment constraint.

        rlt = TO_MARABOU_VISITOR::to_copy_constraint(context, assignmentTarget, assignmentType, stateIndexes.to_marabou(exp->get_index()));

    }
}

/* non-standard *******************************************************************************************************/

void ToMarabouVisitor::visit(const StateConditionExpression* exp) {

    std::unique_ptr<ConjunctionInMarabou> rlt_tmp(new ConjunctionInMarabou(context));

    if (exp->get_constraint()) {
        exp->get_constraint()->accept(this);
        PLAJA_ASSERT(rlt)
        TO_MARABOU_VISITOR::move_rlt_to_target(rlt, *rlt_tmp, &MarabouConstraint::move_to_conjunction);
    }

    for (auto it = exp->init_loc_value_iterator(); !it.end(); ++it) {
        auto loc_value = it();
        if (stateIndexes.exists(loc_value->get_location_index())) { rlt_tmp->add_equality_constraint(stateIndexes.to_marabou(loc_value->get_location_index()), loc_value->get_location_value()); }
    }

    rlt = std::move(rlt_tmp);

}

void ToMarabouVisitor::visit(const NaryExpression* exp) {

    switch (exp->get_op()) {
        case BinaryOpExpression::AND: {
            std::unique_ptr<ConjunctionInMarabou> rlt_tmp(new ConjunctionInMarabou(context));

            for (auto it = exp->iterator(); !it.end(); ++it) {
                it->accept(this);
                rlt->move_to_conjunction(*rlt_tmp);
            }

            rlt = std::move(rlt_tmp);
            break;
        }
        case BinaryOpExpression::OR: {
            std::unique_ptr<DisjunctionInMarabou> rlt_tmp(new DisjunctionInMarabou(context));

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

void ToMarabouVisitor::visit(const LinearExpression* exp) {

    /* Scalar. */
    auto scalar = exp->get_scalar()->evaluate_floating_const();

    /* Op. */
    Equation::EquationType op; // NOLINT(cppcoreguidelines-init-variables)

    switch (exp->get_op()) {

        case BinaryOpExpression::LT: {
            op = Equation::LE;
            scalar -= MARABOU_IN_PLAJA::strictnessPrecision;
            break;
        }

        case BinaryOpExpression::LE: {
            op = Equation::LE;
            break;
        }

        case BinaryOpExpression::EQ: {
            op = Equation::EQ;
            break;
        }

        case BinaryOpExpression::GE: {
            op = Equation::GE;
            break;
        }

        case BinaryOpExpression::GT: {
            op = Equation::GE;
            scalar += MARABOU_IN_PLAJA::strictnessPrecision;
            break;
        }

        case BinaryOpExpression::NE: { // TODO copy as quick fix.

#ifndef NDEBUG
            static bool print(true);
            if (print) {
                PLAJA_LOG_DEBUG(PLAJA_UTILS::to_red_string("Linear non-equal constraint to Marabou."));
                print = false;
            }
#endif

            auto copy = exp->deep_copy();

            auto disjunction = std::make_unique<DisjunctionInMarabou>(context);

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
            PLAJA_ASSERT(assignmentTarget != MARABOU_IN_PLAJA::noneVar)

            if (exp->get_number_addends() == 0) {

                rlt = BoundConstraint::construct(context, assignmentTarget, assignmentType, scalar);

            } else {

                Equation eq(TO_MARABOU_VISITOR::invert_op(assignmentType));
                eq.setScalar(-scalar);
                eq.setCoefficient(assignmentTarget, -1);

                for (auto it = exp->addendIterator(); !it.end(); ++it) {
                    eq.setCoefficient(stateIndexes.to_marabou(it.var_index()), it.factor()->evaluate_floating_const());
                }

                rlt = std::make_unique<EquationConstraint>(context, std::move(eq));
            }

            return;
        }

        default: { PLAJA_ABORT }
    }

    if (exp->is_bound()) {

        rlt = BoundConstraint::construct(context, stateIndexes.to_marabou(exp->addendIterator().var_index()), op, scalar);

    } else {

        Equation eq;
        eq.setScalar(scalar);
        eq.setType(op);

        for (auto it = exp->addendIterator(); !it.end(); ++it) {
            eq.setCoefficient(stateIndexes.to_marabou(it.var_index()), it.factor()->evaluate_floating_const());
        }

        rlt = std::make_unique<EquationConstraint>(context, std::move(eq));

    }

}

/**********************************************************************************************************************/

namespace MARABOU_IN_PLAJA {

    std::unique_ptr<MarabouConstraint> to_marabou_constraint(const Expression& exp, const StateIndexesInMarabou& state_indexes) { return ToMarabouVisitor::to_marabou_constraint(exp, state_indexes); }

    std::unique_ptr<MarabouConstraint> to_assignment_constraint(MarabouVarIndex_type target_var, const Expression& assignment_sum, const StateIndexesInMarabou& state_indexes) { return ToMarabouVisitor::to_assignment_constraint(target_var, Equation::EQ, assignment_sum, state_indexes); }

    std::unique_ptr<MarabouConstraint> to_lb_constraint(MarabouVarIndex_type target_var, const Expression& lb_sum, const StateIndexesInMarabou& state_indexes) { return ToMarabouVisitor::to_assignment_constraint(target_var, Equation::GE, lb_sum, state_indexes); }

    std::unique_ptr<MarabouConstraint> to_ub_constraint(MarabouVarIndex_type target_var, const Expression& ub_sum, const StateIndexesInMarabou& state_indexes) { return ToMarabouVisitor::to_assignment_constraint(target_var, Equation::LE, ub_sum, state_indexes); }

    std::unique_ptr<PredicateConstraint> to_predicate_constraint(const Expression& constraint, const StateIndexesInMarabou& state_indexes) {
        PLAJA_ASSERT(LinearConstraintsChecker::is_linear(constraint, LINEAR_CONSTRAINTS_CHECKER::Specification::set_special_and_predicate(LINEAR_CONSTRAINTS_CHECKER::SpecialAstRequest::NONE, true)))
        return std::make_unique<PredicateConstraint>(ToMarabouVisitor::to_marabou_constraint(constraint, state_indexes));
    }

    std::unique_ptr<MarabouConstraint> to_assignment_constraint(MARABOU_IN_PLAJA::Context& c, MarabouVarIndex_type target_var, MarabouFloating_type scalar) { // NOLINT(bugprone-easily-swappable-parameters)
        return TO_MARABOU_VISITOR::to_assignment_constraint(c, target_var, Equation::EQ, scalar);
    }

    std::unique_ptr<MarabouConstraint> to_copy_constraint(MARABOU_IN_PLAJA::Context& c, MarabouVarIndex_type target_var, MarabouVarIndex_type src_var) {
        return TO_MARABOU_VISITOR::to_copy_constraint(c, target_var, Equation::EQ, src_var);
    }

}