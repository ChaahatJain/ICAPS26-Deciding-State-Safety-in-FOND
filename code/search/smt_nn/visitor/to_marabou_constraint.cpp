//
// This file is part of the PlaJA code base.
// Copyright (c) (2019 - 2020) Marcel Vinzent.
// See README.md in the top-level directory for licensing information.
//

#include <memory>
#include "to_marabou_constraint.h"
#include "../../../utils/floating_utils.h"
#include "../../../parser/ast/expression/binary_op_expression.h"
#include "../../../parser/ast/expression/unary_op_expression.h"
#include "../../../parser/ast/expression/variable_expression.h"
#include "../../../parser/ast/type/int_type.h"
#include "../../../parser/visitor/linear_constraints_checker.h"
#include "../constraints_in_marabou.h"
#include "../marabou_context.h"
#include "../vars_in_marabou.h"


/* Auxiliary. */

namespace MARABOU_IN_PLAJA {
    constexpr MarabouInteger_type False = -1;
    constexpr MarabouInteger_type True = 1;
}

namespace TO_MARABOU_VISITOR { extern Equation::EquationType invert_op(Equation::EquationType op); }

/** Marabou operator type and offset (e.g., for strict op).*/
std::pair<Equation::EquationType, MarabouFloating_type> jani_op_to_marabou(BinaryOpExpression::BinaryOp op) {
    switch (op) {
        case BinaryOpExpression::EQ: return { Equation::EquationType::EQ, 0 };
        case BinaryOpExpression::LE: return { Equation::EquationType::LE, 0 };
        case BinaryOpExpression::GE: return { Equation::EquationType::GE, 0 };
        case BinaryOpExpression::LT: return { Equation::EquationType::LE, -MARABOU_IN_PLAJA::strictnessPrecision };
        case BinaryOpExpression::GT: return { Equation::EquationType::GE, MARABOU_IN_PLAJA::strictnessPrecision };
        default: PLAJA_ABORT
    }
}

/* Auxiliary class to detect and merge duplicate addends *before* adding to Marabou's equation as they seem to trouble Marabou. */
struct PreEquation {
    const StateIndexesInMarabou* stateIndexesInMarabou;
    std::unordered_map<MarabouVarIndex_type, MarabouFloating_type> addends;
    MarabouFloating_type scalar;
    Equation::EquationType type;

    explicit PreEquation(const StateIndexesInMarabou& state_indexes_in_marabou, Equation::EquationType type_, MarabouFloating_type init_scalar = 0):
        stateIndexesInMarabou(&state_indexes_in_marabou)
        , scalar(init_scalar)
        , type(type_) {}

    virtual ~PreEquation() = default;

    DEFAULT_CONSTRUCTOR_ONLY(PreEquation)
    DELETE_ASSIGNMENT(PreEquation)

    [[nodiscard]] MARABOU_IN_PLAJA::Context& _context() const { return stateIndexesInMarabou->get_context(); }

    Equation to_equation() {
        Equation equation(type);
        equation.setScalar(scalar);
        for (auto& addend: addends) {
            PLAJA_ASSERT(addend.second != 0)
            equation.addAddend(addend.second, addend.first); // Note equations takes coefficient (second) then variable index (first).
        }
        return equation;
    }

    virtual void add_scalar(MarabouFloating_type scalar_additive, MarabouFloating_type factor) { scalar += factor * scalar_additive; } // Factor is, e.g., minus if scalar is position right-hand in the translated expression.

    void add_addend(VariableIndex_type state_index, MarabouFloating_type coefficient) {
        /* Factor is, e.g., minus if addend is position left-hand in the translated expression. */
        PLAJA_ASSERT(stateIndexesInMarabou->exists(state_index))
        const MarabouVarIndex_type var_index = stateIndexesInMarabou->to_marabou(state_index);
        if (addends.count(var_index)) { addends[var_index] += coefficient; }
        else { addends.insert(std::make_pair(var_index, coefficient)); }
    }

