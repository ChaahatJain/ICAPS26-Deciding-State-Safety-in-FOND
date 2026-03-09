//
// This file is part of the PlaJA code base (2019 - 2020).
// TODO: Check if these are needed. Ideally, can just use jani operators.
//

#include <memory>
#include "to_veritas_constraint.h"
#include "../../../utils/floating_utils.h"
#include "../../../parser/ast/expression/binary_op_expression.h"
#include "../../../parser/ast/expression/unary_op_expression.h"
#include "../../../parser/ast/expression/variable_expression.h"
#include "../../../parser/ast/type/int_type.h"
#include "../../../parser/visitor/linear_constraints_checker.h"
#include "../constraints_in_veritas.h"
#include "../veritas_context.h"
#include "../vars_in_veritas.h"

// auxiliary:

namespace VERITAS_IN_PLAJA {
    constexpr VeritasInteger_type False = -1;
    constexpr VeritasInteger_type True = 1;
}

namespace TO_VERITAS_VISITOR { extern VERITAS_IN_PLAJA::Equation::EquationType invert_op(VERITAS_IN_PLAJA::Equation::EquationType op); }

/** Veritas operator type and offset (e.g., for strict op).*/
std::pair<VERITAS_IN_PLAJA::Equation::EquationType, VeritasFloating_type> jani_op_to_veritas(BinaryOpExpression::BinaryOp op) {
    switch (op) {
        case BinaryOpExpression::EQ: return { VERITAS_IN_PLAJA::Equation::EquationType::EQ, 0 };
        case BinaryOpExpression::LE: return { VERITAS_IN_PLAJA::Equation::EquationType::LE, 0 };
        case BinaryOpExpression::GE: return { VERITAS_IN_PLAJA::Equation::EquationType::GE, 0 };
        case BinaryOpExpression::LT: return { VERITAS_IN_PLAJA::Equation::EquationType::LE, -VERITAS_IN_PLAJA::strictnessPrecision };
        case BinaryOpExpression::GT: return { VERITAS_IN_PLAJA::Equation::EquationType::GE, VERITAS_IN_PLAJA::strictnessPrecision };
        default: PLAJA_ABORT
    }
}

struct PreEquation { // auxiliary class to detect and merge duplicate addends *before* adding to Veritas's equation as they seem to trouble Veritas
    const StateIndexesInVeritas* stateIndexesInVeritas;
    std::unordered_map<VeritasVarIndex_type, VeritasFloating_type> addends;
    VeritasFloating_type scalar;
    VERITAS_IN_PLAJA::Equation::EquationType type;

    explicit PreEquation(const StateIndexesInVeritas& state_indexes_in_veritas, VERITAS_IN_PLAJA::Equation::EquationType type_, VeritasFloating_type init_scalar = 0):
        stateIndexesInVeritas(&state_indexes_in_veritas)
        , scalar(init_scalar)
        , type(type_) {}

    virtual ~PreEquation() = default;

    DEFAULT_CONSTRUCTOR_ONLY(PreEquation)
    DELETE_ASSIGNMENT(PreEquation)

    [[nodiscard]] VERITAS_IN_PLAJA::Context& _context() const { return stateIndexesInVeritas->_context(); }

    VERITAS_IN_PLAJA::Equation to_equation() {
        VERITAS_IN_PLAJA::Equation equation(type);
        equation.setScalar(scalar);
        for (auto& addend: addends) {
            PLAJA_ASSERT(addend.second != 0)
            equation.addAddend(addend.second, addend.first); // note equations takes coefficient (second) then variable index (first)
        }
        return equation;
    }

    virtual void add_scalar(VeritasFloating_type scalar_additive, VeritasFloating_type factor) { scalar += factor * scalar_additive; } // factor is, e.g., minus if scalar is position right-hand in the translated expression

    void add_addend(VariableIndex_type state_index, VeritasFloating_type coefficient) {
        // factor is, e.g., minus if addend is position left-hand in the translated expression
        PLAJA_ASSERT(stateIndexesInVeritas->exists(state_index))
        const VeritasVarIndex_type var_index = stateIndexesInVeritas->to_veritas(state_index);
        if (addends.count(var_index)) { addends[var_index] += coefficient; }
        else { addends.insert(std::make_pair(var_index, coefficient)); }
    }

