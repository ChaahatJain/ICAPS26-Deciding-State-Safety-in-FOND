//
// This file is part of the PlaJA code base.
// Copyright (c) (2019 - 2020) Marcel Vinzent.
// See README.md in the top-level directory for licensing information.
//

#ifndef PLAJA_EXPRESSION_SET_TEST_CPP
#define PLAJA_EXPRESSION_SET_TEST_CPP

#include "../utils/test_header.h"
#include "../../option_parser/option_parser.h"
#include "../../parser/ast/expression/special_cases/nary_expression.h"
#include "../../parser/ast/expression/array_access_expression.h"
#include "../../parser/ast/expression/unary_op_expression.h"
#include "../../parser/ast/variable_declaration.h"
#include "../../parser/visitor/extern/ast_standardization.h"
#include "../../parser/visitor/to_normalform.h"
#include "../../parser/visitor/semantics_checker.h"
#include "../../parser/parser.h"

class ToNormalformTest: public PLAJA_TEST::TestSuite {

private:
    PLAJA::OptionParser optionParser;
    Parser parser;
    // Variables:
    std::unique_ptr<VariableDeclaration> varDecl1;
    std::unique_ptr<VariableDeclaration> varDecl2;
    std::unique_ptr<VariableDeclaration> varDecl3;
    std::unique_ptr<VariableDeclaration> varDecl4;
    std::unique_ptr<VariableDeclaration> varDecl5;
    std::unique_ptr<VariableDeclaration> varDecl6;
    std::unique_ptr<VariableDeclaration> varDecl7;
    std::unique_ptr<VariableDeclaration> varDecl8;
    std::unique_ptr<VariableDeclaration> varDecl9;
    // Expression:
    std::unique_ptr<Expression> simple1;
    std::unique_ptr<Expression> simple2;
    std::unique_ptr<Expression> complex1;
    //
    std::unique_ptr<Expression> simpleToDis1;
    std::unique_ptr<Expression> complexToDis1;
    std::unique_ptr<Expression> complexToCnf1;

public:

    ToNormalformTest():
        parser(optionParser)
        , varDecl1(parser.parse_VarDeclStr(R"({"name": "A", "type": {"kind":"array", "base":"bool"}})"))
        , varDecl2(parser.parse_VarDeclStr(R"({"name": "x", "type": "int"})"))
        , varDecl3(parser.parse_VarDeclStr(R"({"name": "a", "type": "bool"})"))
        , varDecl4(parser.parse_VarDeclStr(R"({"name": "b", "type": "bool"})"))
        , varDecl5(parser.parse_VarDeclStr(R"({"name": "c", "type": "bool"})"))
        , varDecl6(parser.parse_VarDeclStr(R"({"name": "d", "type": "bool"})"))
        , varDecl7(parser.parse_VarDeclStr(R"({"name": "e", "type": "bool"})"))
        , varDecl8(parser.parse_VarDeclStr(R"({"name": "f", "type": "bool"})"))
        , varDecl9(parser.parse_VarDeclStr(R"({"name": "g", "type": "bool"})")) {

        // Create manual JANI expressions.
        simple1 = parser.parse_ExpStr(R"({"op":"¬", "exp":{"op":"∧","left":{"op":"=","left":"x","right":3},"right":{"op":"aa","exp":"A","index":0}}})");
        simple2 = parser.parse_ExpStr(R"({"op":"¬", "exp":{"op":"∨", "left":{"op":"⇒", "left":true, "right":"b"}, "right":{"op":"¬","exp":{"op":"aa","exp":"A","index":0}}}})");
        complex1 = parser.parse_ExpStr(R"({"op":"⇒", "left":{"op":"∧", "left":{"op":"=","left":"b","right":{"op":"¬","exp":true}}, "right":true}, "right":{"op":"∨", "left":"b","right":false} })");
        simpleToDis1 = parser.parse_ExpStr(R"({"op":"∧", "left":"a", "right":{"op":"∨", "left":"b", "right":"c"} })");
        complexToDis1 = parser.parse_ExpStr(R"({"op":"¬", "exp":{"op":"∧", "left":"a", "right":{"op":"∧", "left":{"op":"∨", "left":"b", "right":"c"}, "right":"d"}} })");
        complexToCnf1 = parser.parse_ExpStr(R"({"op":"⇒", "left":{"op":"¬", "exp":"a"}, "right":{"op":"∨", "left":{"op":"∧", "left":"b", "right":"c"}, "right":"d"}} )");

        // To pass type asserts.
        SemanticsChecker::check_semantics(simple1.get());
        SemanticsChecker::check_semantics(simple2.get());
        SemanticsChecker::check_semantics(complex1.get());
        SemanticsChecker::check_semantics(simpleToDis1.get());
        SemanticsChecker::check_semantics(complexToDis1.get());
        SemanticsChecker::check_semantics(complexToCnf1.get());
    }