    virtual std::unique_ptr<MarabouConstraint> to_marabou_constraint() {
        PLAJA_ASSERT(not addends.empty())

        if (addends.size() == 1) {

            auto addends_it = addends.begin();

            /* Special case bound constraints. */
            auto var = addends_it->first;
            auto factor = addends_it->second;
            PLAJA_ASSERT(factor != 0)

            if (_context().is_marked_integer(var)) { // Special handling for integer vars.

                auto factored_scalar = scalar / factor;
                auto factored_type = factor < 0 ? TO_MARABOU_VISITOR::invert_op(type) : type;
                switch (factored_type) {
                    case Equation::EQ: {

                        if (not PLAJA_FLOATS::is_integer(factored_scalar, MARABOU_IN_PLAJA::integerPrecision)) {

                            if (FloatUtils::isFinite(_context().get_lower_bound(var))) { // Must use Marabou-based infinity check to detect Marabou-based infinity.

                                return std::make_unique<BoundConstraint>(_context(), var, Tightening::UB, _context().get_lower_bound(var) - 1); // UNSAT

                            } else if (FloatUtils::isFinite(_context().get_upper_bound(var))) {

                                return std::make_unique<BoundConstraint>(_context(), var, Tightening::LB, _context().get_upper_bound(var) + 1); // UNSAT

                            } else {

                                Equation eq(Equation::EQ);
                                eq.setScalar(factored_scalar);
                                eq.addAddend(1, var);
                                return std::make_unique<EquationConstraint>(_context(), std::move(eq));

                            }

                        } else {
                            return BoundConstraint::construct_equality_constraint(_context(), var, factored_scalar);
                        }

                    }
                    case Equation::GE: { return std::make_unique<BoundConstraint>(_context(), var, Tightening::LB, std::ceil(factored_scalar)); }
                    case Equation::LE: { return std::make_unique<BoundConstraint>(_context(), var, Tightening::UB, std::floor(factored_scalar)); }
                    default: { PLAJA_ABORT }
                }

            } else { return BoundConstraint::construct(_context(), var, factor < 0 ? TO_MARABOU_VISITOR::invert_op(type) : type, scalar / factor); }

        }

        /* else: equation constraint. */
        return std::make_unique<EquationConstraint>(_context(), to_equation());

    }

};

/* Special handling of duplicate addends necessary. */
struct PreEquationBool: public PreEquation {
    /* Class assumption: A & B & not(C) becomes A + B + (1 - C) >= 3 becomes A + B - C >= 2 */
    /* Class assumption: A | B | not(C) becomes A + B + (1 - C) >= 1 becomes A + B - C >= 0 */

    static_assert(MARABOU_IN_PLAJA::False == -1);
    static_assert(MARABOU_IN_PLAJA::True == 1);
    bool isTrivial;

    explicit PreEquationBool(const StateIndexesInMarabou& state_indexes_in_marabou):
        PreEquation(state_indexes_in_marabou, Equation::EquationType::GE)
        , isTrivial(false) {}

    ~PreEquationBool() override = default;

    DEFAULT_CONSTRUCTOR_ONLY(PreEquationBool)
    DELETE_ASSIGNMENT(PreEquationBool)

    void add_scalar(MarabouFloating_type /*scalar_additive*/, MarabouFloating_type /*factor*/) override { PLAJA_ABORT }

    virtual void add_scalar(MarabouInteger_type scalar_additive) = 0;

