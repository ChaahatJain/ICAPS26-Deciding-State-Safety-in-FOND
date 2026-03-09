//
// This file is part of the PlaJA code base.
// Copyright (c) (2019 - 2020) Marcel Vinzent.
// See README.md in the top-level directory for licensing information.
//

#ifndef PLAJA_TESTS_TO_MARABOU_EQUATION_PREDICATE_TEST_CPP
#define PLAJA_TESTS_TO_MARABOU_EQUATION_PREDICATE_TEST_CPP

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

class ToMarabouEquationPredicateTest: public PLAJA_TEST::TestSuite {

private:
    PLAJA::OptionParser optionParser;
    Parser parser;
    // JANI structures
    std::list<std::unique_ptr<VariableDeclaration>> vars;
    std::unique_ptr<Expression> inequality;
    std::unique_ptr<Expression> inequalityDuplicates;
    std::unique_ptr<Expression> inequalityComplex;
    std::unique_ptr<Expression> inequalityScaled;
    std::unique_ptr<Expression> inequalityBool;
    std::unique_ptr<Expression> inequalityBoolUnsat;

    // Marabou structures
    MARABOU_IN_PLAJA::Context context;
    // to map state variable indexes in marabou
    StateIndexesInMarabou stateIndexesToMarabou;

    // void setUp () override {}

public:

    ToMarabouEquationPredicateTest():
        parser(optionParser)
        , stateIndexesToMarabou(context) {
        vars.push_back(parser.parse_var_decl_str(R"({"name": "x", "type": "int"})", 1)); // Just for completeness, we set the index in the expression explicitly via SetVarIndexByID.
        vars.push_back(parser.parse_var_decl_str(R"({"name": "A", "type": {"kind":"array", "base":"int"}})", 2));
        vars.push_back(parser.parse_var_decl_str(R"({"name": "B", "type": {"kind":"array", "base":"bool"}})", 6));
        vars.push_back(parser.parse_var_decl_str(R"({"name": "w", "type": "bool"})", 7));
        vars.push_back(parser.parse_var_decl_str(R"({"name": "y", "type": "bool"})", 8));
        std::vector<VariableIndex_type> id_to_index { 1, 2, 6, 7, 8 };

        // create manual JANI expressions
        // SemanticsChecker to determine type as used subsequently,
        // Note, determine_type call to parent expression recursively calls children only if necessary (i.e., if type cannot be determined without child type assuming a valid instance)

        // (5*x) + (-1 * A[1]) >= 0
        inequality = parser.parse_ExpStr(R"({"left":{"left":{"left":5,"op":"*","right":"x"},"op":"+","right":{"left":-1,"op":"*","right":{"exp":"A","index":1,"op":"aa"}}},"op":"≥","right":0})");
        SET_VARS::set_var_index_by_id(id_to_index, *inequality);
        SemanticsChecker::check_semantics(inequality.get());

        // (5*x) + (-1 * x) + 2 <= -5
        inequalityDuplicates = parser.parse_ExpStr(R"({"left":{"left":{"left":5,"op":"*","right":"x"},"op":"+","right":{"left":{"left":-1,"op":"*","right":"x"},"op":"+","right":2}},"op":"≤","right":-5})");
        SET_VARS::set_var_index_by_id(id_to_index, *inequalityDuplicates);
        SemanticsChecker::check_semantics(inequalityDuplicates.get());

        // -2 >= (5*x) + (-1 * A[1]) + (-20*A[3])
        inequalityComplex = parser.parse_ExpStr(R"({"left":-2,"op":"≥","right":{"left":{"left":{"left":5,"op":"*","right":"x"},"op":"+","right":{"left":-1,"op":"*","right":{"exp":"A","index":1,"op":"aa"}}},"op":"+","right":{"left":-20,"op":"*","right":{"exp":"A","index":3,"op":"aa"}}}})");
        SET_VARS::set_var_index_by_id(id_to_index, *inequalityComplex);
        SemanticsChecker::check_semantics(inequalityComplex.get());

        // -2 + x <= 2 * ( (5*x) + (-1 * A[1]) + 4 + (-2*A[1]) )
        inequalityScaled = parser.parse_ExpStr(R"({"op":"≤", "left":{"op":"+", "left":-2, "right":"x"}, "right":{ "op":"*", "left":2, "right":{"op":"+", "left":{"op":"+", "left":{"left":5,"op":"*","right":"x"}, "right":{"left":-1,"op":"*","right":{"exp":"A","index":1,"op":"aa"}}}, "right":{"op":"+", "left":4, "right":{"left":-2,"op":"*","right":{"exp":"A","index":1,"op":"aa"}}}} } })");
        SET_VARS::set_var_index_by_id(id_to_index, *inequalityScaled);
        SemanticsChecker::check_semantics(inequalityScaled.get());

        // (y && (!w && !B[1])) && y
        inequalityBool = parser.parse_ExpStr(R"({"left":{"left":"y","op":"∧","right":{"left":{"exp":{"exp":"B","index":0,"op":"aa"},"op":"¬"},"op":"∧","right":{"exp":"w","op":"¬"}}},"op":"∧","right":"y"})");
        SET_VARS::set_var_index_by_id(id_to_index, *inequalityBool);
        SemanticsChecker::check_semantics(inequalityBool.get());

        // (y && (!w && !B[1])) && w
        inequalityBoolUnsat = parser.parse_ExpStr(R"({"left":{"left":"y","op":"∧","right":{"left":{"exp":{"exp":"B","index":0,"op":"aa"},"op":"¬"},"op":"∧","right":{"exp":"w","op":"¬"}}},"op":"∧","right":"w"})");
        SET_VARS::set_var_index_by_id(id_to_index, *inequalityBoolUnsat);
        SemanticsChecker::check_semantics(inequalityBoolUnsat.get());

        // Marabou
        for (VariableIndex_type state_index = 0; state_index <= 8; ++state_index) { stateIndexesToMarabou.add(state_index, FloatUtils::negativeInfinity(), FloatUtils::infinity(), true, true, false); }

    }