    virtual std::unique_ptr<VeritasConstraint> to_veritas_constraint() {
        PLAJA_ASSERT(not addends.empty())

        if (addends.size() == 1) {

            auto addends_it = addends.begin();
            // special case bound constraints:
            auto var = addends_it->first;
            auto factor = addends_it->second;
            PLAJA_ASSERT(factor != 0)

            if (_context().is_marked_integer(var)) { // special handling for integer vars

                auto factored_scalar = scalar / factor;
                auto factored_type = factor < 0 ? TO_VERITAS_VISITOR::invert_op(type) : type;
                switch (factored_type) {
                    case VERITAS_IN_PLAJA::Equation::EQ: {

                        if (not PLAJA_FLOATS::is_integer(factored_scalar, VERITAS_IN_PLAJA::integerPrecision)) {

                            if (VERITAS_IN_PLAJA::negative_infinity < (_context().get_lower_bound(var)) and (_context().get_lower_bound(var)) < VERITAS_IN_PLAJA::infinity) { // must use Veritas-based infinity check to detect Veritas-based infinity

                                return std::make_unique<VERITAS_IN_PLAJA::BoundConstraint>(_context(), var, VERITAS_IN_PLAJA::Tightening::UB, _context().get_lower_bound(var) - 1); // UNSAT

                            } else if (VERITAS_IN_PLAJA::negative_infinity < (_context().get_upper_bound(var)) and (_context().get_upper_bound(var)) < VERITAS_IN_PLAJA::infinity) {

                                return std::make_unique<VERITAS_IN_PLAJA::BoundConstraint>(_context(), var, VERITAS_IN_PLAJA::Tightening::LB, _context().get_upper_bound(var) + 1); // UNSAT

                            } else {

                                VERITAS_IN_PLAJA::Equation eq(VERITAS_IN_PLAJA::Equation::EQ);
                                eq.setScalar(factored_scalar);
                                eq.addAddend(1, var);
                                return std::make_unique<VERITAS_IN_PLAJA::EquationConstraint>(_context(), std::move(eq));

                            }

                        } else {
                            return VERITAS_IN_PLAJA::BoundConstraint::construct_equality_constraint(_context(), var, factored_scalar);
                        }

                    }
                    case VERITAS_IN_PLAJA::Equation::GE: { return std::make_unique<VERITAS_IN_PLAJA::BoundConstraint>(_context(), var, VERITAS_IN_PLAJA::Tightening::LB, std::ceil(factored_scalar)); }
                    case VERITAS_IN_PLAJA::Equation::LE: { return std::make_unique<VERITAS_IN_PLAJA::BoundConstraint>(_context(), var, VERITAS_IN_PLAJA::Tightening::UB, std::floor(factored_scalar)); }
                    default: { PLAJA_ABORT }
                }

            } else { return VERITAS_IN_PLAJA::BoundConstraint::construct(_context(), var, factor < 0 ? TO_VERITAS_VISITOR::invert_op(type) : type, scalar / factor); }

        }

        // else: equation constraint
        return std::make_unique<VERITAS_IN_PLAJA::EquationConstraint>(_context(), to_equation());

    }

};

// special handling of duplicate addends necessary
struct PreEquationBool: public PreEquation {
    // Class assumption: A & B & not(C) becomes A + B + (1 - C) >= 3 becomes A + B - C >= 2
    // Class assumption: A | B | not(C) becomes A + B + (1 - C) >= 1 becomes A + B - C >= 0

    static_assert(VERITAS_IN_PLAJA::False == -1);
    static_assert(VERITAS_IN_PLAJA::True == 1);
    bool isTrivial;

    explicit PreEquationBool(const StateIndexesInVeritas& state_indexes_in_veritas):
        PreEquation(state_indexes_in_veritas, VERITAS_IN_PLAJA::Equation::EquationType::GE)
        , isTrivial(false) {}

    ~PreEquationBool() override = default;

    DEFAULT_CONSTRUCTOR_ONLY(PreEquationBool)
    DELETE_ASSIGNMENT(PreEquationBool)