    void add_addend(VariableIndex_type state_index, MarabouInteger_type coefficient) { // NOLINT(bugprone-easily-swappable-parameters)
        PLAJA_ASSERT(coefficient == MARABOU_IN_PLAJA::False or coefficient == MARABOU_IN_PLAJA::True)
        PLAJA_ASSERT(stateIndexesInMarabou->exists(state_index))
        const MarabouVarIndex_type var_index = stateIndexesInMarabou->to_marabou(state_index);

        if (addends.count(var_index)) {
            auto current_coefficient = addends[var_index];
            if (current_coefficient == coefficient) { return; }
            else { isTrivial = true; } // Positive and negative literal -> the conjunction is false | the disjunction is true.
        } else {
            addends.insert(std::make_pair(var_index, coefficient));
            if (coefficient == MARABOU_IN_PLAJA::True) { ++scalar; } // Count occurrences of positive literals.
        }

    }

    std::unique_ptr<MarabouConstraint> to_marabou_constraint() override = 0;

};

struct PreEquationConBool: public PreEquationBool {

    explicit PreEquationConBool(const StateIndexesInMarabou& state_indexes_in_marabou):
        PreEquationBool(state_indexes_in_marabou) {}

    ~PreEquationConBool() override = default;

    DEFAULT_CONSTRUCTOR_ONLY(PreEquationConBool)
    DELETE_ASSIGNMENT(PreEquationConBool)

    void add_scalar(MarabouInteger_type scalar_additive) override {
        PLAJA_ASSERT(scalar_additive == MARABOU_IN_PLAJA::False or scalar_additive == MARABOU_IN_PLAJA::True)
        if (scalar_additive == MARABOU_IN_PLAJA::False) { isTrivial = true; }
    }

    std::unique_ptr<MarabouConstraint> to_marabou_constraint() override {

        if (isTrivial) { // Special case (unsat).

            if (addends.empty()) { return _context().trivially_false(); }
            else { return std::make_unique<BoundConstraint>(_context(), addends.begin()->first, Tightening::UB, -1); } // Encodes unsat for bools.

        } else {

            PLAJA_ASSERT(not addends.empty())
            return PreEquation::to_marabou_constraint(); // NOLINT(bugprone-parent-virtual-call)

        }

    }

};

struct PreEquationDisBool: public PreEquationBool {

    explicit PreEquationDisBool(const StateIndexesInMarabou& state_indexes_in_marabou):
        PreEquationBool(state_indexes_in_marabou) {}

    ~PreEquationDisBool() override = default;

    DEFAULT_CONSTRUCTOR_ONLY(PreEquationDisBool)
    DELETE_ASSIGNMENT(PreEquationDisBool)

    void add_scalar(MarabouInteger_type scalar_additive) override {
        PLAJA_ASSERT(scalar_additive == MARABOU_IN_PLAJA::False or scalar_additive == MARABOU_IN_PLAJA::True)
        if (scalar_additive == MARABOU_IN_PLAJA::True) { isTrivial = true; }
    }

    std::unique_ptr<MarabouConstraint> to_marabou_constraint() override {
        if (isTrivial) { return _context().trivially_true(); }
        else if (addends.empty()) { return _context().trivially_false(); } // Special case (unsat).
        else {
            PLAJA_ASSERT(not addends.empty())
            return PreEquation::to_marabou_constraint(); // NOLINT(bugprone-parent-virtual-call)
        }
    }

};

/**Boolean structures**************************************************************************************************/

void literal_to_marabou(const Expression& expr, PreEquationBool& pre_equation) {

    if (LinearConstraintsChecker::is_state_variable_index(expr, LINEAR_CONSTRAINTS_CHECKER::Specification())) {

        pre_equation.add_addend(PLAJA_UTILS::cast_ref<LValueExpression>(expr).get_variable_index(), MARABOU_IN_PLAJA::True);
        return;

    } else if (expr.is_constant() and expr.get_type()->is_boolean_type()) {

        pre_equation.add_scalar(expr.evaluate_integer_const() ? MARABOU_IN_PLAJA::True : MARABOU_IN_PLAJA::False);

    } else {

        const auto& unary_exp = PLAJA_UTILS::cast_ref<UnaryOpExpression>(expr);
        PLAJA_ASSERT(unary_exp.get_op() == UnaryOpExpression::NOT)
        PLAJA_ASSERT(LinearConstraintsChecker::is_state_variable_index(*unary_exp.get_operand(), LINEAR_CONSTRAINTS_CHECKER::Specification()))
        pre_equation.add_addend(PLAJA_UTILS::cast_ptr<LValueExpression>(unary_exp.get_operand())->get_variable_index(), MARABOU_IN_PLAJA::False);
        return;

    }

}