    ~ToNormalformTest() override = default;

    DELETE_CONSTRUCTOR(ToNormalformTest)

/**********************************************************************************************************************/

    void test_simple() {
        PLAJA_LOG("Simple test to push down negation:");

        TO_NORMALFORM::push_down_negation(simple1);
        auto simple1_ref = parser.parse_ExpStr(R"({"op":"∨","left":{"op":"≠","left":"x","right":3},"right":{"op":"¬","exp":{"op":"aa","exp":"A","index":0}}})");
        TS_ASSERT(simple1_ref->equals(simple1.get()))

        TO_NORMALFORM::push_down_negation(simple2);
        auto simple2_ref = parser.parse_ExpStr(R"({"op":"∧","left":{"op":"∧","left":true,"right":{"op":"¬", "exp":"b"}},"right":{"op":"aa","exp":"A","index":0}})");
        TS_ASSERT(simple2_ref->equals(simple2.get()))

    }

    void test_complex() {
        PLAJA_LOG("Complex test to push down negation:")

        TO_NORMALFORM::push_down_negation(complex1);
        auto complex1_ref = parser.parse_ExpStr(R"({"left": {"left": {"left":"b","op":"≠","right":false}, "op":"∨", "right":false}, "op": "∨", "right":{"left":"b","op":"∨","right":false}})");
        TS_ASSERT(complex1_ref->equals(complex1.get()))
    }

    void test_simple_to_dis() {
        PLAJA_LOG("Simple test to distribute and-or:")

        auto simple_to_dis_1 = simpleToDis1->deepCopy_Exp();
        TO_NORMALFORM::distribute_or_over_and(simple_to_dis_1);
        TS_ASSERT(simpleToDis1->equals(simple_to_dis_1.get())) // Nothing happens.

        TO_NORMALFORM::distribute_and_over_or(simple_to_dis_1);
        auto simple_to_dis1_ref2 = parser.parse_ExpStr(R"({"op":"∨", "left":{"op":"∧", "left":"a", "right":"b"}, "right":{"op":"∧", "left":"a", "right":"c"} })");
        TS_ASSERT(simple_to_dis1_ref2->equals(simple_to_dis_1.get()))

        /* Distribute back. */
        TO_NORMALFORM::distribute_or_over_and(simple_to_dis_1);
        auto simple_to_dis1_ref3 = parser.parse_ExpStr(R"({"left":{"left":{"left":"a", "op":"∨", "right":"a"}, "op":"∧", "right":{"left":"a", "op":"∨", "right":"c"}}, "op":"∧", "right": {"left":{"left":"b", "op":"∨", "right":"a"}, "op":"∧", "right":{"left":"b", "op":"∨", "right":"c"}} })");
        TS_ASSERT(simple_to_dis1_ref3->equals(simple_to_dis_1.get()))
    }

