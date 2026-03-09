//
// This file is part of the PlaJA code base.
// Copyright (c) (2019 - 2020) Marcel Vinzent.
// See README.md in the top-level directory for licensing information.
//

#ifndef PLAJA_TESTS_MODEL_Z3_TEST_CPP
#define PLAJA_TESTS_MODEL_Z3_TEST_CPP

#include <z3++.h>
#include "../../parser/ast/expression/non_standard/predicate_abstraction_expression.h"
#include "../../parser/ast/model.h"
#include "../../parser/ast/property.h"
#include "../../parser/parser.h"
#include "../../search/predicate_abstraction/smt/model_z3_pa.h"
#include "../../search/predicate_abstraction/successor_generation/successor_generator_pa.h"
#include "../../search/smt/model/model_z3_structures.h"
#include "../../search/smt/solver/smt_solver_z3.h"
#include "../../search/smt/utils/to_z3_expr.h"
#include "../../search/smt/context_z3.h"
#include "../utils/test_header.h"

const std::string filename("../../../tests/test_instances/for_to_z3.jani"); // NOLINT(*-err58-cpp)

class ModelZ3Test: public PLAJA_TEST::TestSuite {

private:
    PLAJA::ModelConstructorForTests modelConstructor;
    std::unique_ptr<ModelZ3PA> modelZ3;
    std::vector<std::unique_ptr<AutomatonZ3>> cachedModelZ3;
    // JANI structures
    std::unique_ptr<Expression> equality1;
    std::unique_ptr<Expression> inequality1;

public:

    ModelZ3Test() {
        // parse test Model
        modelConstructor.get_config().set_bool_option(PLAJA_OPTION::const_vars_to_consts, false); // Manual maintained aux var.
        modelConstructor.construct_model(filename);
        auto* pa_exp = PLAJA_UTILS::cast_ptr<PAExpression>(modelConstructor.constructedModel->get_property(0)->get_propertyExpression());
        modelConstructor.construct_instance(SearchEngineFactory::PaType, 0); // dummy to add options
        //
        modelConstructor.get_config().set_sharable_const(PLAJA::SharableKey::MODEL, modelConstructor.constructedModel.get());
        modelZ3 = std::make_unique<ModelZ3PA>(modelConstructor.get_config(), pa_exp->get_predicates());
        //
        cachedModelZ3 = modelZ3->compute_model(modelZ3->get_src_vars(), modelZ3->get_target_vars());

        // create manual JANI expressions
        modelConstructor.cachedParser->load_local_variables();
        //  x == 5
        equality1 = modelConstructor.cachedParser->parse_ExpStr(R"({"op":"=", "left":"battery", "right":5})");
        // x > 5
        inequality1 = modelConstructor.cachedParser->parse_ExpStr(R"({"op":">", "left":"battery", "right":5})");
    }

    ~ModelZ3Test() override = default;

/**********************************************************************************************************************/

    void test_simple_to_z3() {
        std::cout << std::endl << "Simple test of to-Z3 visitor:" << std::endl;

        ToZ3Expr equality1_z3_pair = modelZ3->to_z3(*equality1, 0);
        ToZ3Expr inequality1_z3_pair = modelZ3->to_z3(*inequality1, 0);

        TS_ASSERT(not equality1_z3_pair.has_auxiliary())
        TS_ASSERT(not inequality1_z3_pair.has_auxiliary())

        // solving to test encoding via intended behavior
        Z3_IN_PLAJA::SMTSolver s(modelZ3->share_context());
        s.add(equality1_z3_pair.primary);
        TS_ASSERT(s.check())
        s.add(inequality1_z3_pair.primary);
        TS_ASSERT(not s.check())

    }