void bool_junction_to_marabou(const Expression& expr, PreEquationBool& pre_equation) { // NOLINT(misc-no-recursion)

    if (LinearConstraintsChecker::is_literal(expr, LINEAR_CONSTRAINTS_CHECKER::Specification())) {
        literal_to_marabou(expr, pre_equation);
        return;
    }

    /* Else. */
    const auto& binary_exp = PLAJA_UTILS::cast_ref<BinaryOpExpression>(expr);
    PLAJA_ASSERT(binary_exp.get_op() == BinaryOpExpression::AND or binary_exp.get_op() == BinaryOpExpression::OR)
    PLAJA_ASSERT(binary_exp.get_left())
    PLAJA_ASSERT(binary_exp.get_right())
    bool_junction_to_marabou(*binary_exp.get_left(), pre_equation);
    bool_junction_to_marabou(*binary_exp.get_right(), pre_equation);

}

/**Linear constraint***************************************************************************************************/

void addend_to_marabou(const Expression& expr, PreEquation& pre_equation, MarabouFloating_type global_factor /*as left-hand addend in Marabou*/) {
    auto specification = LINEAR_CONSTRAINTS_CHECKER::Specification::set_bools(false);

    if (LinearConstraintsChecker::is_state_variable_index(expr, specification)) {

        pre_equation.add_addend(PLAJA_UTILS::cast_ref<LValueExpression>(expr).get_variable_index(), global_factor);
        return;

    } else {

        const auto& binary_exp = PLAJA_UTILS::cast_ref<BinaryOpExpression>(expr);
        PLAJA_ASSERT(binary_exp.get_op() == BinaryOpExpression::TIMES)

        if (LinearConstraintsChecker::is_state_variable_index(*binary_exp.get_left(), specification)) { // Variable to the left.
            PLAJA_ASSERT(PLAJA_UTILS::is_derived_ptr_type<LValueExpression>(binary_exp.get_left()))
            PLAJA_ASSERT(LinearConstraintsChecker::is_factor(*binary_exp.get_right()))
            pre_equation.add_addend(PLAJA_UTILS::cast_ptr<LValueExpression>(binary_exp.get_left())->get_variable_index(), binary_exp.get_right()->evaluate_floating_const() * global_factor);
            return;
        }

        if (LinearConstraintsChecker::is_state_variable_index(*binary_exp.get_right(), specification)) { // Variable to the right.
            PLAJA_ASSERT(PLAJA_UTILS::is_derived_ptr_type<LValueExpression>(binary_exp.get_right()))
            PLAJA_ASSERT(LinearConstraintsChecker::is_factor(*binary_exp.get_left()))
            pre_equation.add_addend(PLAJA_UTILS::cast_ptr<LValueExpression>(binary_exp.get_right())->get_variable_index(), binary_exp.get_left()->evaluate_floating_const() * global_factor);
            return;
        }

    }

    PLAJA_ABORT
}