    void test_complex_to_dis() {
        PLAJA_LOG("Complex test to distribute and-or:")

        auto complex_to_dis_1 = complexToDis1->deepCopy_Exp();
        TO_NORMALFORM::distribute_or_over_and(complex_to_dis_1);
        TS_ASSERT(complexToDis1->equals(complex_to_dis_1.get())) // Nothing happens.

        TO_NORMALFORM::distribute_and_over_or(complex_to_dis_1);
        auto complex_to_dis1_ref2 = parser.parse_ExpStr(R"({"exp":{"left":{"left":"a", "op":"∧", "right":{"left":"d", "op":"∧", "right":"b"}}, "op": "∨", "right":{"left":"a", "op":"∧", "right":{"left":"d", "op":"∧", "right":"c"}}}, "op":"¬"})");
        TS_ASSERT(complex_to_dis1_ref2->equals(complex_to_dis_1.get()))
    }

    void test_complex_to_cnf() {
        PLAJA_LOG("Test to cnf:")
        TO_NORMALFORM::to_cnf(complexToCnf1);
        auto complex_to_cnf1_ref1 = parser.parse_ExpStr(R"({"left":{"left":"a", "op":"∨", "right":{"left":"d", "op":"∨", "right":"b"}}, "op": "∧", "right":{"left":"a", "op":"∨", "right":{"left":"d", "op":"∨", "right":"c"}}})");
        TS_ASSERT(complex_to_cnf1_ref1->equals(complexToCnf1.get()))
    }

