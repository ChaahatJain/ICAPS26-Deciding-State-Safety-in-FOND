//
// This file is part of the PlaJA code base.
// Copyright (c) (2019 - 2020) Marcel Vinzent.
// See README.md in the top-level directory for licensing information.
//

#ifndef PLAJA_TESTS_TO_MARABOU_PREDICATE_TEST_CPP
#define PLAJA_TESTS_TO_MARABOU_PREDICATE_TEST_CPP

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
#include "../../../search/smt_nn/marabou_query.h"
#include "../../../search/smt_nn/vars_in_marabou.h"

class ToMarabouBoundPredicateTest: public PLAJA_TEST::TestSuite {

private:
    PLAJA::OptionParser optionParser;
    Parser parser;
    // JANI structures
    std::list<std::unique_ptr<VariableDeclaration>> vars;
    std::unique_ptr<Expression> bound;
    std::unique_ptr<Expression> bound2;
    std::unique_ptr<Expression> boundMirror;
    std::unique_ptr<Expression> boundEquality;
    std::unique_ptr<Expression> boundComplex;
    std::unique_ptr<Expression> floatingBound;
    std::unique_ptr<Expression> floatingEquality;
    std::unique_ptr<Expression> boundBool;
    std::unique_ptr<Expression> boundBoolNeg;

    // Marabou structures
    std::shared_ptr<MARABOU_IN_PLAJA::Context> context;
    // to map state variable indexes in marabou
    StateIndexesInMarabou stateIndexesToMarabou;

public:

    ToMarabouBoundPredicateTest():
        parser(optionParser)
        , context(new MARABOU_IN_PLAJA::Context())
        , stateIndexesToMarabou(*context) {
        vars.push_back(parser.parse_var_decl_str(R"({"name": "x", "type": "int"})", 1)); // Just for completeness, we set the index in the expression explicitly via SetVarIndexByID
        vars.push_back(parser.parse_var_decl_str(R"({"name": "A", "type": {"kind":"array", "base":"int"}})", 2));
        vars.push_back(parser.parse_var_decl_str(R"({"name": "y", "type": "bool"})", 4));
        vars.push_back(parser.parse_var_decl_str(R"({"name": "B", "type": {"kind":"array", "base":"bool"}})", 5));
        const std::vector<VariableIndex_type> id_to_index { 1, 2, 4, 5 }; // to set indexes in expressions

        // create manual JANI expressions
        // SemanticsChecker to determine type as used subsequently,
        // Note, determine_type call to parent expression recursively calls children only if necessary (i.e., if type cannot be determined without child type assuming a valid instance)

        // x <= 5
        bound = parser.parse_ExpStr(R"({"left":"x","op":"≤","right":5})");
        SET_VARS::set_var_index_by_id(id_to_index, *bound);
        SemanticsChecker::check_semantics(bound.get());

        // A[1] >= -5
        bound2 = parser.parse_ExpStr(R"({"left":{"exp":"A","index":1,"op":"aa"},"op":"≥","right":-5})");
        SET_VARS::set_var_index_by_id(id_to_index, *bound2);
        SemanticsChecker::check_semantics(bound2.get());

        // 5 >= x
        boundMirror = parser.parse_ExpStr(R"({"left":5,"op":"≥","right":"x"}})");
        SET_VARS::set_var_index_by_id(id_to_index, *boundMirror);
        SemanticsChecker::check_semantics(boundMirror.get());

        // 5 == x
        boundEquality = parser.parse_ExpStr(R"({"left":5,"op":"=","right":"x"}})");
        SET_VARS::set_var_index_by_id(id_to_index, *boundEquality);
        SemanticsChecker::check_semantics(boundEquality.get());

        // 6 + x >= 3*x
        boundComplex = parser.parse_ExpStr(R"({"left":{"op":"+", "left":6, "right":"x"}, "op":"≥", "right":{"op":"*", "left":3, "right":"x"}}})");
        SET_VARS::set_var_index_by_id(id_to_index, *boundComplex);
        SemanticsChecker::check_semantics(boundComplex.get());

        // 5 + x >= 3*x
        floatingBound = parser.parse_ExpStr(R"({"left":{"op":"+", "left":5, "right":"x"}, "op":"≥", "right":{"op":"*", "left":3, "right":"x"}}})");
        SET_VARS::set_var_index_by_id(id_to_index, *floatingBound);
        SemanticsChecker::check_semantics(floatingBound.get());

        // 5 + x = 3*x
        floatingEquality = parser.parse_ExpStr(R"({"left":{"op":"+", "left":5, "right":"x"}, "op":"=", "right":{"op":"*", "left":3, "right":"x"}}})");
        SET_VARS::set_var_index_by_id(id_to_index, *floatingEquality);
        SemanticsChecker::check_semantics(floatingEquality.get());

        // y
        boundBool = parser.parse_ExpStr(R"("y")");
        SET_VARS::set_var_index_by_id(id_to_index, *boundBool);
        SemanticsChecker::check_semantics(boundBool.get());

        // !B[0]
        boundBoolNeg = parser.parse_ExpStr(R"({"exp":{"exp":"B","index":0,"op":"aa"},"op":"¬"})");
        SET_VARS::set_var_index_by_id(id_to_index, *boundBoolNeg);
        SemanticsChecker::check_semantics(boundBoolNeg.get());

        // Marabou
        for (VariableIndex_type state_index = 0; state_index <= 5; ++state_index) { stateIndexesToMarabou.add(state_index, FloatUtils::negativeInfinity(), FloatUtils::infinity(), true, true, false); }

    }