    ~ToMarabouEquationPredicateTest() override = default;

/**********************************************************************************************************************/

    void testLinearConstraintCheckerPositive() {
        std::cout << std::endl << "Simple positive test on linear restrictions checker:" << std::endl;
        TS_ASSERT(LinearConstraintsChecker::is_linear(*inequality, LINEAR_CONSTRAINTS_CHECKER::Specification::as_predicate(true)))
        inequality->dump();
        TS_ASSERT(LinearConstraintsChecker::is_linear(*inequalityDuplicates, LINEAR_CONSTRAINTS_CHECKER::Specification::as_predicate(true)))
        inequalityDuplicates->dump();
        TS_ASSERT(LinearConstraintsChecker::is_linear(*inequalityComplex, LINEAR_CONSTRAINTS_CHECKER::Specification::as_predicate(true)))
        inequalityComplex->dump();
        TS_ASSERT(LinearConstraintsChecker::is_linear(*inequalityBool, LINEAR_CONSTRAINTS_CHECKER::Specification::as_predicate(true)))
        inequalityScaled->dump();
        TS_ASSERT(LinearConstraintsChecker::is_linear(*inequalityScaled, LINEAR_CONSTRAINTS_CHECKER::Specification::as_predicate(true)))
        inequalityBool->dump();
        TS_ASSERT(LinearConstraintsChecker::is_linear(*inequalityBoolUnsat, LINEAR_CONSTRAINTS_CHECKER::Specification::as_predicate(true)))
        inequalityBoolUnsat->dump();
    }

    void testToMarabouInequality() {
        std::cout << std::endl << "Test to Marabou equation constraint on inequality:" << std::endl;
        std::unique_ptr<PredicateConstraint> inequality_constraint = MARABOU_IN_PLAJA::to_predicate_constraint(*inequality, stateIndexesToMarabou);

        // inequality
        const auto* inequality_constraint_casted = PLAJA_UTILS::cast_ptr_if<EquationConstraint>(inequality_constraint->constraint.get());
        TS_ASSERT(inequality_constraint_casted)
        TS_ASSERT_EQUALS(inequality_constraint_casted->get_equation()._type, Equation::GE)
        TS_ASSERT_EQUALS(inequality_constraint_casted->get_equation()._scalar, 0)
        TS_ASSERT_EQUALS(inequality_constraint_casted->get_equation().getCoefficient(0), 0) // not present
        TS_ASSERT_EQUALS(inequality_constraint_casted->get_equation().getCoefficient(1), 5)
        TS_ASSERT_EQUALS(inequality_constraint_casted->get_equation().getCoefficient(2), 0) // not present
        TS_ASSERT_EQUALS(inequality_constraint_casted->get_equation().getCoefficient(3), -1)

        // inequality negation
        const auto* inequality_constraint_negation_casted = PLAJA_UTILS::cast_ptr_if<EquationConstraint>(inequality_constraint->negation.get());
        TS_ASSERT(inequality_constraint_negation_casted)
        TS_ASSERT_EQUALS(inequality_constraint_negation_casted->get_equation()._type, Equation::LE)
        TS_ASSERT_EQUALS(inequality_constraint_negation_casted->get_equation()._scalar, -1)
        TS_ASSERT_EQUALS(inequality_constraint_negation_casted->get_equation().getCoefficient(0), inequality_constraint_casted->get_equation().getCoefficient(0))
        TS_ASSERT_EQUALS(inequality_constraint_negation_casted->get_equation().getCoefficient(1), inequality_constraint_casted->get_equation().getCoefficient(1))
        TS_ASSERT_EQUALS(inequality_constraint_negation_casted->get_equation().getCoefficient(2), inequality_constraint_casted->get_equation().getCoefficient(2))
        TS_ASSERT_EQUALS(inequality_constraint_negation_casted->get_equation().getCoefficient(3), inequality_constraint_casted->get_equation().getCoefficient(3))
    }

