//
// This file is part of the PlaJA code base.
// Copyright (c) (2019 - 2020) Marcel Vinzent.
// See README.md in the top-level directory for licensing information.
//

#ifndef PLAJA_EXPRESSION_SET_TEST_CPP
#define PLAJA_EXPRESSION_SET_TEST_CPP

#include "../utils/test_header.h"
#include "../../option_parser/option_parser.h"
#include "../../parser/ast/expression/non_standard/expression_set.h"
#include "../../parser/ast/expression/binary_op_expression.h"
#include "../../parser/ast/expression/bool_value_expression.h"
#include "../../parser/ast/expression/constant_expression.h"
#include "../../parser/ast/expression/expression_utils.h"
#include "../../parser/ast/expression/integer_value_expression.h"
#include "../../parser/ast/expression/real_value_expression.h"
#include "../../parser/ast/expression/unary_op_expression.h"
#include "../../parser/ast/constant_declaration.h"
#include "../../parser/ast/variable_declaration.h"
#include "../../parser/parser.h"

class ExpressionSetTest: public PLAJA_TEST::TestSuite {

private:
    PLAJA::OptionParser optionParser;
    Parser parser;
    /* JANI structures. */
    std::unique_ptr<ConstantDeclaration> constantDecl;
    std::unique_ptr<VariableDeclaration> arrayDecl;
    std::unique_ptr<VariableDeclaration> varDecl;
    /**/
    std::unique_ptr<Expression> complex1;
    ConstantExpression* constantExp;
    BinaryOpExpression* timesExp;

public:

    ExpressionSetTest():
        parser(optionParser)
        , constantDecl(parser.parse_ConstDeclStr(R"({"name": "constant_name", "type": "int"})"))
        , arrayDecl(parser.parse_VarDeclStr(R"({"name": "A", "type": {"kind":"array", "base":"int"}})"))
        , varDecl(parser.parse_VarDeclStr(R"({"name": "x", "type": "int"})")) {

        // create manual JANI expressions
        // not ((A[x] >= -5 * c) && true) // missing real value + constant value

        // -5 * c
        std::unique_ptr<BinaryOpExpression> times_exp(timesExp = new BinaryOpExpression(BinaryOpExpression::TIMES));
        times_exp->set_left(std::make_unique<IntegerValueExpression>(-5));
        times_exp->set_right(parser.parse_ExpStr(R"("constant_name")"));
        constantExp = PLAJA_UTILS::cast_ptr<ConstantExpression>(times_exp->get_right());

        // (A[x] >= -5 * c)
        std::unique_ptr<BinaryOpExpression> le_exp(new BinaryOpExpression(BinaryOpExpression::LE));
        le_exp->set_left(parser.parse_ExpStr(R"({"op":"aa", "exp":"A", "index":"x"})"));
        le_exp->set_right(std::move(times_exp));
        // ((A[x] >= -5 * c) && true)
        std::unique_ptr<BinaryOpExpression> and_exp(new BinaryOpExpression(BinaryOpExpression::AND));
        and_exp->set_left(std::move(le_exp));
        and_exp->set_right(parser.parse_ExpStr(R"(true)"));
        complex1 = std::make_unique<UnaryOpExpression>(UnaryOpExpression::NOT);
        PLAJA_UTILS::cast_ptr<UnaryOpExpression>(complex1.get())->set_operand(std::move(and_exp));
    }

    ~ExpressionSetTest() override = default;

    DELETE_CONSTRUCTOR(ExpressionSetTest)

/**********************************************************************************************************************/

    void testExpSetSimple() {
        std::cout << std::endl << "Simple test on exp set:" << std::endl;

        const auto TRUE1 = PLAJA_EXPRESSION::make_bool(true);
        const auto TRUE2 = PLAJA_EXPRESSION::make_bool(true);
        const auto FALSE1 = PLAJA_EXPRESSION::make_bool(false);
        const auto FALSE2 = PLAJA_EXPRESSION::make_bool(false);

        ExpressionSet test_set;
        test_set.insert(TRUE1.get());
        TS_ASSERT(test_set.count(TRUE2.get()))
        TS_ASSERT(not test_set.count(FALSE1.get()))
        test_set.erase(TRUE2.get());
        TS_ASSERT(not test_set.count(TRUE1.get()))

        // TRUE1.reset(nullptr);
        // TS_ASSERT(!test_set.count(TRUE2.get()))

        test_set.insert(FALSE1.get());
        TS_ASSERT(test_set.count(FALSE2.get()))
        PLAJA_UTILS::cast_ref<BoolValueExpression>(*FALSE1).set_value(true);
        TS_ASSERT(not test_set.count(FALSE2.get()))
        TS_ASSERT(not test_set.count(TRUE2.get()))

        // force error:
        // FALSE1.reset(nullptr);
        // TS_ASSERT(!test_set.count(FALSE2.get()))

    }

    void testExpSetComplex() {
        std::cout << std::endl << "Complex test on exp set:" << std::endl;
        ExpressionSet test_set;
        test_set.insert(complex1.get());
        TS_ASSERT(test_set.count(complex1.get()))
        const auto complex1_copy = complex1->deepCopy_Exp();
        TS_ASSERT(test_set.count(complex1_copy.get()))

        // Constant expression hash/equality do not reflect set value.
        constantDecl->set_value(std::make_unique<IntegerValueExpression>(4)); // constantExp->set_value();
        TS_ASSERT(test_set.count(complex1_copy.get()))
        constantDecl->set_value(nullptr); // constantExp->set_value(nullptr);
        TS_ASSERT(test_set.count(complex1.get()))

        timesExp->set_left(std::make_unique<RealValueExpression>(RealValueExpression::NONE_C, -5));
        TS_ASSERT(not test_set.count(complex1_copy.get())) // Equality check fails.

        timesExp->set_left(std::make_unique<IntegerValueExpression>(5));
        TS_ASSERT(not test_set.count(complex1_copy.get())) // Equality check fails.
    }

/**********************************************************************************************************************/

    void run_tests() override {
        RUN_TEST(testExpSetSimple())
        RUN_TEST(testExpSetComplex())
    }

};

TEST_MAIN(ExpressionSetTest)

#endif //PLAJA_EXPRESSION_SET_TEST_CPP