    ~ToMarabouBoundPredicateTest() override = default;

/**********************************************************************************************************************/

    void testLinearConstraintCheckerPositive() {
        std::cout << std::endl << "Simple positive test on linear restrictions checker:" << std::endl;
        TS_ASSERT(LinearConstraintsChecker::is_linear(*bound, LINEAR_CONSTRAINTS_CHECKER::Specification::as_predicate(true)))
        std::cout << bound->to_string() << std::endl;
        TS_ASSERT(LinearConstraintsChecker::is_linear(*bound2, LINEAR_CONSTRAINTS_CHECKER::Specification::as_predicate(true)))
        std::cout << bound2->to_string() << std::endl;
        TS_ASSERT(LinearConstraintsChecker::is_linear(*boundMirror, LINEAR_CONSTRAINTS_CHECKER::Specification::as_predicate(true)))
        std::cout << boundMirror->to_string() << std::endl;
        //
        TS_ASSERT(not LinearConstraintsChecker::is_linear(*boundEquality, LINEAR_CONSTRAINTS_CHECKER::Specification::as_predicate(true)))
        // but:
        TS_ASSERT(LinearConstraintsChecker::is_linear(*boundEquality, LINEAR_CONSTRAINTS_CHECKER::Specification::as_predicate(false)))
        std::cout << boundEquality->to_string() << std::endl;
        //
        TS_ASSERT(LinearConstraintsChecker::is_linear(*boundComplex, LINEAR_CONSTRAINTS_CHECKER::Specification::as_predicate(true)))
        std::cout << boundComplex->to_string() << std::endl;
        TS_ASSERT(LinearConstraintsChecker::is_linear(*boundBool, LINEAR_CONSTRAINTS_CHECKER::Specification::as_predicate(true)))
        std::cout << boundBool->to_string() << std::endl;
        TS_ASSERT(LinearConstraintsChecker::is_linear(*boundBoolNeg, LINEAR_CONSTRAINTS_CHECKER::Specification::as_predicate(true)))
        std::cout << boundBoolNeg->to_string() << std::endl;
    }