    void test_to_nary() {
        PLAJA_LOG("Test to nary:")

        /* Simple1 */
        {
            TO_NORMALFORM::to_nary(simple1);
            auto simple1_ref = parser.parse_ExpStr(R"({"op":"∨","left":{"op":"≠","left":"x","right":3},"right":{"op":"¬","exp":{"op":"aa","exp":"A","index":0}}})");
            TS_ASSERT(simple1_ref->equals(simple1.get())) // Binary is preserved.
        }

        /* Simple2 */
        {
            TO_NORMALFORM::to_nary(simple2);
            NaryExpression simple2_nary(BinaryOpExpression::AND);
            simple2_nary.reserve(3);
            simple2_nary.add_sub(parser.parse_ExpStr(R"(true)"));
            simple2_nary.add_sub(parser.parse_ExpStr(R"({"op":"¬", "exp":"b"})"));
            simple2_nary.add_sub(parser.parse_ExpStr(R"({"op":"aa","exp":"A","index":0})"));
            TS_ASSERT(simple2_nary.equals(simple2.get()))
            /* Standardize again. */
            auto simple2_ref = parser.parse_ExpStr(R"({"op": "∧", "left": true, "right": {"op": "∧", "left": {"op": "¬", "exp":"b"}, "right": {"op": "aa", "exp": "A", "index": 0 } } })");
            AST_STANDARDIZATION::standardize_ast(simple2);
            TS_ASSERT(simple2_ref->equals(simple2.get()))
        }

        /* Complex1. */
        {
            TO_NORMALFORM::to_nary(complex1);
            NaryExpression complex1_nary(BinaryOpExpression::OR);
            complex1_nary.reserve(4);
            complex1_nary.add_sub(parser.parse_ExpStr(R"({"left": "b","op": "≠", "right": false})"));
            complex1_nary.add_sub(parser.parse_ExpStr(R"(false)"));
            complex1_nary.add_sub(parser.parse_ExpStr(R"("b")"));
            complex1_nary.add_sub(parser.parse_ExpStr(R"(false)"));
            TS_ASSERT(complex1_nary.equals(complex1.get()))
            AST_STANDARDIZATION::standardize_ast(complex1);
            auto complex1_ref = parser.parse_ExpStr(R"({ "left": { "left": "b", "op": "≠", "right": false }, "op":"∨", "right": { "left": false, "op":"∨", "right": { "left":"b", "op":"∨", "right":false } } })");
            TS_ASSERT(complex1_ref->equals(complex1.get()))
        }

        /* to-nary top-dis. */
        {

            auto to_nary_expr = parser.parse_ExpStr(R"({ "left": { "op":"∧", "left":"a", "right":"b" }, "op":"∨", "right": { "left": false, "op":"∨", "right": { "op":"∧", "left": "a", "right":{ "op":"∧", "left":"b", "right":"c" } } } })");
            SemanticsChecker::check_semantics(to_nary_expr.get());

            auto to_nary_expr_ref = to_nary_expr->deepCopy_Exp();

            auto nary_dis = std::make_unique<NaryExpression>(BinaryOpExpression::OR);
            nary_dis->reserve(3);
            nary_dis->add_sub(parser.parse_ExpStr(R"({ "op":"∧", "left":"a", "right":"b" })"));
            nary_dis->add_sub(parser.parse_ExpStr(R"(false)"));
            auto nary_con = std::make_unique<NaryExpression>(BinaryOpExpression::AND);
            nary_con->reserve(3);
            nary_con->add_sub(parser.parse_ExpStr(R"("a")"));
            nary_con->add_sub(parser.parse_ExpStr(R"("b")"));
            nary_con->add_sub(parser.parse_ExpStr(R"("c")"));
            nary_dis->add_sub(std::move(nary_con));

            TO_NORMALFORM::to_nary(to_nary_expr);
            TS_ASSERT(nary_dis->equals(to_nary_expr.get()));
            AST_STANDARDIZATION::standardize_ast(to_nary_expr);
            TS_ASSERT(to_nary_expr_ref->equals(to_nary_expr.get()))

        }

        /* to-nary top-con. */
        {

            auto to_nary_expr = parser.parse_ExpStr(R"({ "left": { "op":"∨", "left":"a", "right":"b" }, "op":"∧", "right": { "left": false, "op":"∧", "right": { "op":"∨", "left": "a", "right":{ "op":"∨", "left":"b", "right":"c" } } } })");
            SemanticsChecker::check_semantics(to_nary_expr.get());

            auto to_nary_expr_ref = to_nary_expr->deepCopy_Exp();

            auto nary_con = std::make_unique<NaryExpression>(BinaryOpExpression::AND);
            nary_con->reserve(3);
            nary_con->add_sub(parser.parse_ExpStr(R"({ "op":"∨", "left":"a", "right":"b" })"));
            nary_con->add_sub(parser.parse_ExpStr(R"(false)"));
            auto nary_dis = std::make_unique<NaryExpression>(BinaryOpExpression::OR);
            nary_dis->reserve(3);
            nary_dis->add_sub(parser.parse_ExpStr(R"("a")"));
            nary_dis->add_sub(parser.parse_ExpStr(R"("b")"));
            nary_dis->add_sub(parser.parse_ExpStr(R"("c")"));
            nary_con->add_sub(std::move(nary_dis));

            TO_NORMALFORM::to_nary(to_nary_expr);
            TS_ASSERT(nary_con->equals(to_nary_expr.get()));
            AST_STANDARDIZATION::standardize_ast(to_nary_expr);
            TS_ASSERT(to_nary_expr_ref->equals(to_nary_expr.get()))

        }

        /* to-nary encapsulated. */
        {
            auto to_nary_expr = parser.parse_ExpStr(R"({ "op": "¬", "exp": { "left": "a", "op":"∨", "right": { "op":"∨", "left":"b", "right":"c" } } })");
            SemanticsChecker::check_semantics(to_nary_expr.get());

            auto to_nary_expr_ref = to_nary_expr->deepCopy_Exp();

            auto nary_dis = std::make_unique<NaryExpression>(BinaryOpExpression::OR);
            nary_dis->reserve(3);
            nary_dis->add_sub(parser.parse_ExpStr(R"("a")"));
            nary_dis->add_sub(parser.parse_ExpStr(R"("b")"));
            nary_dis->add_sub(parser.parse_ExpStr(R"("c")"));

            auto nary_encapsulated = std::make_unique<UnaryOpExpression>(UnaryOpExpression::NOT);
            nary_encapsulated->set_operand(std::move(nary_dis));

            TO_NORMALFORM::to_nary(to_nary_expr);
            TS_ASSERT(nary_encapsulated->equals(to_nary_expr.get()));
            AST_STANDARDIZATION::standardize_ast(to_nary_expr);
            TS_ASSERT(to_nary_expr_ref->equals(to_nary_expr.get()))

        }

    }