    void testToMarabouInequalityDuplicates() {
        std::cout << std::endl << "Test to Marabou equation constraint on inequality with duplicates:" << std::endl;
        std::unique_ptr<PredicateConstraint> inequality_duplicates_constraint = MARABOU_IN_PLAJA::to_predicate_constraint(*inequalityDuplicates, stateIndexesToMarabou);

        // inequality
        const auto* inequality_duplicates_constraint_casted = PLAJA_UTILS::cast_ptr_if<BoundConstraint>(inequality_duplicates_constraint->constraint.get());
        TS_ASSERT(inequality_duplicates_constraint_casted)
        TS_ASSERT_EQUALS(inequality_duplicates_constraint_casted->get_type(), Tightening::UB)
        TS_ASSERT_EQUALS(inequality_duplicates_constraint_casted->get_value(), std::floor(-7.0 / 4.0))
        TS_ASSERT_EQUALS(inequality_duplicates_constraint_casted->get_var(), 1)

        // inequality negation
        const auto* inequality_duplicates_constraint_negation_casted = PLAJA_UTILS::cast_ptr_if<BoundConstraint>(inequality_duplicates_constraint->negation.get());
        TS_ASSERT(inequality_duplicates_constraint_negation_casted)
        TS_ASSERT_EQUALS(inequality_duplicates_constraint_negation_casted->get_type(), Tightening::LB)
        TS_ASSERT_EQUALS(inequality_duplicates_constraint_negation_casted->get_value(), -1)
        TS_ASSERT_EQUALS(inequality_duplicates_constraint_casted->get_var(), 1)
    }

    void testToMarabouInequalityComplex() {
        std::cout << std::endl << "Test to Marabou equation constraint on complex inequality:" << std::endl;
        std::unique_ptr<PredicateConstraint> inequality_complex_constraint = MARABOU_IN_PLAJA::to_predicate_constraint(*inequalityComplex, stateIndexesToMarabou);

        // inequality complex
        const auto* inequality_complex_constraint_casted = PLAJA_UTILS::cast_ptr_if<EquationConstraint>(inequality_complex_constraint->constraint.get());
        TS_ASSERT(inequality_complex_constraint_casted)
        TS_ASSERT_EQUALS(inequality_complex_constraint_casted->get_equation()._type, Equation::GE)
        TS_ASSERT_EQUALS(inequality_complex_constraint_casted->get_equation()._scalar, 2)
        TS_ASSERT_EQUALS(inequality_complex_constraint_casted->get_equation().getCoefficient(0), 0) // not present
        TS_ASSERT_EQUALS(inequality_complex_constraint_casted->get_equation().getCoefficient(1), -5)
        TS_ASSERT_EQUALS(inequality_complex_constraint_casted->get_equation().getCoefficient(2), 0) // not present
        TS_ASSERT_EQUALS(inequality_complex_constraint_casted->get_equation().getCoefficient(3), 1)
        TS_ASSERT_EQUALS(inequality_complex_constraint_casted->get_equation().getCoefficient(4), 0) // not present
        TS_ASSERT_EQUALS(inequality_complex_constraint_casted->get_equation().getCoefficient(5), 20)

        // inequality complex negation
        const auto* inequality_complex_constraint_negation_casted = PLAJA_UTILS::cast_ptr_if<EquationConstraint>(inequality_complex_constraint->negation.get());
        TS_ASSERT(inequality_complex_constraint_negation_casted)
        TS_ASSERT_EQUALS(inequality_complex_constraint_negation_casted->get_equation()._type, Equation::LE)
        TS_ASSERT_EQUALS(inequality_complex_constraint_negation_casted->get_equation()._scalar, 1)
    };