void linear_sum_to_marabou(const Expression& expr, PreEquation& pre_equation, MarabouFloating_type global_factor /*as left-hand sum in Marabou*/) { // NOLINT(misc-no-recursion)
    if (LinearConstraintsChecker::is_addend(expr, LINEAR_CONSTRAINTS_CHECKER::Specification())) {
        addend_to_marabou(expr, pre_equation, global_factor);
        return;
    }
    if (LinearConstraintsChecker::is_scalar(expr)) {
        pre_equation.add_scalar(expr.evaluate_floating_const(), -1 * global_factor);
        return;
    }

    /* Else. */
    const auto& binary_exp = PLAJA_UTILS::cast_ref<BinaryOpExpression>(expr);
    PLAJA_ASSERT(binary_exp.get_left())
    PLAJA_ASSERT(binary_exp.get_right())

    switch (binary_exp.get_op()) {

        case BinaryOpExpression::PLUS: {
            linear_sum_to_marabou(*binary_exp.get_left(), pre_equation, global_factor);
            linear_sum_to_marabou(*binary_exp.get_right(), pre_equation, global_factor);
            return;
        }

        case BinaryOpExpression::TIMES: {
            if (LinearConstraintsChecker::is_factor(*binary_exp.get_left())) {
                linear_sum_to_marabou(*binary_exp.get_right(), pre_equation, binary_exp.get_left()->evaluate_floating_const() * global_factor);
            } else {
                PLAJA_ASSERT(LinearConstraintsChecker::is_factor(*binary_exp.get_right()))
                linear_sum_to_marabou(*binary_exp.get_left(), pre_equation, binary_exp.get_right()->evaluate_floating_const() * global_factor);
            }
            return;
        }

        default: PLAJA_ABORT

    }
}

/**********************************************************************************************************************/

std::unique_ptr<MarabouConstraint> TO_MARABOU_CONSTRAINT::to_marabou_linear_constraint(const Expression& expr, const StateIndexesInMarabou& state_indexes_in_marabou) {
    PLAJA_ASSERT(LinearConstraintsChecker::is_linear_constraint(expr))

    auto& binary_exp = PLAJA_UTILS::cast_ref<BinaryOpExpression>(expr);
    const auto op_offset = jani_op_to_marabou(binary_exp.get_op());

    PreEquation pre_equation(state_indexes_in_marabou, op_offset.first, op_offset.second);
    linear_sum_to_marabou(*binary_exp.get_left(), pre_equation, 1);
    linear_sum_to_marabou(*binary_exp.get_right(), pre_equation, -1);
    return pre_equation.to_marabou_constraint();
}

std::unique_ptr<MarabouConstraint> TO_MARABOU_CONSTRAINT::to_marabou_conjunction_bool(const Expression& expr, const StateIndexesInMarabou& state_indexes_in_marabou) {
    PLAJA_ASSERT(LinearConstraintsChecker::is_conjunction_bool(expr))
    PreEquationConBool pre_equation(state_indexes_in_marabou);
    bool_junction_to_marabou(expr, pre_equation);
    return pre_equation.to_marabou_constraint();
}

std::unique_ptr<MarabouConstraint> TO_MARABOU_CONSTRAINT::to_marabou_disjunction_bool(const Expression& expr, const StateIndexesInMarabou& state_indexes_in_marabou) {
    PLAJA_ASSERT(LinearConstraintsChecker::is_disjunction_bool(expr))
    PreEquationDisBool pre_equation(state_indexes_in_marabou);
    bool_junction_to_marabou(expr, pre_equation);
    return pre_equation.to_marabou_constraint();
}

std::unique_ptr<MarabouConstraint> TO_MARABOU_CONSTRAINT::to_marabou_linear_sum(MarabouVarIndex_type target_var, Equation::EquationType op, const Expression& assignment_sum, const StateIndexesInMarabou& state_indexes_in_marabou) {
    PLAJA_ASSERT(LinearConstraintsChecker::is_linear_sum(assignment_sum))
    PreEquation pre_equation(state_indexes_in_marabou, TO_MARABOU_VISITOR::invert_op(op));
    linear_sum_to_marabou(assignment_sum, pre_equation, 1);
    pre_equation.addends[target_var] = -1; // note: y [op] a*x + c becomes -c [invert(op)] a*x - y
    return pre_equation.to_marabou_constraint();
}

/**********************************************************************************************************************/

