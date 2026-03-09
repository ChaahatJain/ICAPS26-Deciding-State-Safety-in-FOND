//
// This file is part of the PlaJA code base.
// Copyright (c) (2019 - 2020) Marcel Vinzent.
// See README.md in the top-level directory for licensing information.
//

#ifndef PLAJA_PREDICATE_ABSTRACTION_TEST_H
#define PLAJA_PREDICATE_ABSTRACTION_TEST_H

#include "../utils/test_header.h"
#include <z3++.h>
#include <memory>
#include "../../parser/ast/expression/non_standard/predicate_abstraction_expression.h"
#include "../../parser/ast/model.h"
#include "../../parser/ast/property.h"
#include "../../search/information/model_information.h"
#include "../../search/predicate_abstraction/smt/model_z3_pa.h"
#include "../../search/smt/model/model_z3_structures.h"
#include "../../search/smt/solver/smt_solver_z3.h"
#include "../../search/smt/solution_enumerator/solution_enumerator_z3.h"
#include "../../search/states/state_values.h"
#include "../utils/test_parser.h"

const std::string filename1("../../../tests/test_instances/for_solution_enumerator.jani"); // NOLINT(*-err58-cpp)

class SolutionEnumeratorTest: public PLAJA_TEST::TestSuite {

private:
    PLAJA::ModelConstructorForTests modelConstructor1;
    const PredicatesExpression* predicates;

    std::unique_ptr<ModelZ3PA> modelZ3;
    std::vector<std::unique_ptr<AutomatonZ3>> cachedModelZ3;
    const StateValues* initialState;

public:

    SolutionEnumeratorTest() {
        // parse test Model1
        modelConstructor1.get_config().set_bool_option(PLAJA_OPTION::const_vars_to_consts, false); // Unused var.
        modelConstructor1.construct_model(filename1);

        predicates = PLAJA_UTILS::cast_ptr<PAExpression>(modelConstructor1.constructedModel->get_property(0)->get_propertyExpression())->get_predicates(); // NOLINT(cppcoreguidelines-prefer-member-initializer)
        PLAJA_ASSERT(predicates)
        modelConstructor1.construct_instance(SearchEngineFactory::PaType, 0); // dummy to add options

        PLAJA::Configuration config(modelConstructor1.optionParser);
        config.set_sharable_const(PLAJA::SharableKey::MODEL, modelConstructor1.constructedModel.get());
        modelZ3 = std::make_unique<ModelZ3PA>(config, predicates);
        //
        cachedModelZ3 = modelZ3->compute_model(modelZ3->get_src_vars(), modelZ3->get_target_vars());

        const ModelInformation& model_information = modelConstructor1.constructedModel->get_model_information();
        initialState = &model_information.get_initial_values();

        StateValues::suppress_written_to(true);
    }

    ~SolutionEnumeratorTest() override = default;

/**********************************************************************************************************************/

    // Tests on Model1

    void test_sol_enumerator_simple() {
        std::cout << std::endl << "Test solution enumerator on a empty and a singleton abstract state:" << std::endl;

        Z3_IN_PLAJA::SMTSolver solver(modelZ3->share_context());
        SolutionEnumeratorZ3 solution_enumerator_z3(solver, *modelZ3);
        solution_enumerator_z3.set_variables_of_interest(std::vector<VariableID_type> { 0, 1 }, true);

        // source constraints; (no target bounds added)
        modelZ3->add_src_bounds(solver, true);
        solver.add(cachedModelZ3[0]->get_edge(0)->get_destination(0)->get_assignment(0));
        solver.add(cachedModelZ3[0]->get_edge(0)->get_destination(0)->get_assignment(1));

        StateValues dummy_state = *initialState;
        // sat
        solver.push();
        solver.add(modelZ3->get_target_predicate(0));
        TS_ASSERT(solution_enumerator_z3.retrieve_solution(dummy_state))
        dummy_state.dump();
        TS_ASSERT(not solution_enumerator_z3.retrieve_solution(dummy_state))
        // unsat
        solver.pop();
        solver.add(!modelZ3->get_target_predicate(0));
        TS_ASSERT(not solution_enumerator_z3.retrieve_solution(dummy_state))
        dummy_state.dump();
    }

