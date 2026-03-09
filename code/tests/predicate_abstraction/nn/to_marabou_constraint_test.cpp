//
// This file is part of the PlaJA code base.
// Copyright (c) (2019 - 2020) Marcel Vinzent.
// See README.md in the top-level directory for licensing information.
//

#ifndef PLAJA_TESTS_TO_MARABOU_CONSTRAINT_TEST_CPP
#define PLAJA_TESTS_TO_MARABOU_CONSTRAINT_TEST_CPP

#include "../../utils/test_header.h"
#include <memory>
#include "../../../option_parser/option_parser.h"
#include "../../../parser/ast/expression/expression.h"
#include "../../../parser/ast/variable_declaration.h"
#include "../../../parser/visitor/linear_constraints_checker.h"
#include "../../../parser/visitor/semantics_checker.h"
#include "../../../parser/visitor/set_variable_index.h"
#include "../../../parser/parser.h"
#include "../../../search/smt_nn/visitor/extern/to_marabou_visitor.h"
#include "../../../search/smt_nn/constraints_in_marabou.h"
#include "../../../search/smt_nn/marabou_context.h"
#include "../../../search/smt_nn/vars_in_marabou.h"

class ToMarabouConstraintTest: public PLAJA_TEST::TestSuite {

private:
    PLAJA::OptionParser optionParser;
    Parser parser;
    // JANI structures
    std::list<std::unique_ptr<VariableDeclaration>> vars;
    std::unique_ptr<Expression> racetrackGuard;
    std::unique_ptr<Expression> racetrackAssignment;
    std::unique_ptr<Expression> racetrackAssignmentScaled;
    std::unique_ptr<Expression> booleanScalar;

    // Marabou structures
    MARABOU_IN_PLAJA::Context context;
    StateIndexesInMarabou stateIndexesToMarabou; // To map state variable indexes in marabou.

public:

    ToMarabouConstraintTest():
        parser(optionParser)
        , stateIndexesToMarabou(context) {
        vars.push_back(parser.parse_var_decl_str(R"({"name": "car0_dx", "type": "int"})", 5)); // Just for completeness, we set the index in the expression explicitly via SetVarIndexByID.
        vars.push_back(parser.parse_var_decl_str(R"({"name": "car0_crashed", "type": "bool"})", 6));
        std::vector<VariableIndex_type> id_to_index { 5, 6 };

        // std::unique_ptr<VariableDeclaration> var_decl2 = parser.parse_VarDeclStr(R"({"name": "A", "type": {"kind":"array", "base":"int"}})");

        // create manual JANI expressions
        // SemanticsChecker to determine type as used subsequently,
        // Note, determine_type call to parent expression recursively calls children only if necessary (i.e., if type cannot be determined without child type assuming a valid instance)

        // "exp": { "exp": "car0_crashed", "op": "¬" }
        racetrackGuard = parser.parse_ExpStr(R"({"exp":"car0_crashed","op": "¬"})");
        SET_VARS::set_var_index_by_id(id_to_index, *racetrackGuard);
        SemanticsChecker::check_semantics(racetrackGuard.get());

        // { "ref": "car0_dx", "value": { "left": "car0_dx", "op": "+", "right": 1 } }
        racetrackAssignment = parser.parse_ExpStr(R"({"left":"car0_dx","op":"+","right":1})");
        SET_VARS::set_var_index_by_id(id_to_index, *racetrackAssignment);
        SemanticsChecker::check_semantics(racetrackAssignment.get());

        // { "ref": "car0_dx", "value": { "op":"*", "left":{ "left": {"op":"*", "left":3, "right":"car0_dx"}, "op": "+", "right": 1 }, "right":-3 }}
        racetrackAssignmentScaled = parser.parse_ExpStr(R"({ "op":"*", "left":{"left": {"op":"*", "left":3, "right":"car0_dx"}, "op": "+", "right":1}, "right":-3 })");
        SET_VARS::set_var_index_by_id(id_to_index, *racetrackAssignmentScaled);
        SemanticsChecker::check_semantics(racetrackAssignmentScaled.get());

        booleanScalar = parser.parse_ExpStr(R"(true)");

        // Marabou
        stateIndexesToMarabou.add(5, FloatUtils::negativeInfinity(), FloatUtils::infinity(), true, true, false);
        stateIndexesToMarabou.add(6, FloatUtils::negativeInfinity(), FloatUtils::infinity(), true, true, false);

    }

