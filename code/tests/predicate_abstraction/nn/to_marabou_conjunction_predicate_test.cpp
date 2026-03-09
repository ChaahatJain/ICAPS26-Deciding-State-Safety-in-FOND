//
// This file is part of the PlaJA code base.
// Copyright (c) (2019 - 2021) Marcel Vinzent.
// See README.md in the top-level directory for licensing information.
//

#ifndef PLAJA_TO_MARABOU_CONJUNCTION_PREDICATE_TEST_CPP
#define PLAJA_TO_MARABOU_CONJUNCTION_PREDICATE_TEST_CPP

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

class ToMarabouConjunctionPredicateTest: public PLAJA_TEST::TestSuite {

private:
    PLAJA::OptionParser optionParser;
    Parser parser;
    // JANI structures
    std::list<std::unique_ptr<VariableDeclaration>> vars;
    std::unique_ptr<Expression> simpleConjunction; // No disjunction but equality (this is all we support so far anyway).
    // TODO implement tests for these structures
    std::unique_ptr<Expression> simpleDisjunction; // Singleton case splits and case split with equality.
    std::unique_ptr<Expression> complexDisjunction; // Non-singleton case splits and case split with equality.
    // std::unique_ptr<Expression> complexConjunction; // With complex disjunction.
    // std::unique_ptr<Expression> moreComplexConjunction; // With complex disjunction and singleton disjunction.
    //
    // std::unique_ptr<Expression> simpleConjunctionNeg;

    // Marabou structures
    MARABOU_IN_PLAJA::Context context;
    // To map state variable indexes in marabou.
    StateIndexesInMarabou stateIndexesToMarabou;

public:

    ToMarabouConjunctionPredicateTest():
        parser(optionParser)
        , stateIndexesToMarabou(context) {
        vars.push_back(parser.parse_var_decl_str(R"({"name": "x", "type": "int"})", 1)); // Just for completeness, we set the index in the expression explicitly via SetVarIndexByID.
        vars.push_back(parser.parse_var_decl_str(R"({"name": "y", "type": "int"})", 2));
        vars.push_back(parser.parse_var_decl_str(R"({"name": "b1", "type": "bool"})", 3));
        vars.push_back(parser.parse_var_decl_str(R"({"name": "b2", "type": "bool"})", 4));
        const std::vector<VariableIndex_type> id_to_index { 1, 2, 3, 4 };

        // create manual JANI expressions
        // SemanticsChecker to determine type as used subsequently,
        // Note, determine_type call to parent expression recursively calls children only if necessary (i.e., if type cannot be determined without child type assuming a valid instance)

        // x = 0 && b1 && x + y >= 5 && x + y = 5 && (b1 && ! b2)
        simpleConjunction = parser.parse_ExpStr(R"({"op":"∧", "left":{"op":"=", "left":"x", "right":0}, "right":{"op":"∧", "left":"b1", "right":{"op":"∧", "left":{"op":"≥", "left":{"op":"+", "left":"x", "right":"y"}, "right":5}, "right":{"op":"∧", "left":{"op":"=", "left":{"op":"+", "left":"x", "right":"y"}, "right":5}, "right":{"op":"∧", "left":"b1", "right":{"op":"¬", "exp":"b2"}}} }}})");
        SET_VARS::set_var_index_by_id(id_to_index, *simpleConjunction);
        SemanticsChecker::check_semantics(simpleConjunction.get());

        // x = 0 || b1 || x + y >= 5 || x + y = 5
        simpleDisjunction = parser.parse_ExpStr(R"({"op":"∨", "left":{"op":"=", "left":"x", "right":0}, "right":{"op":"∨", "left":"b1", "right":{"op":"∨", "left":{"op":"≥", "left":{"op":"+", "left":"x", "right":"y"}, "right":5}, "right":{"op":"=", "left":{"op":"+", "left":"x", "right":"y"}, "right":5} }}})");
        SET_VARS::set_var_index_by_id(id_to_index, *simpleDisjunction);
        SemanticsChecker::check_semantics(simpleDisjunction.get());

        // (!b2 && x = 0 && x + y >= 5) || (b1 && x + y = 5)
        complexDisjunction = parser.parse_ExpStr(R"({"op":"∨", "left":{"op":"∧", "left":{"op":"¬", "exp":"b2"}, "right":{"op":"∧", "left":{"op":"=", "left":"x", "right":0}, "right":{"op":"≥", "left":{"op":"+", "left":"x", "right":"y"}, "right":5}}}, "right":{"op":"∧", "left":"b1", "right":{"op":"=", "left":{"op":"+", "left":"x", "right":"y"}, "right":5}}})");
        SET_VARS::set_var_index_by_id(id_to_index, *complexDisjunction);
        SemanticsChecker::check_semantics(complexDisjunction.get());

#if 0
        // neg(x = 0 && b1 >= 1 && x + y >= 5 && x + y = 5 && (b1 - b2 >= 1)) -> x <= -1 || x >= 1 || b1 <= 0 || x + y <= 4 || x + y <= 4 || x + y >= 6 || (b1 - b2 <= 0)
        simpleConjunctionNeg = parser.parse_ExpStr(R"(
        {"op":"∨", "left":{ "op":"≤", "left":"x", "right":-1 },
        "right":{ "op":"∨", "left":{ "op":"≥", "left":"x", "right": 1 },
        "right":{ "op":"∨", "left":{ "op":"¬", "exp":"b1" },
        "right":{ "op":"∨", "left":{ "op":"≤", "left": { "op":"+", "left":"x", "right":"y" }, "right": 4 },
        "right":{ "op":"∨", "left":{ "op":"≤", "left": { "op":"+", "left":"x", "right":"y" }, "right": 4 },
        "right":{ "op":"∨", "left":{ "op":"≥", "left": { "op":"+", "left":"x", "right":"y" }, "right": 6 },
        "right": {"op":"∨", "left": {"op":"¬", "exp": "b1" }, "right":"b2" }
        }}}}} })");
        SET_VARS::set_var_index_by_id(id_to_index, *simpleConjunctionNeg);
        SemanticsChecker::check_semantics(simpleConjunctionNeg.get());
#endif

        // Marabou
        for (VariableIndex_type state_index = 0; state_index <= 8; ++state_index) { stateIndexesToMarabou.add(state_index, FloatUtils::negativeInfinity(), FloatUtils::infinity(), true, true, false); }

    }