    void add_scalar(VeritasFloating_type /*scalar_additive*/, VeritasFloating_type /*factor*/) override { PLAJA_ABORT }

    virtual void add_scalar(VeritasInteger_type scalar_additive) = 0;

    void add_addend(VariableIndex_type state_index, VeritasInteger_type coefficient) { // NOLINT(bugprone-easily-swappable-parameters)
        PLAJA_ASSERT(coefficient == VERITAS_IN_PLAJA::False or coefficient == VERITAS_IN_PLAJA::True)
        PLAJA_ASSERT(stateIndexesInVeritas->exists(state_index))
        const VeritasVarIndex_type var_index = stateIndexesInVeritas->to_veritas(state_index);

        if (addends.count(var_index)) {
            auto current_coefficient = addends[var_index];
            if (current_coefficient == coefficient) { return; }
            else { isTrivial = true; } // positive and negative literal -> the conjunction is false | the disjunction is true
        } else {
            addends.insert(std::make_pair(var_index, coefficient));
            if (coefficient == VERITAS_IN_PLAJA::True) { ++scalar; } // count occurrences of positive literals
        }

    }

    std::unique_ptr<VeritasConstraint> to_veritas_constraint() override = 0;

};

struct PreEquationConBool: public PreEquationBool {

    explicit PreEquationConBool(const StateIndexesInVeritas& state_indexes_in_veritas):
        PreEquationBool(state_indexes_in_veritas) {}

    ~PreEquationConBool() override = default;

    DEFAULT_CONSTRUCTOR_ONLY(PreEquationConBool)
    DELETE_ASSIGNMENT(PreEquationConBool)

    void add_scalar(VeritasInteger_type scalar_additive) override {
        PLAJA_ASSERT(scalar_additive == VERITAS_IN_PLAJA::False or scalar_additive == VERITAS_IN_PLAJA::True)
        if (scalar_additive == VERITAS_IN_PLAJA::False) { isTrivial = true; }
    }

    std::unique_ptr<VeritasConstraint> to_veritas_constraint() override {

        if (isTrivial) { // special case (unsat)

            if (addends.empty()) { return _context().trivially_false(); }
            else { return std::make_unique<VERITAS_IN_PLAJA::BoundConstraint>(_context(), addends.begin()->first, VERITAS_IN_PLAJA::Tightening::UB, -1); } // encodes unsat for bools

        } else {

            PLAJA_ASSERT(not addends.empty())
            return PreEquation::to_veritas_constraint(); // NOLINT(bugprone-parent-virtual-call)

        }

    }

};

struct PreEquationDisBool: public PreEquationBool {

    explicit PreEquationDisBool(const StateIndexesInVeritas& state_indexes_in_veritas):
        PreEquationBool(state_indexes_in_veritas) {}

    ~PreEquationDisBool() override = default;

    DEFAULT_CONSTRUCTOR_ONLY(PreEquationDisBool)
    DELETE_ASSIGNMENT(PreEquationDisBool)

    void add_scalar(VeritasInteger_type scalar_additive) override {
        PLAJA_ASSERT(scalar_additive == VERITAS_IN_PLAJA::False or scalar_additive == VERITAS_IN_PLAJA::True)
        if (scalar_additive == VERITAS_IN_PLAJA::True) { isTrivial = true; }
    }

    std::unique_ptr<VeritasConstraint> to_veritas_constraint() override {
        if (isTrivial) { return _context().trivially_true(); }
        else if (addends.empty()) { return _context().trivially_false(); } // special case (unsat)
        else {
            PLAJA_ASSERT(not addends.empty())
            return PreEquation::to_veritas_constraint(); // NOLINT(bugprone-parent-virtual-call)
        }
    }

};

/**Boolean structures**************************************************************************************************/