    ~ToMarabouConstraintTest() override = default;

/**********************************************************************************************************************/

    void testLinearConstraintCheckerPositive() {
        std::cout << std::endl << "Simple positive test on linear restrictions checker:" << std::endl;
        TS_ASSERT(LinearConstraintsChecker::is_linear(*racetrackGuard))
        std::cout << racetrackGuard->to_string() << std::endl;
        TS_ASSERT(LinearConstraintsChecker::is_linear_assignment(*racetrackAssignment))
        std::cout << racetrackAssignment->to_string() << std::endl;
        TS_ASSERT(LinearConstraintsChecker::is_linear_assignment(*racetrackAssignmentScaled))
        std::cout << racetrackAssignmentScaled->to_string() << std::endl;
        // but:
        TS_ASSERT(not LinearConstraintsChecker::is_linear(*racetrackAssignment))
        TS_ASSERT(not LinearConstraintsChecker::is_linear(*racetrackAssignmentScaled))
        // but also:
        TS_ASSERT(LinearConstraintsChecker::is_linear_assignment(*racetrackGuard))
        TS_ASSERT(LinearConstraintsChecker::is_linear_assignment(*booleanScalar))
        TS_ASSERT(LinearConstraintsChecker::is_linear(*booleanScalar))
    }

    void testLinearConstraintCheckerNegative() {
        std::cout << std::endl << "Simple negative test on linear restrictions checker:" << std::endl;
        TS_ASSERT(not LinearConstraintsChecker::is_linear(*parser.parse_ExpStr(R"(1)")))
        // but:
        TS_ASSERT(LinearConstraintsChecker::is_linear_sum(*parser.parse_ExpStr(R"(1)")))
        TS_ASSERT(LinearConstraintsChecker::is_linear_assignment(*parser.parse_ExpStr(R"(1)")))
    }

    void testToMarabouGuard() {
        std::cout << std::endl << "Test to Marabou bound constraint on bool guard:" << std::endl;
        const auto guard_constraint = MARABOU_IN_PLAJA::to_marabou_constraint(*racetrackGuard, stateIndexesToMarabou);

        // inequality
        const auto* guard_constraint_casted = PLAJA_UTILS::cast_ptr_if<BoundConstraint>(guard_constraint.get());
        TS_ASSERT(guard_constraint_casted)
        TS_ASSERT_EQUALS(guard_constraint_casted->get_var(), stateIndexesToMarabou.to_marabou(6))
        TS_ASSERT_EQUALS(guard_constraint_casted->get_value(), 0)
        TS_ASSERT_EQUALS(guard_constraint_casted->get_type(), Tightening::UB)
    }