    void testToMarabouBoundConstraint() {
        std::cout << std::endl << "Test to Marabou bound constraint:" << std::endl;
        std::unique_ptr<PredicateConstraint> bound_constraint = MARABOU_IN_PLAJA::to_predicate_constraint(*bound, stateIndexesToMarabou);

        // bound
        const auto* bound_constraint_casted = PLAJA_UTILS::cast_ptr_if<BoundConstraint>(bound_constraint->constraint.get());
        TS_ASSERT(bound_constraint_casted)
        TS_ASSERT_EQUALS(bound_constraint_casted->get_var(), 1)
        TS_ASSERT_EQUALS(bound_constraint_casted->get_value(), 5)
        TS_ASSERT_EQUALS(bound_constraint_casted->get_type(), Tightening::UB)

        // bound negation
        const auto* bound_constraint_negation_casted = PLAJA_UTILS::cast_ptr_if<BoundConstraint>(bound_constraint->negation.get());
        TS_ASSERT(bound_constraint_negation_casted)
        TS_ASSERT_EQUALS(bound_constraint_negation_casted->get_var(), 1)
        TS_ASSERT_EQUALS(bound_constraint_negation_casted->get_value(), 6)
        TS_ASSERT_EQUALS(bound_constraint_negation_casted->get_type(), Tightening::LB)

        // add to query
        MARABOU_IN_PLAJA::MarabouQuery query(PLAJA_TEST_UTILS::share(context));
        query.set_upper_bound(1, 25);
        bound_constraint->add_to_query(query);
        TS_ASSERT_EQUALS(query.get_upper_bound(1), 5)

        // add to query negation
        MARABOU_IN_PLAJA::MarabouQuery query_negation(PLAJA_TEST_UTILS::share(context));
        query_negation.set_lower_bound(1, 0);
        bound_constraint->add_negation_to_query(query_negation);
        TS_ASSERT_EQUALS(query_negation.get_lower_bound(1), 6)
    }

    void testToMarabouBound2() {
        std::cout << std::endl << "Test to Marabou constraint on bound 2:" << std::endl;

        std::unique_ptr<PredicateConstraint> bound2_constraint = MARABOU_IN_PLAJA::to_predicate_constraint(*bound2, stateIndexesToMarabou);

        // bound2
        const auto* bound2_constraint_casted = PLAJA_UTILS::cast_ptr_if<BoundConstraint>(bound2_constraint->constraint.get());
        TS_ASSERT(bound2_constraint_casted)
        TS_ASSERT_EQUALS(bound2_constraint_casted->get_var(), 3)
        TS_ASSERT_EQUALS(bound2_constraint_casted->get_value(), -5)
        TS_ASSERT_EQUALS(bound2_constraint_casted->get_type(), Tightening::LB)

        // bound2 negation
        const auto* bound2_constraint_negation_casted = PLAJA_UTILS::cast_ptr_if<BoundConstraint>(bound2_constraint->negation.get());
        TS_ASSERT(bound2_constraint_negation_casted)
        TS_ASSERT_EQUALS(bound2_constraint_negation_casted->get_var(), 3)
        TS_ASSERT_EQUALS(bound2_constraint_negation_casted->get_value(), -6)
        TS_ASSERT_EQUALS(bound2_constraint_negation_casted->get_type(), Tightening::UB)

        // bound2 add to query
        MARABOU_IN_PLAJA::MarabouQuery query_2(PLAJA_TEST_UTILS::share(context));
        query_2.set_lower_bound(3, -25);
        bound2_constraint->add_to_query(query_2);
        TS_ASSERT_EQUALS(query_2.get_lower_bound(3), -5)

        // bound2 negation
        MARABOU_IN_PLAJA::MarabouQuery query_2_negation(PLAJA_TEST_UTILS::share(context));
        query_2_negation.set_upper_bound(3, 0);
        bound2_constraint->add_negation_to_query(query_2_negation);
        TS_ASSERT_EQUALS(query_2_negation.get_upper_bound(3), -6)
    }