void literal_to_veritas(const Expression& expr, PreEquationBool& pre_equation) {
    if (LinearConstraintsChecker::is_state_variable_index(expr, LINEAR_CONSTRAINTS_CHECKER::Specification())) {
        PLAJA_ASSERT(dynamic_cast<const LValueExpression*>(&expr))
        pre_equation.add_addend(static_cast<const LValueExpression*>(&expr)->get_variable_index(), VERITAS_IN_PLAJA::True); // NOLINT(cppcoreguidelines-pro-type-static-cast-downcast)
        return;
    } else if (expr.is_constant() and expr.get_type()->is_boolean_type()) {
        pre_equation.add_scalar(expr.evaluate_integer_const() ? VERITAS_IN_PLAJA::True : VERITAS_IN_PLAJA::False);
    } else {
        PLAJA_ASSERT(dynamic_cast<const UnaryOpExpression*>(&expr))
        const auto* unary_exp = static_cast<const UnaryOpExpression*>(&expr); // NOLINT(cppcoreguidelines-pro-type-static-cast-downcast)
        PLAJA_ASSERT(unary_exp->get_op() == UnaryOpExpression::NOT)
        PLAJA_ASSERT(LinearConstraintsChecker::is_state_variable_index(*unary_exp->get_operand(), LINEAR_CONSTRAINTS_CHECKER::Specification()))
        pre_equation.add_addend(static_cast<const LValueExpression*>(unary_exp->get_operand())->get_variable_index(), VERITAS_IN_PLAJA::False); // NOLINT(cppcoreguidelines-pro-type-static-cast-downcast)
        return;
    }
}

void bool_junction_to_veritas(const Expression& expr, PreEquationBool& pre_equation) { // NOLINT(misc-no-recursion)
    if (LinearConstraintsChecker::is_literal(expr, LINEAR_CONSTRAINTS_CHECKER::Specification())) {
        literal_to_veritas(expr, pre_equation);
        return;
    }
    // else:
    const auto* binary_exp = static_cast<const BinaryOpExpression*>(&expr); // NOLINT(cppcoreguidelines-pro-type-static-cast-downcast)
    PLAJA_ASSERT(binary_exp->get_op() == BinaryOpExpression::AND or binary_exp->get_op() == BinaryOpExpression::OR)
    bool_junction_to_veritas(*binary_exp->get_left(), pre_equation);
    bool_junction_to_veritas(*binary_exp->get_right(), pre_equation);
}

/**Linear constraint***************************************************************************************************/

void addend_to_veritas(const Expression& expr, PreEquation& pre_equation, VeritasFloating_type global_factor /*as left-hand addend in Veritas*/) {
    auto specification = LINEAR_CONSTRAINTS_CHECKER::Specification::set_bools(false);

    if (LinearConstraintsChecker::is_state_variable_index(expr, specification)) {
        PLAJA_ASSERT(dynamic_cast<const LValueExpression*>(&expr))
        pre_equation.add_addend(static_cast<const LValueExpression*>(&expr)->get_variable_index(), global_factor); // NOLINT(cppcoreguidelines-pro-type-static-cast-downcast)
        return;
    } else {
        PLAJA_ASSERT(dynamic_cast<const BinaryOpExpression*>(&expr))
        const auto* binary_exp = static_cast<const BinaryOpExpression*>(&expr); // NOLINT(cppcoreguidelines-pro-type-static-cast-downcast)
        PLAJA_ASSERT(binary_exp->get_op() == BinaryOpExpression::TIMES)
        if (LinearConstraintsChecker::is_state_variable_index(*binary_exp->get_left(), specification)) { // variable to the left
            PLAJA_ASSERT(dynamic_cast<const LValueExpression*>(binary_exp->get_left()))
            PLAJA_ASSERT(LinearConstraintsChecker::is_factor(*binary_exp->get_right()))
            pre_equation.add_addend(static_cast<const LValueExpression*>(binary_exp->get_left())->get_variable_index(), binary_exp->get_right()->evaluate_floating_const() * global_factor); // NOLINT(cppcoreguidelines-pro-type-static-cast-downcast)
            return;
        }
        if (LinearConstraintsChecker::is_state_variable_index(*binary_exp->get_right(), specification)) { // variable to the right
            PLAJA_ASSERT(dynamic_cast<const LValueExpression*>(binary_exp->get_right()))
            PLAJA_ASSERT(LinearConstraintsChecker::is_factor(*binary_exp->get_left()))
            pre_equation.add_addend(static_cast<const LValueExpression*>(binary_exp->get_right())->get_variable_index(), binary_exp->get_left()->evaluate_floating_const() * global_factor); // NOLINT(cppcoreguidelines-pro-type-static-cast-downcast)
            return;
        }
    }
    PLAJA_ABORT
}

