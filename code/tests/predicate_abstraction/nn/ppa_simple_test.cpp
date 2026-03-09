//
// This file is part of the PlaJA code base.
// Copyright (c) (2019 - 2020) Marcel Vinzent.
// See README.md in the top-level directory for licensing information.
//

#ifndef PLAJA_TESTS_PPA_SIMPLE_TEST_CPP
#define PLAJA_TESTS_PPA_SIMPLE_TEST_CPP

#include <memory>
#include "../../../search/factories/predicate_abstraction/search_engine_config_pa.h"
#include "../../../search/predicate_abstraction/heuristic/state_queue.h"
#include "../../../search/predicate_abstraction/nn/nn_sat_checker/nn_sat_checker_factory.h"
#include "../../../search/predicate_abstraction/pa_states/predicate_state.h"
#include "../../../search/predicate_abstraction/search_space/search_space_pa_base.h"
#include "../../../search/predicate_abstraction/predicate_abstraction.h"
#include "../../../search/smt_nn/solver/smt_solver_marabou.h"
#include "../../utils/test_header.h"

const std::string filename("../../../../tests/test_instances/for_ppa_simple.jani"); // NOLINT(*-err58-cpp)

class PPASimpleTest: public PLAJA_TEST::TestSuite {

private:
    // structures
    PLAJA::ModelConstructorForTests modelConstructor;
    PLAJA::ModelConstructorForTests modelConstructorRelaxed;
    std::unique_ptr<PredicateAbstraction> ppa;
    std::unique_ptr<PredicateAbstraction> ppaRelaxed;

public:

    PPASimpleTest() {
        modelConstructor.construct_model(filename); // parse test Model
        modelConstructorRelaxed.construct_model(filename); // parse test Model
    }

    ~PPASimpleTest() override = default;

/**********************************************************************************************************************/

    void set_up() override {
        modelConstructor.get_config().set_value_option(PLAJA_OPTION::nn_sat, "z3");
        ppa = PLAJA_UTILS::cast_unique<PredicateAbstraction>(modelConstructor.construct_instance(SearchEngineFactory::PaType, 0));
        ppa->initialize();
        // reset initial state to be unreached
        ppa->frontier->clear();
        ppa->searchSpace->get_node(ppa->searchSpace->_initialState()).set_unreached();

        modelConstructorRelaxed.get_config().set_value_option(PLAJA_OPTION::nn_sat, "marabou");
        modelConstructorRelaxed.get_config().set_value_option(PLAJA_OPTION::marabou_mode, "relaxed");
        ppaRelaxed = PLAJA_UTILS::cast_unique<PredicateAbstraction>(modelConstructorRelaxed.construct_instance(SearchEngineFactory::PaType, 0));
        ppaRelaxed->initialize();
        // reset initial state to be unreached
        ppaRelaxed->frontier->clear();
        ppaRelaxed->searchSpace->get_node(ppaRelaxed->searchSpace->_initialState()).set_unreached();

        modelConstructor.get_config().disable_value_option(PLAJA_OPTION::nn_sat);
        modelConstructorRelaxed.get_config().disable_value_option(PLAJA_OPTION::nn_sat);
    }

    void testPPASimple() {
        std::cout << std::endl << "Simple test on PPA:" << std::endl;

        // set initial state ...
        auto node = ppa->searchSpace->add_node(ppa->searchSpace->get_abstract_initial_state());
        node.set_reached_init();
        ppa->frontier->push(node);

        ppa->step();
        TS_ASSERT(ppa->frontier->size() == 1) // note, test on NN-SAT(S,a) but also NN-SAT(S,g,u,a,S) as we have only one successor for the selected action
        const auto pa_state = ppa->searchSpace->get_abstract_state(ppa->frontier->top());
        TS_ASSERT(pa_state->location_value(0) == 1 and pa_state->predicate_value(0) and not pa_state->predicate_value(1))
    }

    void testPPARelaxedSimple() {
        std::cout << std::endl << "Simple test on relaxed PPA:" << std::endl;

        // set initial state ...
        auto node = ppaRelaxed->searchSpace->add_node(ppaRelaxed->searchSpace->get_abstract_initial_state());
        node.set_reached_init();
        ppaRelaxed->frontier->push(node);

        ppaRelaxed->step();
        TS_ASSERT(ppaRelaxed->frontier->size() == 1) // note, test on relaxed NN-SAT(S,a) but *not* NN-SAT(S,g,u,a,S') as SMT(S,g,u,S') filters the result
        const auto pa_state = ppaRelaxed->searchSpace->get_abstract_state(ppaRelaxed->frontier->top());
        TS_ASSERT(pa_state->location_value(0) == 1 and pa_state->predicate_value(0) and not pa_state->predicate_value(1))
    }

    void testPPASimple2() {
        PLAJA_LOG("\n2nd simple test on PPA:")

        // construct new source state:
        auto abstract_state = ppa->searchSpace->get_abstract_initial_state();
        auto pa_values = abstract_state->to_pa_state_values(true);
        pa_values->set_predicate_value(1, true); // {"op":"≥", "left":"x", "right":2}
        // set new source state:
        auto node = ppa->searchSpace->add_node(ppa->searchSpace->set_abstract_state(*pa_values));
        node.set_reached_init();
        ppa->frontier->push(node);

        ppa->step();
        TS_ASSERT(ppa->frontier->empty()) // empty as guard of second action is not enabled for states for which the action is action (\ie we test NN-SAT(S,g,a))

        node.set_unreached(); // also have to set the source state to be unreached ()
    }

    void testPPARelaxedSimple2() {
        PLAJA_LOG("\n2nd simple test on relaxed PPA:")

        // construct new source state:
        auto abstract_state = ppaRelaxed->searchSpace->get_abstract_initial_state();
        auto pa_values = abstract_state->to_pa_state_values(true);
        pa_values->set_predicate_value(1, true); // {"op":"≥", "left":"x", "right":2}
        // set new source state:
        auto node = ppaRelaxed->searchSpace->add_node(ppaRelaxed->searchSpace->set_abstract_state(*pa_values));
        node.set_reached_init();
        ppaRelaxed->frontier->push(node);

        ppaRelaxed->step();
        TS_ASSERT(ppaRelaxed->frontier->empty()) // empty as guard of second action is not enabled for states for which the action is chosen (\ie we test relaxed NN-SAT(S,g,a))

        node.set_unreached(); // also have to set the source state to be unreached
    }

/**********************************************************************************************************************/

    void run_tests() override {
        RUN_TEST(testPPASimple())
        RUN_TEST(testPPARelaxedSimple())
        RUN_TEST(testPPASimple2())
        RUN_TEST(testPPARelaxedSimple2())
    }

};

TEST_MAIN(PPASimpleTest)

#endif //PLAJA_TESTS_PPA_SIMPLE_TEST_CPP