#if 0

void to_marabou_constraint_rec(const Expression& expr, std::unique_ptr<ConjunctionInMarabou>& marabou_constraint_conjunction) { // NOLINT(misc-no-recursion)

    const LINEAR_CONSTRAINTS_CHECKER::Specification specification;

    if (LinearConstraintsChecker::is_conjunction_bool(expr, specification)) {
        PreEquationConBool pre_equation(marabou_constraint_conjunction->_context());
        bool_junction_to_marabou(expr, pre_equation);
        pre_equation.to_marabou_constraint()->move_to_conjunction(*marabou_constraint_conjunction);
        return;
    }

    if (LinearConstraintsChecker::is_disjunction_bool(expr, specification)) {
        PreEquationDisBool pre_equation(marabou_constraint_conjunction->_context());
        bool_junction_to_marabou(expr, pre_equation);
        pre_equation.to_marabou_constraint()->move_to_conjunction(*marabou_constraint_conjunction);
        return;
    }

    if (LinearConstraintsChecker::is_linear_constraint(expr, specification)) {
        to_marabou_linear_constraint(marabou_constraint_conjunction->_context(), expr)->move_to_conjunction(*marabou_constraint_conjunction);
        return;
    }

    // else:
    auto* binary_exp = PLAJA_UTILS::cast_ptr<BinaryOpExpression>(&expr);
    PLAJA_ASSERT(binary_exp)
    PLAJA_ASSERT(binary_exp->get_op() == BinaryOpExpression::AND)
    to_marabou_constraint_rec(*binary_exp->get_left(), marabou_constraint_conjunction);
    to_marabou_constraint_rec(*binary_exp->get_right(), marabou_constraint_conjunction);

}

void to_marabou_constraint_rec(const Expression& expr, std::unique_ptr<DisjunctionInMarabou>& marabou_constraint_disjunction) { // NOLINT(misc-no-recursion)

    const LINEAR_CONSTRAINTS_CHECKER::Specification specification;

    if (LinearConstraintsChecker::is_conjunction_bool(expr, specification)) {
        PreEquationConBool pre_equation(marabou_constraint_disjunction->_context());
        bool_junction_to_marabou(expr, pre_equation);
        pre_equation.to_marabou_constraint()->move_to_disjunction(*marabou_constraint_disjunction);
        return;
    }

    if (LinearConstraintsChecker::is_disjunction_bool(expr, specification)) {
        PreEquationDisBool pre_equation(marabou_constraint_disjunction->_context());
        bool_junction_to_marabou(expr, pre_equation);
        pre_equation.to_marabou_constraint()->move_to_disjunction(*marabou_constraint_disjunction);
        return;
    }

    if (LinearConstraintsChecker::is_linear_constraint(expr, specification)) {
        to_marabou_linear_constraint(marabou_constraint_disjunction->_context(), expr)->move_to_disjunction(*marabou_constraint_disjunction);
        return;
    }

    // else:
    auto* binary_exp = PLAJA_UTILS::cast_ptr<BinaryOpExpression>(&expr);
    PLAJA_ASSERT(binary_exp)
    PLAJA_ASSERT(binary_exp->get_op() == BinaryOpExpression::OR)
    to_marabou_constraint_rec(*binary_exp->get_left(), marabou_constraint_disjunction);
    to_marabou_constraint_rec(*binary_exp->get_right(), marabou_constraint_disjunction);

}

/**********************************************************************************************************************/

