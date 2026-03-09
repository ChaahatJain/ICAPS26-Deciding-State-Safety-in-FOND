//
// This file is part of the PlaJA code base.
// Copyright (c) (2019 - 2020) Marcel Vinzent.
// See README.md in the top-level directory for licensing information.
//

#ifndef PLAJA_EXPRESSION_SET_TEST_CPP
#define PLAJA_EXPRESSION_SET_TEST_CPP

#include "../utils/test_header.h"
#include "../../option_parser/option_parser.h"
#include "../../parser/ast/expression/array_access_expression.h"
#include "../../parser/ast/expression/binary_op_expression.h"
#include "../../parser/ast/expression/unary_op_expression.h"
#include "../../parser/ast/variable_declaration.h"
#include "../../parser/visitor/variable_substitution.h"
#include "../../parser/visitor/semantics_checker.h"
#include "../../parser/parser.h"

class VariableSubstitutionTest: public PLAJA_TEST::TestSuite {

private:
    PLAJA::OptionParser optionParser;
    Parser parser;
    // JANI structures
    // variables:
    std::unique_ptr<VariableDeclaration> varDecl1;
    std::unique_ptr<VariableDeclaration> varDecl2;
    std::unique_ptr<VariableDeclaration> varDecl3;
    // expression:
    std::unique_ptr<Expression> simple1;
    std::unique_ptr<Expression> complex1;
    ArrayAccessExpression* aaExp;
    BinaryOpExpression* timesExp;

public:
    VariableSubstitutionTest():
        parser(optionParser),
        varDecl1(parser.parse_VarDeclStr(R"({"name": "A", "type": {"kind":"array", "base":"int"}})")),
        varDecl2(parser.parse_VarDeclStr(R"({"name": "x", "type": "int"})")),
        varDecl3(parser.parse_VarDeclStr(R"({"name": "y", "type": "int"})")) {

        // create manual JANI expressions
        simple1 = parser.parse_ExpStr(R"("x")");
        SemanticsChecker::check_semantics(simple1.get());

        // not ((A[x] >= -5 * y)
        // (A[x] >= -5 * y)
        std::unique_ptr<BinaryOpExpression> leExp(new BinaryOpExpression(BinaryOpExpression::LE));
        leExp->set_left(parser.parse_ExpStr(R"({"op":"aa", "exp":"A", "index":"x"})"));
        leExp->set_right(parser.parse_ExpStr(R"({"op":"*", "left":-5, "right":"y"})"));
        aaExp = static_cast<ArrayAccessExpression*>(leExp->get_left()); // NOLINT(cppcoreguidelines-pro-type-static-cast-downcast)
        timesExp = static_cast<BinaryOpExpression*>(leExp->get_right()); // NOLINT(cppcoreguidelines-pro-type-static-cast-downcast)
        // not (A[x] >= -5 * c)
        complex1 = std::make_unique<UnaryOpExpression>(UnaryOpExpression::NOT);
        static_cast<UnaryOpExpression*>(complex1.get())->set_operand(std::move(leExp)); // NOLINT(cppcoreguidelines-pro-type-static-cast-downcast)
        SemanticsChecker::check_semantics(complex1.get());
    }

    ~VariableSubstitutionTest() override = default;

/**********************************************************************************************************************/

    void testVarSubSimple() {
        std::cout << std::endl << "Simple test on variable substitution:" << std::endl;

        std::unordered_map<VariableID_type, const Expression*> id_to_exp{{1, complex1.get()}};

        VariableSubstitution::substitute(id_to_exp, simple1);
        std::cout << simple1->to_string() << std::endl;
        TS_ASSERT(complex1->equals(simple1.get()))
    }

    void testVarSubComplex() {
        std::cout << std::endl << "Complex test on variable substitution:" << std::endl;

        std::unique_ptr<Expression> substituteX = parser.parse_ExpStr(R"({"op":"+", "left":"y", "right":2})");
        std::unique_ptr<Expression> substituteY = parser.parse_ExpStr(R"({"op":"floor", "exp":"x"})");
        std::unordered_map<VariableID_type, const Expression*> id_to_exp{{1, substituteX.get()}, {2, substituteY.get()}};

        VariableSubstitution::substitute(id_to_exp, complex1);
        std::cout << complex1->to_string() << std::endl;
        TS_ASSERT(aaExp->get_index()->equals(substituteX.get()))
        TS_ASSERT(timesExp->get_right()->equals(substituteY.get()))
    }

/**********************************************************************************************************************/

    void run_tests() override {
        RUN_TEST(testVarSubSimple())
        RUN_TEST(testVarSubComplex())
    }

};

TEST_MAIN(VariableSubstitutionTest)

#endif //PLAJA_EXPRESSION_SET_TEST_CPP