    void testToMarabouInequalityScaled() {
        std::cout << std::endl << "Test to Marabou equation constraint on scaled inequality:" << std::endl;
        std::unique_ptr<PredicateConstraint> inequality_scaled_constraint = MARABOU_IN_PLAJA::to_predicate_constraint(*inequalityScaled, stateIndexesToMarabou);

        // inequality complex
        const auto* inequality_scaled_constraint_casted = PLAJA_UTILS::cast_ptr_if<EquationConstraint>(inequality_scaled_constraint->constraint.get());
        TS_ASSERT(inequality_scaled_constraint_casted)
        TS_ASSERT_EQUALS(inequality_scaled_constraint_casted->get_equation()._type, Equation::LE)
        TS_ASSERT_EQUALS(inequality_scaled_constraint_casted->get_equation()._scalar, 10)
        TS_ASSERT_EQUALS(inequality_scaled_constraint_casted->get_equation().getCoefficient(0), 0) // not present
        TS_ASSERT_EQUALS(inequality_scaled_constraint_casted->get_equation().getCoefficient(1), -9)
        TS_ASSERT_EQUALS(inequality_scaled_constraint_casted->get_equation().getCoefficient(2), 0) // not present
        TS_ASSERT_EQUALS(inequality_scaled_constraint_casted->get_equation().getCoefficient(3), 6)
        TS_ASSERT_EQUALS(inequality_scaled_constraint_casted->get_equation().getCoefficient(4), 0) // not present
        TS_ASSERT_EQUALS(inequality_scaled_constraint_casted->get_equation().getCoefficient(5), 0)

        // inequality complex negation
        const auto* inequality_scaled_constraint_negation_casted = PLAJA_UTILS::cast_ptr_if<EquationConstraint>(inequality_scaled_constraint->negation.get());
        TS_ASSERT(inequality_scaled_constraint_negation_casted)
        TS_ASSERT_EQUALS(inequality_scaled_constraint_negation_casted->get_equation()._type, Equation::GE)
        TS_ASSERT_EQUALS(inequality_scaled_constraint_negation_casted->get_equation()._scalar, 11)
    };

    void testToMarabouInequalityBool() {
        std::cout << std::endl << "Test to Marabou equation constraint on bool inequalities, i.e., conjunctions:" << std::endl;
        std::unique_ptr<PredicateConstraint> inequality_bool_constraint = MARABOU_IN_PLAJA::to_predicate_constraint(*inequalityBool, stateIndexesToMarabou);

        // inequality bool
        const auto* inequality_bool_constraint_casted = PLAJA_UTILS::cast_ptr_if<EquationConstraint>(inequality_bool_constraint->constraint.get());
        TS_ASSERT(inequality_bool_constraint_casted)
        TS_ASSERT_EQUALS(inequality_bool_constraint_casted->get_equation()._type, Equation::GE)
        TS_ASSERT_EQUALS(inequality_bool_constraint_casted->get_equation()._scalar, 1)
        TS_ASSERT_EQUALS(inequality_bool_constraint_casted->get_equation().getCoefficient(6), -1)
        TS_ASSERT_EQUALS(inequality_bool_constraint_casted->get_equation().getCoefficient(7), -1)
        TS_ASSERT_EQUALS(inequality_bool_constraint_casted->get_equation().getCoefficient(8), 1)
    };

    void testToMarabouBoolUnsat() {
        std::cout << std::endl << "Test to Marabou equation constraint on bool unsat conjunctions:" << std::endl;
        std::unique_ptr<PredicateConstraint> inequality_bool_unsat_constraint = MARABOU_IN_PLAJA::to_predicate_constraint(*inequalityBoolUnsat, stateIndexesToMarabou);

        // inequality bool
        const auto* inequality_bool_unsat_constraint_casted = PLAJA_UTILS::cast_ptr_if<BoundConstraint>(inequality_bool_unsat_constraint->constraint.get());
        TS_ASSERT(inequality_bool_unsat_constraint_casted)
        TS_ASSERT_EQUALS(inequality_bool_unsat_constraint_casted->get_type(), Tightening::UB)
        TS_ASSERT_EQUALS(inequality_bool_unsat_constraint_casted->get_value(), -1)
        TS_ASSERT(inequality_bool_unsat_constraint_casted->get_var() == 6 || inequality_bool_unsat_constraint_casted->get_var() == 7 || inequality_bool_unsat_constraint_casted->get_var() == 8)
    };

/**********************************************************************************************************************/

    void run_tests() override {
        RUN_TEST(testLinearConstraintCheckerPositive())
        RUN_TEST(testToMarabouInequality())
        RUN_TEST(testToMarabouInequalityDuplicates())
        RUN_TEST(testToMarabouInequalityComplex())
        RUN_TEST(testToMarabouInequalityScaled())
        RUN_TEST(testToMarabouInequalityBool())
        RUN_TEST(testToMarabouBoolUnsat())
    }

};

TEST_MAIN(ToMarabouEquationPredicateTest)

#endif //PLAJA_TESTS_TO_MARABOU_EQUATION_PREDICATE_TEST_CPP