    void test_compute_edge() {
        std::cout << std::endl << "Test on edge to-Z3 computation:" << std::endl;

        const AutomatonZ3* automaton = cachedModelZ3[0].get(); // modelZ3->get_automaton(0)
        const EdgeZ3* edge = automaton->get_edge(0);
        Z3_IN_PLAJA::SMTSolver s(modelZ3->share_context()); // solving to test encoding via intended behavior
        s.add(*edge->get_guard()); // b > 0
        for (size_t i = 0; i < edge->get_destination(0)->size(); ++i) {
            s.add(edge->get_destination(0)->get_assignment(i)); // b' = b - 1
        }
        s.add(modelZ3->get_target_var(3) == 0); // b' == 0
        // std::cout << std::endl << s << std::endl;
        TS_ASSERT(s.check())

        s.push();
        s.add(modelZ3->get_src_var(3) == 1); // b == 1
        TS_ASSERT(s.check())
        s.pop();

        s.push();
        s.add(modelZ3->get_src_var(3) > 1);
        TS_ASSERT(!s.check())
        s.pop();

        s.add(modelZ3->get_target_var(3) > 0); // b' > 0
        TS_ASSERT(!s.check())
    }

    void test_compute_bound() {
        std::cout << std::endl << "Test on edge and variable bound to-Z3 computation:" << std::endl;

        const AutomatonZ3* automaton = cachedModelZ3[0].get(); // modelZ3->get_automaton(0)
        const EdgeZ3* edge = automaton->get_edge(0);
        Z3_IN_PLAJA::SMTSolver s(modelZ3->share_context());
        for (size_t i = 0; i < edge->get_destination(0)->size(); ++i) {
            s.add(edge->get_destination(0)->get_assignment(i));
        }

        // guard do not check the bounds ...
        s.push();
        s.add(modelZ3->get_target_var(3) == -1);  // b' == -1
        TS_ASSERT(s.check())
        s.pop();
        s.push();
        s.add(modelZ3->get_target_var(3) == 5);  // b' == 5 -> b > 5
        TS_ASSERT(s.check())
        s.pop();

        // add target bounds
        modelZ3->register_target_bound(s, 3); // 5 >= b' >= 0
        // now it is unsat ...
        s.push();
        s.add(modelZ3->get_target_var(3) == -1);  // b' == -1
        TS_ASSERT(not s.check())
        s.pop();
        // but we may still violate the src bound ...
        s.add(modelZ3->get_target_var(3) == 5);  // b' == 5 -> b > 5
        TS_ASSERT(s.check())
        // add src bounds
        modelZ3->register_src_bounds(s, true); // i.a., 5 >= b >= 0
        // now it is unsat ...
        TS_ASSERT(not s.check())
    }

    void test_compute_array() {
        std::cout << std::endl << "Test on to-Z3 computation in the context of arrays and predicates:" << std::endl;

        const AutomatonZ3* automaton = cachedModelZ3[0].get(); // modelZ3->get_automaton(0)
        const EdgeZ3* edge = automaton->get_edge(5);
        Z3_IN_PLAJA::SMTSolver s(modelZ3->share_context());
        // for (size_t i = 0; i < edge->get_destination(0)->size(); ++i) {
        //    s.add(edge->get_destination(0)->get_assignment(i));
        //}
        s.add(edge->get_destination(0)->get_assignment(3));

        // test array bounds
        s.push();
        s.add(z3::select(modelZ3->get_target_var(1), 0) == modelZ3->get_context().bool_val(false));
        TS_ASSERT(s.check())
        s.add(z3::select(modelZ3->get_target_var(1), 1) == modelZ3->get_context().bool_val(false)); // now all target array indexes are false -> conflict with assignment to true
        TS_ASSERT(!s.check())
        s.pop();

        // add src predicates
        s.add(modelZ3->get_src_predicate(1)); // aux == 0
        s.add(z3::select(modelZ3->get_target_var(1), 0) == modelZ3->get_context().bool_val(false)); // send[0] = true
        TS_ASSERT(s.check()) // still sat as assignment can be send[1] = true for data[aux=0] = 1

        // test assignment 2
        s.push();
        s.add(edge->get_destination(0)->get_assignment(2)); // send[aux] = true
        TS_ASSERT(!s.check())
        s.pop();

        // back to assignment3
        s.add(*edge->get_guard()); // data[aux = 0] = 1s
        TS_ASSERT(s.check())  // still sat as assignment can be send[1] = true for data[aux=0] = 1
        s.push();
        s.add(z3::select(modelZ3->get_src_var(0), 0) == 0); // yet we can violate the guard
        s.pop();
        // alternatively conflict assignment again
        s.add(z3::select(modelZ3->get_target_var(1), 1) == modelZ3->get_context().bool_val(false)); // send[1] = true
        TS_ASSERT(!s.check())
    }