    void testToMarabouBoundMirror() {
        std::cout << std::endl << "Test to Marabou constraint on mirrored bound:" << std::endl;
        std::unique_ptr<PredicateConstraint> bound_mirror_constraint = MARABOU_IN_PLAJA::to_predicate_constraint(*boundMirror, stateIndexesToMarabou);

        // bound mirror
        const auto* bound_mirror_constraint_casted = PLAJA_UTILS::cast_ptr_if<BoundConstraint>(bound_mirror_constraint->constraint.get());
        TS_ASSERT(bound_mirror_constraint_casted)
        TS_ASSERT_EQUALS(bound_mirror_constraint_casted->get_var(), 1)
        TS_ASSERT_EQUALS(bound_mirror_constraint_casted->get_value(), 5)
        TS_ASSERT_EQUALS(bound_mirror_constraint_casted->get_type(), Tightening::UB)

        // bound mirror negation
        const auto* bound_mirror_constraint_negation_casted = PLAJA_UTILS::cast_ptr_if<BoundConstraint>(bound_mirror_constraint->negation.get());
        TS_ASSERT(bound_mirror_constraint_negation_casted)
        TS_ASSERT_EQUALS(bound_mirror_constraint_negation_casted->get_var(), 1)
        TS_ASSERT_EQUALS(bound_mirror_constraint_negation_casted->get_value(), 6)
        TS_ASSERT_EQUALS(bound_mirror_constraint_negation_casted->get_type(), Tightening::LB)
    }

    void testToMarabouBoundEquality() {
        std::cout << std::endl << "Test to Marabou bound constraint on equality:" << std::endl;
        auto bound_equality_constraint = MARABOU_IN_PLAJA::to_marabou_constraint(*boundEquality, stateIndexesToMarabou);

        // bound
        const auto* bound_equality_constraint_casted = PLAJA_UTILS::cast_ptr_if<ConjunctionInMarabou>(bound_equality_constraint.get());
        TS_ASSERT(bound_equality_constraint_casted->equations.empty())
        TS_ASSERT_EQUALS(bound_equality_constraint_casted->bounds.size(), 2)
        TS_ASSERT(bound_equality_constraint_casted->disjunctions.empty())
        TS_ASSERT_EQUALS(bound_equality_constraint_casted->bounds.front()._variable, 1)
        TS_ASSERT_EQUALS(bound_equality_constraint_casted->bounds.back()._variable, 1)
        TS_ASSERT_EQUALS(bound_equality_constraint_casted->bounds.front()._value, 5)
        TS_ASSERT_EQUALS(bound_equality_constraint_casted->bounds.back()._value, 5)
        TS_ASSERT_EQUALS(bound_equality_constraint_casted->bounds.front()._type, Tightening::LB)
        TS_ASSERT_EQUALS(bound_equality_constraint_casted->bounds.back()._type, Tightening::UB)

        // bound negation
        auto negation = bound_equality_constraint_casted->compute_negation();
        const auto* bound_equality_negation_casted = PLAJA_UTILS::cast_ptr_if<DisjunctionInMarabou>(negation.get());
        TS_ASSERT(bound_equality_negation_casted)
        TS_ASSERT(bound_equality_negation_casted->get_disjuncts().size() == 2)
        for (const auto& disjunct: bound_equality_negation_casted->get_disjuncts()) {
            TS_ASSERT(disjunct.getEquations().empty())
            const auto& bounds = disjunct.getBoundTightenings();
            TS_ASSERT(bounds.size() == 1)
            const auto& tightening = bounds.front();
            TS_ASSERT_EQUALS(tightening._variable, 1)
            TS_ASSERT((tightening._value == 4 && tightening._type == Tightening::UB) || (tightening._value == 6 && tightening._type == Tightening::LB))
        }

        // add to query
        MARABOU_IN_PLAJA::MarabouQuery query(PLAJA_TEST_UTILS::share(context));
        query.set_lower_bound(1, -25);
        query.set_upper_bound(1, 25);
        bound_equality_constraint->add_to_query(query);
        TS_ASSERT_EQUALS(query.get_lower_bound(1), 5)
        TS_ASSERT_EQUALS(query.get_upper_bound(1), 5)
    }