    void testToMarabouAssignment() {
        std::cout << std::endl << "Test to Marabou equation constraint on assigment:" << std::endl;

        auto target_var = context.add_var(); // add target var
        auto assignment_constraint = MARABOU_IN_PLAJA::to_assignment_constraint(target_var, *racetrackAssignment, stateIndexesToMarabou);

        // racetrack assignment
        const auto* assignment_constraint_casted = PLAJA_UTILS::cast_ptr_if<EquationConstraint>(assignment_constraint.get());
        TS_ASSERT(assignment_constraint_casted)
        TS_ASSERT_EQUALS(assignment_constraint_casted->get_equation()._type, Equation::EQ)
        TS_ASSERT_EQUALS(assignment_constraint_casted->get_equation()._scalar, -1)
        TS_ASSERT_EQUALS(assignment_constraint_casted->get_equation().getCoefficient(stateIndexesToMarabou.to_marabou(5)), 1)
        TS_ASSERT_EQUALS(assignment_constraint_casted->get_equation().getCoefficient(target_var), -1)

        // literal
        auto literal_assignment = MARABOU_IN_PLAJA::to_assignment_constraint(target_var, *racetrackGuard, stateIndexesToMarabou);

        const auto* literal_assignment_casted = PLAJA_UTILS::cast_ptr_if<EquationConstraint>(literal_assignment.get());
        TS_ASSERT(assignment_constraint_casted)
        TS_ASSERT_EQUALS(literal_assignment_casted->get_equation()._type, Equation::EQ)
        TS_ASSERT_EQUALS(literal_assignment_casted->get_equation()._scalar, 1)
        TS_ASSERT_EQUALS(literal_assignment_casted->get_equation().getCoefficient(stateIndexesToMarabou.to_marabou(6)), 1)
        TS_ASSERT_EQUALS(literal_assignment_casted->get_equation().getCoefficient(target_var), 1)

        // boolean scalar
        auto boolean_scalar_assignment = MARABOU_IN_PLAJA::to_assignment_constraint(target_var, *booleanScalar, stateIndexesToMarabou);

        const auto* boolean_scalar_assignment_casted = PLAJA_UTILS::cast_ptr_if<ConjunctionInMarabou>(boolean_scalar_assignment.get());
        TS_ASSERT(boolean_scalar_assignment_casted)
        TS_ASSERT(boolean_scalar_assignment_casted->equations.empty())
        TS_ASSERT(boolean_scalar_assignment_casted->bounds.size() == 2)
        TS_ASSERT(boolean_scalar_assignment_casted->disjunctions.empty())
        TS_ASSERT_EQUALS(boolean_scalar_assignment_casted->bounds.front()._variable, target_var)
        TS_ASSERT_EQUALS(boolean_scalar_assignment_casted->bounds.back()._variable, target_var)
        TS_ASSERT_EQUALS(boolean_scalar_assignment_casted->bounds.front()._value, true)
        TS_ASSERT_EQUALS(boolean_scalar_assignment_casted->bounds.back()._value, true)
        TS_ASSERT_EQUALS(boolean_scalar_assignment_casted->bounds.front()._type, Tightening::LB)
        TS_ASSERT_EQUALS(boolean_scalar_assignment_casted->bounds.back()._type, Tightening::UB)
    }

    void testToMarabouAssignmentScaled() {
        std::cout << std::endl << "Test to Marabou equation constraint on scaled assigment:" << std::endl;

        auto target_var = context.add_var(); // add target var
        auto assignment_constraint_scaled = MARABOU_IN_PLAJA::to_assignment_constraint(target_var, *racetrackAssignmentScaled, stateIndexesToMarabou);

        // racetrack scaled assignment
        const auto* assignment_constraint_casted = PLAJA_UTILS::cast_ptr_if<EquationConstraint>(assignment_constraint_scaled.get());
        TS_ASSERT(assignment_constraint_casted)
        TS_ASSERT_EQUALS(assignment_constraint_casted->get_equation()._type, Equation::EQ)
        TS_ASSERT_EQUALS(assignment_constraint_casted->get_equation()._scalar, 3)
        TS_ASSERT_EQUALS(assignment_constraint_casted->get_equation().getCoefficient(stateIndexesToMarabou.to_marabou(5)), -9)
        TS_ASSERT_EQUALS(assignment_constraint_casted->get_equation().getCoefficient(target_var), -1)
    }

/**********************************************************************************************************************/

    void run_tests() override {
        RUN_TEST(testLinearConstraintCheckerPositive())
        RUN_TEST(testLinearConstraintCheckerNegative())
        RUN_TEST(testToMarabouGuard())
        RUN_TEST(testToMarabouAssignment())
        RUN_TEST(testToMarabouAssignmentScaled())
    }

};

TEST_MAIN(ToMarabouConstraintTest)

#endif //PLAJA_TESTS_TO_MARABOU_CONSTRAINT_TEST_CPP