void linear_sum_to_veritas(const Expression& expr, PreEquation& pre_equation, VeritasFloating_type global_factor /*as left-hand sum in Veritas*/) { // NOLINT(misc-no-recursion)
    if (LinearConstraintsChecker::is_addend(expr, LINEAR_CONSTRAINTS_CHECKER::Specification())) {
        addend_to_veritas(expr, pre_equation, global_factor);
        return;
    }
    if (LinearConstraintsChecker::is_scalar(expr)) {
        pre_equation.add_scalar(expr.evaluate_floating_const(), -1 * global_factor);
        return;
    }
    // else:
    const auto* binary_exp = static_cast<const BinaryOpExpression*>(&expr); // NOLINT(cppcoreguidelines-pro-type-static-cast-downcast)

    switch (binary_exp->get_op()) {
        case BinaryOpExpression::PLUS: {
            linear_sum_to_veritas(*binary_exp->get_left(), pre_equation, global_factor);
            linear_sum_to_veritas(*binary_exp->get_right(), pre_equation, global_factor);
            return;
        }
        case BinaryOpExpression::TIMES: {
            if (LinearConstraintsChecker::is_factor(*binary_exp->get_left())) {
                linear_sum_to_veritas(*binary_exp->get_right(), pre_equation, binary_exp->get_left()->evaluate_floating_const() * global_factor);
            } else {
                PLAJA_ASSERT(LinearConstraintsChecker::is_factor(*binary_exp->get_right()))
                linear_sum_to_veritas(*binary_exp->get_left(), pre_equation, binary_exp->get_right()->evaluate_floating_const() * global_factor);
            }
            return;
        }
        default: PLAJA_ABORT
    }
}

/**********************************************************************************************************************/

std::unique_ptr<VeritasConstraint> TO_VERITAS_CONSTRAINT::to_veritas_linear_constraint(const Expression& expr, const StateIndexesInVeritas& state_indexes_in_veritas) {
    PLAJA_ASSERT(LinearConstraintsChecker::is_linear_constraint(expr))

    auto* binary_exp = dynamic_cast<const BinaryOpExpression*>(&expr);
    PLAJA_ASSERT(binary_exp)
    const auto op_offset = jani_op_to_veritas(binary_exp->get_op());
    PreEquation pre_equation(state_indexes_in_veritas, op_offset.first, op_offset.second);
    linear_sum_to_veritas(*binary_exp->get_left(), pre_equation, 1);
    linear_sum_to_veritas(*binary_exp->get_right(), pre_equation, -1);
    return pre_equation.to_veritas_constraint();
}

std::unique_ptr<VeritasConstraint> TO_VERITAS_CONSTRAINT::to_veritas_conjunction_bool(const Expression& expr, const StateIndexesInVeritas& state_indexes_in_veritas) {
    PLAJA_ASSERT(LinearConstraintsChecker::is_conjunction_bool(expr))
    //
    PreEquationConBool pre_equation(state_indexes_in_veritas);
    bool_junction_to_veritas(expr, pre_equation);
    return pre_equation.to_veritas_constraint();
}

std::unique_ptr<VeritasConstraint> TO_VERITAS_CONSTRAINT::to_veritas_disjunction_bool(const Expression& expr, const StateIndexesInVeritas& state_indexes_in_veritas) {
    PLAJA_ASSERT(LinearConstraintsChecker::is_disjunction_bool(expr))
    //
    PreEquationDisBool pre_equation(state_indexes_in_veritas);
    bool_junction_to_veritas(expr, pre_equation);
    return pre_equation.to_veritas_constraint();
}