    ~ToMarabouConjunctionPredicateTest() override = default;

/**********************************************************************************************************************/

    void testLinearConstraintChecker() {
        std::cout << std::endl << "Simple positive test on linear restrictions checker:" << std::endl;
        TS_ASSERT(LinearConstraintsChecker::is_linear(*simpleConjunction))
        std::cout << simpleConjunction->to_string() << std::endl;
        TS_ASSERT(LinearConstraintsChecker::is_linear(*simpleDisjunction))
        std::cout << simpleDisjunction->to_string() << std::endl;
        TS_ASSERT(LinearConstraintsChecker::is_linear(*complexDisjunction.get()))
        std::cout << complexDisjunction->to_string() << std::endl;
    }

    void testToMarabouSimpleConjunction() {
        std::cout << std::endl << "Test to Marabou conjunction:" << std::endl;
        std::unique_ptr<PredicateConstraint> simple_conjunction(new PredicateConstraint(MARABOU_IN_PLAJA::to_marabou_constraint(*simpleConjunction, stateIndexesToMarabou)));

        // neg(x = 0 && b1 >= 1 && x + y >= 5 && x + y = 5 && (b1 - b2 >= 1)) -> x <= -1 || x >= 1 || b1 <= 0 || x + y <= 4 || x + y <= 4 || x + y >= 6 || (b1 - b2 <= 0)
        // conjunction
        const auto* simple_conjunction_casted = PLAJA_UTILS::cast_ptr_if<ConjunctionInMarabou>(simple_conjunction->constraint.get());
        TS_ASSERT(simple_conjunction_casted)
        TS_ASSERT_EQUALS(simple_conjunction_casted->equations.size(), 3)
        TS_ASSERT_EQUALS(simple_conjunction_casted->bounds.size(), 3)
        TS_ASSERT_EQUALS(simple_conjunction_casted->disjunctions.size(), 0)
        // conjunction negation
        const auto* simple_conjunction_negation_casted = PLAJA_UTILS::cast_ptr_if<DisjunctionInMarabou>(simple_conjunction->negation.get());
        TS_ASSERT(simple_conjunction_negation_casted)
        TS_ASSERT_EQUALS(simple_conjunction_negation_casted->get_disjuncts().size(), 7)
        unsigned int num_bounds = 0;
        for (const auto& case_split: simple_conjunction_negation_casted->get_disjuncts()) {
            TS_ASSERT_EQUALS(case_split.getBoundTightenings().size() + case_split.getEquations().size(), 1)
            if (!case_split.getBoundTightenings().empty()) {
                ++num_bounds;
            }
        }
        TS_ASSERT_EQUALS(num_bounds, 3)
    }

/**********************************************************************************************************************/

    void run_tests() override {
        RUN_TEST(testLinearConstraintChecker())
        RUN_TEST(testToMarabouSimpleConjunction())
    }

};

TEST_MAIN(ToMarabouConjunctionPredicateTest)

#endif //PLAJA_TO_MARABOU_CONJUNCTION_PREDICATE_TEST_CPP