std::unique_ptr<MarabouConstraint> ToMarabouConstraint::to_marabou_constraint(const Expression& constraint, const StateIndexesInMarabou& state_indexes_in_marabou) {

    const LINEAR_CONSTRAINTS_CHECKER::Specification specification;

    PLAJA_ASSERT(LinearConstraintsChecker::is_linear(constraint, specification))
    MARABOU_IN_PLAJA::stateIndexesToMarabou = &state_indexes_in_marabou;

    if (LinearConstraintsChecker::is_conjunction_bool(constraint, specification)) {
        PreEquationConBool pre_equation(state_indexes_in_marabou._context());
        bool_junction_to_marabou(constraint, pre_equation);
        return pre_equation.to_marabou_constraint();
    }

    if (LinearConstraintsChecker::is_disjunction_bool(constraint, specification)) {
        PreEquationDisBool pre_equation(state_indexes_in_marabou._context());
        bool_junction_to_marabou(constraint, pre_equation);
        return pre_equation.to_marabou_constraint();
    }

    if (LinearConstraintsChecker::is_linear_constraint(constraint, specification)) { return to_marabou_linear_constraint(state_indexes_in_marabou._context(), constraint); }

    // else:
    if (LinearConstraintsChecker::is_linear_conjunction(constraint, specification)) {
        std::unique_ptr<ConjunctionInMarabou> marabou_constraint_conjunction(new ConjunctionInMarabou(state_indexes_in_marabou._context()));
        to_marabou_constraint_rec(constraint, marabou_constraint_conjunction);
        return marabou_constraint_conjunction;
    } else {
        PLAJA_ASSERT(LinearConstraintsChecker::is_linear_disjunction(constraint, specification))
        std::unique_ptr<DisjunctionInMarabou> marabou_constraint_disjunction(new DisjunctionInMarabou(state_indexes_in_marabou._context()));
        to_marabou_constraint_rec(constraint, marabou_constraint_disjunction);
        return marabou_constraint_disjunction;

    }

}

std::unique_ptr<MarabouConstraint> ToMarabouConstraint::to_assignment_constraint(MarabouVarIndex_type target_var, const Expression& assignment_sum, const StateIndexesInMarabou& state_indexes_in_marabou) {

    const LINEAR_CONSTRAINTS_CHECKER::Specification specification;

    PLAJA_ASSERT(LinearConstraintsChecker::is_linear_assignment(assignment_sum, specification))
    MARABOU_IN_PLAJA::stateIndexesToMarabou = &state_indexes_in_marabou;

    // special case bool:
    if (LinearConstraintsChecker::is_literal(assignment_sum, specification)) {
        // extract literal:
        PreEquationConBool pre_equation_aux(state_indexes_in_marabou._context());
        literal_to_marabou(assignment_sum, pre_equation_aux);
        PLAJA_ASSERT(pre_equation_aux.addends.size() <= 1)

        PreEquation pre_equation(state_indexes_in_marabou._context(), Equation::EquationType::EQ);

        if (pre_equation_aux.addends.empty()) {
            pre_equation.addends[target_var] = 1;
            pre_equation.scalar = pre_equation_aux.isTrivial ? 0 : 1; // trivial conjunction is unsat, i.e., we added false
        } else {
            auto addend_it = pre_equation_aux.addends.begin();

            if (addend_it->second == MARABOU_IN_PLAJA::False) { // negative literal ..
                pre_equation.addends[addend_it->first] = 1; // note: a := !b becomes a + b = 1
                pre_equation.addends[target_var] = 1;
                pre_equation.scalar = 1;
            } else { // positive literal ..
                pre_equation.addends[addend_it->first] = -1; // note: a := b becomes a - b = 0
                pre_equation.addends[target_var] = 1;
                pre_equation.scalar = 0;
            }
        }

        return pre_equation.to_marabou_constraint();
    }

    // else: // normal linear sum:
    PLAJA_ASSERT(LinearConstraintsChecker::is_linear_sum(assignment_sum, specification))
    PreEquation pre_equation(state_indexes_in_marabou._context(), Equation::EquationType::EQ);
    linear_sum_to_marabou(assignment_sum, pre_equation, 1);
    pre_equation.addends[target_var] = -1; // note: y := a*x + c becomes -c = a*x - y
    return pre_equation.to_marabou_constraint();

}

#endif