std::unique_ptr<VeritasConstraint> TO_VERITAS_CONSTRAINT::to_veritas_linear_sum(VeritasVarIndex_type target_var, VERITAS_IN_PLAJA::Equation::EquationType op, const Expression& assignment_sum, const StateIndexesInVeritas& state_indexes_in_veritas) {
    PLAJA_ASSERT(LinearConstraintsChecker::is_linear_sum(assignment_sum))
    PreEquation pre_equation(state_indexes_in_veritas, TO_VERITAS_VISITOR::invert_op(op));
    linear_sum_to_veritas(assignment_sum, pre_equation, 1);
    pre_equation.addends[target_var] = -1; // note: y [op] a*x + c becomes -c [invert(op)] a*x - y
    return pre_equation.to_veritas_constraint();
}

/**********************************************************************************************************************/

#if 0

void to_veritas_constraint_rec(const Expression& expr, std::unique_ptr<ConjunctionInVeritas>& veritas_constraint_conjunction) { // NOLINT(misc-no-recursion)

    const LINEAR_CONSTRAINTS_CHECKER::Specification specification;

    if (LinearConstraintsChecker::is_conjunction_bool(expr, specification)) {
        PreEquationConBool pre_equation(veritas_constraint_conjunction->_context());
        bool_junction_to_veritas(expr, pre_equation);
        pre_equation.to_veritas_constraint()->move_to_conjunction(*veritas_constraint_conjunction);
        return;
    }

    if (LinearConstraintsChecker::is_disjunction_bool(expr, specification)) {
        PreEquationDisBool pre_equation(veritas_constraint_conjunction->_context());
        bool_junction_to_veritas(expr, pre_equation);
        pre_equation.to_veritas_constraint()->move_to_conjunction(*veritas_constraint_conjunction);
        return;
    }

    if (LinearConstraintsChecker::is_linear_constraint(expr, specification)) {
        to_veritas_linear_constraint(veritas_constraint_conjunction->_context(), expr)->move_to_conjunction(*veritas_constraint_conjunction);
        return;
    }

    // else:
    auto* binary_exp = dynamic_cast<const BinaryOpExpression*>(&expr);
    PLAJA_ASSERT(binary_exp)
    PLAJA_ASSERT(binary_exp->get_op() == BinaryOpExpression::AND)
    to_veritas_constraint_rec(*binary_exp->get_left(), veritas_constraint_conjunction);
    to_veritas_constraint_rec(*binary_exp->get_right(), veritas_constraint_conjunction);

}

void to_veritas_constraint_rec(const Expression& expr, std::unique_ptr<DisjunctionInVeritas>& veritas_constraint_disjunction) { // NOLINT(misc-no-recursion)

    const LINEAR_CONSTRAINTS_CHECKER::Specification specification;

    if (LinearConstraintsChecker::is_conjunction_bool(expr, specification)) {
        PreEquationConBool pre_equation(veritas_constraint_disjunction->_context());
        bool_junction_to_veritas(expr, pre_equation);
        pre_equation.to_veritas_constraint()->move_to_disjunction(*veritas_constraint_disjunction);
        return;
    }

    if (LinearConstraintsChecker::is_disjunction_bool(expr, specification)) {
        PreEquationDisBool pre_equation(veritas_constraint_disjunction->_context());
        bool_junction_to_veritas(expr, pre_equation);
        pre_equation.to_veritas_constraint()->move_to_disjunction(*veritas_constraint_disjunction);
        return;
    }

    if (LinearConstraintsChecker::is_linear_constraint(expr, specification)) {
        to_veritas_linear_constraint(veritas_constraint_disjunction->_context(), expr)->move_to_disjunction(*veritas_constraint_disjunction);
        return;
    }

    // else:
    auto* binary_exp = dynamic_cast<const BinaryOpExpression*>(&expr);
    PLAJA_ASSERT(binary_exp)
    PLAJA_ASSERT(binary_exp->get_op() == BinaryOpExpression::OR)
    to_veritas_constraint_rec(*binary_exp->get_left(), veritas_constraint_disjunction);
    to_veritas_constraint_rec(*binary_exp->get_right(), veritas_constraint_disjunction);

}

/**********************************************************************************************************************/