    void testToMarabouBoundComplex() {
        std::cout << std::endl << "Test to Marabou bound constraint:" << std::endl;
        std::unique_ptr<PredicateConstraint> bound_constraint = MARABOU_IN_PLAJA::to_predicate_constraint(*boundComplex, stateIndexesToMarabou);

        // bound
        const auto* bound_complex_constraint_casted = PLAJA_UTILS::cast_ptr_if<BoundConstraint>(bound_constraint->constraint.get());
        TS_ASSERT(bound_complex_constraint_casted)
        TS_ASSERT_EQUALS(bound_complex_constraint_casted->get_var(), 1)
        TS_ASSERT_EQUALS(bound_complex_constraint_casted->get_value(), 3)
        TS_ASSERT_EQUALS(bound_complex_constraint_casted->get_type(), Tightening::UB)

        // bound negation
        const auto* bound_constraint_negation_casted = PLAJA_UTILS::cast_ptr_if<BoundConstraint>(bound_constraint->negation.get());
        TS_ASSERT(bound_constraint_negation_casted)
        TS_ASSERT_EQUALS(bound_constraint_negation_casted->get_var(), 1)
        TS_ASSERT_EQUALS(bound_constraint_negation_casted->get_value(), 4)
        TS_ASSERT_EQUALS(bound_constraint_negation_casted->get_type(), Tightening::LB)

        // add to query
        MARABOU_IN_PLAJA::MarabouQuery query(PLAJA_TEST_UTILS::share(context));
        query.set_upper_bound(1, 25);
        bound_constraint->add_to_query(query);
        TS_ASSERT_EQUALS(query.get_upper_bound(1), 3)

        // add to query negation
        MARABOU_IN_PLAJA::MarabouQuery query_negation(PLAJA_TEST_UTILS::share(context));
        query_negation.set_lower_bound(1, 0);
        bound_constraint->add_negation_to_query(query_negation);
        TS_ASSERT_EQUALS(query_negation.get_lower_bound(1), 4)
    }

    void testToMarabouFloatingBound() {
        std::cout << std::endl << "Test to Marabou floating bound constraint:" << std::endl;
        auto floating_bound_constraint = MARABOU_IN_PLAJA::to_predicate_constraint(*floatingBound, stateIndexesToMarabou);

        // bound
        const auto* floating_bound_constraint_casted = PLAJA_UTILS::cast_ptr_if<BoundConstraint>(floating_bound_constraint->constraint.get());
        TS_ASSERT_EQUALS(floating_bound_constraint_casted->get_var(), 1)
        TS_ASSERT_EQUALS(floating_bound_constraint_casted->get_value(), 2)
        TS_ASSERT_EQUALS(floating_bound_constraint_casted->get_type(), Tightening::UB)

        // bound negation
        const auto* bound_constraint_negation_casted = PLAJA_UTILS::cast_ptr_if<BoundConstraint>(floating_bound_constraint->negation.get());
        TS_ASSERT(bound_constraint_negation_casted)
        TS_ASSERT_EQUALS(bound_constraint_negation_casted->get_var(), 1)
        TS_ASSERT_EQUALS(bound_constraint_negation_casted->get_value(), 3)
        TS_ASSERT_EQUALS(bound_constraint_negation_casted->get_type(), Tightening::LB)

        // add to query
        MARABOU_IN_PLAJA::MarabouQuery query(PLAJA_TEST_UTILS::share(context));
        query.set_upper_bound(1, 25);
        floating_bound_constraint->add_to_query(query);
        TS_ASSERT_EQUALS(query.get_upper_bound(1), 2)

        // add to query negation
        MARABOU_IN_PLAJA::MarabouQuery query_negation(PLAJA_TEST_UTILS::share(context));
        query_negation.set_lower_bound(1, 0);
        floating_bound_constraint->add_negation_to_query(query_negation);
        TS_ASSERT_EQUALS(query_negation.get_lower_bound(1), 3)
    }