    void test_sol_enumerator_advanced() {
        std::cout << std::endl << "Test solution enumerator on an abstract state of size greater 1:" << std::endl;

        Z3_IN_PLAJA::SMTSolver solver(modelZ3->share_context());
        SolutionEnumeratorZ3 solution_enumerator_z3(solver, *modelZ3);
        solution_enumerator_z3.set_variables_of_interest(std::vector<VariableID_type> { 0, 1, 2, 3 }, true);

        // target bounds
        modelZ3->add_bounds(solver, 1, true);

        // predicates
        solver.add(modelZ3->get_target_predicate(1));
        solver.add(modelZ3->get_target_predicate(2));
        solver.add(modelZ3->get_target_predicate(3));
        solver.add(modelZ3->get_target_predicate(4));

        StateValues dummy_state = *initialState;
        // sat
        for (std::size_t i = 0; i < 2 * 1 * 3 * 4; ++i) {
            TS_ASSERT(solution_enumerator_z3.retrieve_solution(dummy_state))
            dummy_state.dump();
            TS_ASSERT(dummy_state.get_int(1) > 3) // x > 3
            TS_ASSERT(dummy_state.get_int(2)) // y
            TS_ASSERT(dummy_state.get_int(4) == 2) // X[1] == 2
            TS_ASSERT(dummy_state.get_int(6)) // Y[1]
        }
        // unsat
        TS_ASSERT(not solution_enumerator_z3.retrieve_solution(dummy_state))
    }

    void test_sol_enumerator_unused_bools() {
        std::cout << std::endl << "Test solution enumerator on an abstract state with bool variables not encoded but of interest:" << std::endl;

        Z3_IN_PLAJA::SMTSolver solver(modelZ3->share_context());
        SolutionEnumeratorZ3 solution_enumerator_z3(solver, *modelZ3);
        solution_enumerator_z3.set_variables_of_interest(std::vector<VariableID_type> { 3, 1 }, true);

        // target bounds
        modelZ3->add_bounds(solver, 1, true);

        StateValues dummy_state = *initialState;
        // sat
        for (std::size_t i = 0; i < 8 * 2; ++i) {
            TS_ASSERT(solution_enumerator_z3.retrieve_solution(dummy_state))
            dummy_state.dump();
        }
        // unsat
        TS_ASSERT(not solution_enumerator_z3.retrieve_solution(dummy_state))
    }

    void test_sol_enumerator_unused_ints() {
        std::cout << std::endl << "Test solution enumerator on an abstract state with bounded int variables not encoded but of interest:" << std::endl;

        Z3_IN_PLAJA::SMTSolver solver(modelZ3->share_context());
        SolutionEnumeratorZ3 solution_enumerator_z3(solver, *modelZ3);
        solution_enumerator_z3.set_variables_of_interest(std::vector<VariableID_type> { 2, 0 }, false);

        // source bounds
        modelZ3->add_src_bounds(solver, false);

        StateValues dummy_state = *initialState;
        // sat
        for (std::size_t i = 0; i < 9 * 6; ++i) {
            TS_ASSERT(solution_enumerator_z3.retrieve_solution(dummy_state))
            dummy_state.dump();
        }
        // unsat
        TS_ASSERT(not solution_enumerator_z3.retrieve_solution(dummy_state))
    }

/**********************************************************************************************************************/

    void run_tests() override {
        RUN_TEST(test_sol_enumerator_simple())
        RUN_TEST(test_sol_enumerator_advanced())
        RUN_TEST(test_sol_enumerator_unused_bools())
        RUN_TEST(test_sol_enumerator_unused_ints())
    }

};

TEST_MAIN(SolutionEnumeratorTest)

#endif //PLAJA_PREDICATE_ABSTRACTION_TEST_H