std::unique_ptr<VeritasConstraint> ToVeritasConstraint::to_veritas_constraint(const Expression& constraint, const StateIndexesInVeritas& state_indexes_in_veritas) {

    const LINEAR_CONSTRAINTS_CHECKER::Specification specification;

    PLAJA_ASSERT(LinearConstraintsChecker::is_linear(constraint, specification))
    VERITAS_IN_PLAJA::stateIndexesToVeritas = &state_indexes_in_veritas;

    if (LinearConstraintsChecker::is_conjunction_bool(constraint, specification)) {
        PreEquationConBool pre_equation(state_indexes_in_veritas._context());
        bool_junction_to_veritas(constraint, pre_equation);
        return pre_equation.to_veritas_constraint();
    }

    if (LinearConstraintsChecker::is_disjunction_bool(constraint, specification)) {
        PreEquationDisBool pre_equation(state_indexes_in_veritas._context());
        bool_junction_to_veritas(constraint, pre_equation);
        return pre_equation.to_veritas_constraint();
    }

    if (LinearConstraintsChecker::is_linear_constraint(constraint, specification)) { return to_veritas_linear_constraint(state_indexes_in_veritas._context(), constraint); }

    // else:
    if (LinearConstraintsChecker::is_linear_conjunction(constraint, specification)) {
        std::unique_ptr<ConjunctionInVeritas> veritas_constraint_conjunction(new ConjunctionInVeritas(state_indexes_in_veritas._context()));
        to_veritas_constraint_rec(constraint, veritas_constraint_conjunction);
        return veritas_constraint_conjunction;
    } else {
        PLAJA_ASSERT(LinearConstraintsChecker::is_linear_disjunction(constraint, specification))
        std::unique_ptr<DisjunctionInVeritas> veritas_constraint_disjunction(new DisjunctionInVeritas(state_indexes_in_veritas._context()));
        to_veritas_constraint_rec(constraint, veritas_constraint_disjunction);
        return veritas_constraint_disjunction;

    }

}

std::unique_ptr<VeritasConstraint> ToVeritasConstraint::to_assignment_constraint(VeritasVarIndex_type target_var, const Expression& assignment_sum, const StateIndexesInVeritas& state_indexes_in_veritas) {

    const LINEAR_CONSTRAINTS_CHECKER::Specification specification;

    PLAJA_ASSERT(LinearConstraintsChecker::is_linear_assignment(assignment_sum, specification))
    VERITAS_IN_PLAJA::stateIndexesToVeritas = &state_indexes_in_veritas;

    // special case bool:
    if (LinearConstraintsChecker::is_literal(assignment_sum, specification)) {
        // extract literal:
        PreEquationConBool pre_equation_aux(state_indexes_in_veritas._context());
        literal_to_veritas(assignment_sum, pre_equation_aux);
        PLAJA_ASSERT(pre_equation_aux.addends.size() <= 1)

        PreEquation pre_equation(state_indexes_in_veritas._context(), Equation::EquationType::EQ);

        if (pre_equation_aux.addends.empty()) {
            pre_equation.addends[target_var] = 1;
            pre_equation.scalar = pre_equation_aux.isTrivial ? 0 : 1; // trivial conjunction is unsat, i.e., we added false
        } else {
            auto addend_it = pre_equation_aux.addends.begin();

            if (addend_it->second == VERITAS_IN_PLAJA::False) { // negative literal ..
                pre_equation.addends[addend_it->first] = 1; // note: a := !b becomes a + b = 1
                pre_equation.addends[target_var] = 1;
                pre_equation.scalar = 1;
            } else { // positive literal ..
                pre_equation.addends[addend_it->first] = -1; // note: a := b becomes a - b = 0
                pre_equation.addends[target_var] = 1;
                pre_equation.scalar = 0;
            }
        }

        return pre_equation.to_veritas_constraint();
    }

    // else: // normal linear sum:
    PLAJA_ASSERT(LinearConstraintsChecker::is_linear_sum(assignment_sum, specification))
    PreEquation pre_equation(state_indexes_in_veritas._context(), Equation::EquationType::EQ);
    linear_sum_to_veritas(assignment_sum, pre_equation, 1);
    pre_equation.addends[target_var] = -1; // note: y := a*x + c becomes -c = a*x - y
    return pre_equation.to_veritas_constraint();

}

#endif