    void test_compute_predicates_aux() {
        std::cout << std::endl << "Test on to-Z3 computation of auxiliary expressions in the context of predicates:" << std::endl;

        Z3_IN_PLAJA::SMTSolver s(modelZ3->share_context());
        // first check pred aux src
        s.push();
        s.add(modelZ3->get_src_predicate(0)); // 0 <= aux < 2
        s.push();
        s.add(modelZ3->get_src_var(2) == 0); // aux == 0
        TS_ASSERT(s.check()) // sanity test
        s.pop();
        s.push();
        s.add(modelZ3->get_src_var(2) == 2); // aux == 2
        TS_ASSERT(!s.check())
        s.pop();
        s.push();
        s.add(modelZ3->get_src_var(2) == -1); // aux == -1
        TS_ASSERT(!s.check())
        s.pop();
        s.pop();

        // second check pred aux target
        s.push();
        s.add(modelZ3->get_target_predicate(0)); // 0 <= aux' < 2
        s.push();
        s.add(modelZ3->get_target_var(2) == 0); // aux' == 0
        TS_ASSERT(s.check()) // sanity test
        s.pop();
        s.push();
        s.add(modelZ3->get_target_var(2) == 2); // aux' == 2
        TS_ASSERT(!s.check())
        s.pop();
        s.push();
        s.add(modelZ3->get_target_var(2) == -1); // aux' == -1
        TS_ASSERT(not s.check())
        s.pop();
        s.pop();
    }

    void test_compute_predicates() {
        std::cout << std::endl << "Test on predicate to-Z3 computation:" << std::endl;

        Z3_IN_PLAJA::SMTSolver s(modelZ3->share_context());
        // first check on src predicates
        s.push();
        s.add(modelZ3->get_src_predicate(0)); // data[aux] <= z3aux_var where in aux exp: z3aux_var  <= 0.1 <= z3aux_var + 1
        s.add(modelZ3->get_src_predicate(1)); // aux == 0
        s.add(z3::select(modelZ3->get_src_var(0), 0) > 0); // data[0] > 0
        TS_ASSERT(not s.check())
        s.pop();

        // second check with target predicates
        s.add(modelZ3->get_target_predicate(0));
        s.add(!modelZ3->get_target_predicate(1)); // !(aux == 0)
        TS_ASSERT(s.check()) // predicate combination can be fulfilled
        s.add(z3::select(modelZ3->get_src_var(0), 1) > 0); // sanity check on src variables
        TS_ASSERT(s.check()) // sanity check
        s.add(z3::select(modelZ3->get_target_var(0), 1) > 0); // data[1] > 0
        TS_ASSERT(!s.check())
    }

/**********************************************************************************************************************/

    void run_tests() override {
        RUN_TEST(test_simple_to_z3())
        RUN_TEST(test_compute_edge())
        RUN_TEST(test_compute_bound())
        RUN_TEST(test_compute_array())
        RUN_TEST(test_compute_predicates_aux())
        RUN_TEST(test_compute_predicates())
    }

};

TEST_MAIN(ModelZ3Test)

#endif //PLAJA_TESTS_MODEL_Z3_TEST_CPP