    void test_to_nf_with_nary() {
        PLAJA_LOG("Test to nf with nary:")

        { /* To dnf with top nary. */
            auto expr_to_dnf = parser.parse_ExpStr(R"({ "left": { "op":"∨", "left":"a", "right":"b" }, "op":"∧", "right": { "left": "c", "op":"∧", "right": "d" } })");
            SemanticsChecker::check_semantics(expr_to_dnf.get());

            TO_NORMALFORM::to_nary(expr_to_dnf);
            TO_NORMALFORM::to_dnf(expr_to_dnf);

            auto expr_to_dnf_ref = parser.parse_ExpStr(R"({ "left": { "op":"∧", "left":"c", "right": { "left": "d", "op":"∧", "right": "a" } }, "op":"∨", "right": { "op":"∧", "left":"c", "right": { "left": "d", "op":"∧", "right": "b" } } })");
            TO_NORMALFORM::to_nary(expr_to_dnf_ref);
            TS_ASSERT(expr_to_dnf_ref->equals(expr_to_dnf.get()))
        }

        { /* To cnf with bottom nary. */
            auto expr_to_cnf = parser.parse_ExpStr(R"({ "left": "a", "op":"∨", "right": { "op":"∧", "left":"b", "right": { "left": "c", "op":"∧", "right": "d" } } })");
            SemanticsChecker::check_semantics(expr_to_cnf.get());

            TO_NORMALFORM::to_nary(expr_to_cnf);
            TO_NORMALFORM::to_cnf(expr_to_cnf);

            auto expr_to_cnf_ref = parser.parse_ExpStr(R"({ "left": { "op":"∨", "left":"a", "right":"b" }, "op":"∧", "right": { "left": { "op":"∨", "left":"a", "right":"c" }, "op":"∧", "right": { "op":"∨", "left":"a", "right":"d" } } })");
            TO_NORMALFORM::to_nary(expr_to_cnf_ref);
            TS_ASSERT(expr_to_cnf_ref->equals(expr_to_cnf.get()))
        }

        { /* To dnf nary-nary */

            auto expr_to_dnf = parser.parse_ExpStr(R"({ "left": { "op":"∨", "left":"a", "right": { "op": "∨", "left": "b", "right": "c" } }, "op":"∧", "right": { "op":"∧", "left":"d", "right":"e" } })");
            SemanticsChecker::check_semantics(expr_to_dnf.get());

            TO_NORMALFORM::to_nary(expr_to_dnf);
            TO_NORMALFORM::to_dnf(expr_to_dnf);

            auto expr_to_dnf_ref = parser.parse_ExpStr(R"({ "left": { "op":"∧", "left":"d", "right": { "left": "e", "op":"∧", "right": "a" } }, "op":"∨", "right": { "op":"∨", "left": { "op":"∧", "left":"d", "right": { "left": "e", "op":"∧", "right": "b" } },  "right": { "op":"∧", "left":"d", "right": { "left": "e", "op":"∧", "right": "c" } } } })");
            TO_NORMALFORM::to_nary(expr_to_dnf_ref);
            TS_ASSERT(expr_to_dnf_ref->equals(expr_to_dnf.get()))

        }

        { /* To cnf complex. */

            /*
             * f | g | (a & b & c) | (d & e)
             * f | g | ( (a | (d & e) & (b | (d & e) & (c | (d & e) )
             * f | g | ( (a | d) & (a | e) & (b | d) & (b | e) & (c | d) & (c | e) )
             * (f | g | a | d) & ... & (f | g | c | e)
             */

            auto nary_bottom = std::make_unique<NaryExpression>(BinaryOpExpression::AND);
            nary_bottom->add_sub(parser.parse_ExpStr(R"("a")"));
            nary_bottom->add_sub(parser.parse_ExpStr(R"("b")"));
            nary_bottom->add_sub(parser.parse_ExpStr(R"("c")"));
            SemanticsChecker::check_semantics(nary_bottom.get());

            auto binary_bottom = parser.parse_ExpStr(R"({"left": "d", "op":"∧", "right": "e"})");
            SemanticsChecker::check_semantics(binary_bottom.get());

            auto binary_mid = std::make_unique<BinaryOpExpression>(BinaryOpExpression::OR);
            binary_mid->set_left(std::move(nary_bottom));
            binary_mid->set_right(std::move(binary_bottom));
            SemanticsChecker::check_semantics(binary_mid.get());

            auto nary_top = std::make_unique<NaryExpression>(BinaryOpExpression::OR);
            nary_top->add_sub(parser.parse_ExpStr(R"("f")"));
            nary_top->add_sub(parser.parse_ExpStr(R"("g")"));
            nary_top->add_sub(std::move(binary_mid));
            SemanticsChecker::check_semantics(nary_top.get());

            auto expr_to_cnf = PLAJA_UTILS::cast_unique<Expression>(std::move(nary_top));

            TO_NORMALFORM::to_cnf(expr_to_cnf);

            auto expr_to_cnf_ref = std::make_unique<NaryExpression>(BinaryOpExpression::AND);
            expr_to_cnf_ref->add_sub(parser.parse_ExpStr(R"({ "op":"∨", "left":{ "left": "f", "op":"∨", "right": "g" }, "right": { "left": "a", "op":"∨", "right": "d" } })"));
            expr_to_cnf_ref->add_sub(parser.parse_ExpStr(R"({ "op":"∨", "left":{ "left": "f", "op":"∨", "right": "g" }, "right": { "left": "a", "op":"∨", "right": "e" } })"));
            expr_to_cnf_ref->add_sub(parser.parse_ExpStr(R"({ "op":"∨", "left":{ "left": "f", "op":"∨", "right": "g" }, "right": { "left": "b", "op":"∨", "right": "d" } })"));
            expr_to_cnf_ref->add_sub(parser.parse_ExpStr(R"({ "op":"∨", "left":{ "left": "f", "op":"∨", "right": "g" }, "right": { "left": "b", "op":"∨", "right": "e" } })"));
            expr_to_cnf_ref->add_sub(parser.parse_ExpStr(R"({ "op":"∨", "left":{ "left": "f", "op":"∨", "right": "g" }, "right": { "left": "c", "op":"∨", "right": "d" } })"));
            expr_to_cnf_ref->add_sub(parser.parse_ExpStr(R"({ "op":"∨", "left":{ "left": "f", "op":"∨", "right": "g" }, "right": { "left": "c", "op":"∨", "right": "e" } })"));
            auto expr_to_cnf_ref_cast = PLAJA_UTILS::cast_unique<Expression>(std::move(expr_to_cnf_ref));
            TO_NORMALFORM::to_nary(expr_to_cnf_ref_cast);

            TS_ASSERT(expr_to_cnf_ref_cast->equals(expr_to_cnf.get()))

        }

    }

/**********************************************************************************************************************/

    void run_tests() override {
        RUN_TEST(test_simple())
        RUN_TEST(test_complex())
        RUN_TEST(test_simple_to_dis())
        RUN_TEST(test_complex_to_dis())
        RUN_TEST(test_complex_to_cnf())
        RUN_TEST(test_to_nary())
        RUN_TEST(test_to_nf_with_nary())
    }

};

TEST_MAIN(ToNormalformTest)

#endif //PLAJA_EXPRESSION_SET_TEST_CPP