    void testToMarabouFloatingEquality() {
        std::cout << std::endl << "Test to Marabou floating equality constraint:" << std::endl;

        auto floating_equality_constraint = MARABOU_IN_PLAJA::to_marabou_constraint(*floatingEquality, stateIndexesToMarabou);
        const auto* floating_equality_constraint_casted = PLAJA_UTILS::cast_ptr_if<EquationConstraint>(floating_equality_constraint.get());
        TS_ASSERT(floating_equality_constraint_casted)
        //
        TS_ASSERT_EQUALS(floating_equality_constraint_casted->get_equation()._type, Equation::EQ)
        TS_ASSERT_EQUALS(floating_equality_constraint_casted->get_equation()._scalar, 5.0 / 2.0)
        TS_ASSERT_EQUALS(floating_equality_constraint_casted->get_equation().getCoefficient(0), 0) // not present
        TS_ASSERT_EQUALS(floating_equality_constraint_casted->get_equation().getCoefficient(1), 1)
    }

    void testToMarabouBool() {
        std::cout << std::endl << "Test to Marabou constraint on bool literals:" << std::endl;
        std::unique_ptr<PredicateConstraint> bound_bool_constraint = MARABOU_IN_PLAJA::to_predicate_constraint(*boundBool, stateIndexesToMarabou);
        std::unique_ptr<PredicateConstraint> bound_bool_neg_constraint = MARABOU_IN_PLAJA::to_predicate_constraint(*boundBoolNeg, stateIndexesToMarabou);

        // simple pos. literal
        const auto* bound_bool_constraint_casted = PLAJA_UTILS::cast_ptr_if<BoundConstraint>(bound_bool_constraint->constraint.get());
        TS_ASSERT(bound_bool_constraint_casted)
        TS_ASSERT_EQUALS(bound_bool_constraint_casted->get_var(), 4)
        TS_ASSERT_EQUALS(bound_bool_constraint_casted->get_value(), 1)
        TS_ASSERT_EQUALS(bound_bool_constraint_casted->get_type(), Tightening::LB)

        // negative literal
        const auto* bound_bool_neg_constraint_casted = PLAJA_UTILS::cast_ptr_if<BoundConstraint>(bound_bool_neg_constraint->constraint.get());
        TS_ASSERT(bound_bool_neg_constraint_casted)
        TS_ASSERT_EQUALS(bound_bool_neg_constraint_casted->get_var(), 5)
        TS_ASSERT_EQUALS(bound_bool_neg_constraint_casted->get_value(), 0)
        TS_ASSERT_EQUALS(bound_bool_neg_constraint_casted->get_type(), Tightening::UB)
    }

/**********************************************************************************************************************/

    void run_tests() override {
        RUN_TEST(testLinearConstraintCheckerPositive())
        RUN_TEST(testToMarabouBoundConstraint())
        RUN_TEST(testToMarabouBound2())
        RUN_TEST(testToMarabouBoundMirror())
        RUN_TEST(testToMarabouBoundEquality())
        RUN_TEST(testToMarabouBoundComplex())
        RUN_TEST(testToMarabouFloatingBound())
        RUN_TEST(testToMarabouFloatingEquality())
        RUN_TEST(testToMarabouBool())
    }

};

TEST_MAIN(ToMarabouBoundPredicateTest)

#endif //PLAJA_TESTS_TO_MARABOU_PREDICATE_TEST_CPP
