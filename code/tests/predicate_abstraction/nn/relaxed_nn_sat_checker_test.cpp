//
// This file is part of the PlaJA code base.
// Copyright (c) (2019 - 2020) Marcel Vinzent.
// See README.md in the top-level directory for licensing information.
//

#ifndef PLAJA_TESTS_NN_SAT_CHECKER_TEST_CPP
#define PLAJA_TESTS_NN_SAT_CHECKER_TEST_CPP

#include <memory>
#include "../../../option_parser/enum_option_values_set.h"
#include "../../../search/factories/predicate_abstraction/pa_options.h"
#include "../../../search/fd_adaptions/state.h"
#include "../../../search/predicate_abstraction/nn/nn_sat_checker/nn_sat_checker.h"
#include "../../../search/predicate_abstraction/pa_states/predicate_state.h"
#include "../../../search/predicate_abstraction/search_space/search_space_pa_base.h"
#include "../../../search/predicate_abstraction/pa_transition_structure.h"
#include "../../../search/predicate_abstraction/predicate_abstraction.h"
#include "../../../search/smt_nn/solver/smt_solver_marabou.h"
#include "../../utils/test_header.h"

const std::string filename("../../../../tests/test_instances/for_nn_sat.jani"); // NOLINT(*-err58-cpp)

class NNSatCheckerTest: public PLAJA_TEST::TestSuite {

private:
    // structures
    PLAJA::ModelConstructorForTests modelConstructor;
    std::unique_ptr<SearchPABase> base;

    PATransitionStructure transStruct;
    NNSatChecker* nnSatChecker;
    // to construct states:
    std::unique_ptr<AbstractState> abstractInitialState;

public:

    NNSatCheckerTest():
        nnSatChecker(nullptr) {
        // parse test Model
        modelConstructor.get_config().set_bool_option(PLAJA_OPTION::const_vars_to_consts, false); // Const var is part of input interface.
        modelConstructor.get_config().set_bool_option(PLAJA_OPTION::predicate_relations, false);
        modelConstructor.get_config().set_enum_option(PLAJA_OPTION::marabou_mode, PLAJA_OPTION::EnumOptionValuesSet::construct_singleton(PLAJA_OPTION::EnumOptionValue::Relaxed));
        modelConstructor.construct_model(filename);
        base = PLAJA_UTILS::cast_unique<SearchPABase>(modelConstructor.construct_instance(SearchEngineFactory::PaType, 0));

        nnSatChecker = base->get_nn_sat_checker();
        nnSatChecker->shared_transition_structure(transStruct);

        // set initial state
        PredicateState predicate_state(4, 1);
        predicate_state.get_location_state().set_int(0, 0);
        predicate_state.set(0, false);
        predicate_state.set(1, false);
        predicate_state.set(2, true);
        predicate_state.set(3, false);
        abstractInitialState = base->searchSpace->set_abstract_state(predicate_state);
        abstractInitialState->drop_entailment_information();

        State::suppress_written_to(true); // as tests modify state objects
    }

    ~NNSatCheckerTest() override = default;

    DELETE_CONSTRUCTOR(NNSatCheckerTest)

/**********************************************************************************************************************/

    void test_nn_unsat_check() {
        std::cout << std::endl << "Test relaxed NN-Sat check:" << std::endl;

        // first, second false implies first action, but also second in the linear-relaxed case
        nnSatChecker->push();
        transStruct.set_source(abstractInitialState->to_ptr());
        nnSatChecker->add_source_state();
        nnSatChecker->push();
        transStruct.set_action_label(0);
        nnSatChecker->add_output_interface();
        TS_ASSERT_EQUALS(nnSatChecker->check(), true) // here it is crucial, as the third predicate constraint is true, we can fulfill it with x=0 (and thus action 1 in the NN), since we can assign A[1] = -1
        nnSatChecker->pop();
        nnSatChecker->push();
        transStruct.set_action_label(1);
        nnSatChecker->add_output_interface();
        TS_ASSERT_EQUALS(nnSatChecker->check(), true)
        nnSatChecker->pop();
        nnSatChecker->push();
        transStruct.set_action_label(2);
        nnSatChecker->add_output_interface();
        TS_ASSERT_EQUALS(nnSatChecker->check(), false) // third is not due to negative bias
        nnSatChecker->pop();
        nnSatChecker->pop();

        // first predicate true, second false implies second action, (but also third in the linear-relaxed case: no longer due to bound optimization)
        auto pa_values = abstractInitialState->to_pa_state_values(true);
        pa_values->set_predicate_value(0, true);
        auto tmp_abstract_state_1 = base->searchSpace->set_abstract_state(*pa_values);
        tmp_abstract_state_1->drop_entailment_information();
        nnSatChecker->push();
        transStruct.set_source(std::move(tmp_abstract_state_1));
        nnSatChecker->add_source_state();
        nnSatChecker->push();
        transStruct.set_action_label(0);
        nnSatChecker->add_output_interface();
        TS_ASSERT_EQUALS(nnSatChecker->check(), false)
        nnSatChecker->pop();
        nnSatChecker->push();
        transStruct.set_action_label(1);
        nnSatChecker->add_output_interface();
        TS_ASSERT_EQUALS(nnSatChecker->check(), true)
        nnSatChecker->pop();
        nnSatChecker->push();
        transStruct.set_action_label(2);
        nnSatChecker->add_output_interface();
        TS_ASSERT_EQUALS(nnSatChecker->check(), false)
        nnSatChecker->pop();
        nnSatChecker->pop();

        // first,second predicate true implies third action
        pa_values->set_predicate_value(1, true);
        auto tmp_abstract_state_2 = base->searchSpace->set_abstract_state(*pa_values);
        tmp_abstract_state_2->drop_entailment_information();
        nnSatChecker->push();
        transStruct.set_source(std::move(tmp_abstract_state_2));
        nnSatChecker->add_source_state();
        nnSatChecker->push();
        transStruct.set_action_label(0);
        nnSatChecker->add_output_interface();
        TS_ASSERT_EQUALS(nnSatChecker->check(), false)
        nnSatChecker->pop();
        nnSatChecker->push();
        transStruct.set_action_label(1);
        nnSatChecker->add_output_interface();
        TS_ASSERT_EQUALS(nnSatChecker->check(), false)
        nnSatChecker->pop();
        nnSatChecker->push();
        transStruct.set_action_label(2);
        nnSatChecker->add_output_interface();
        TS_ASSERT_EQUALS(nnSatChecker->check(), true)
        nnSatChecker->pop();
        nnSatChecker->pop();

        // first false, second predicate true is unsat
        pa_values->set_predicate_value(0, false);
        auto tmp_abstract_state_3 = base->searchSpace->set_abstract_state(*pa_values);
        tmp_abstract_state_3->drop_entailment_information();
        nnSatChecker->push();
        transStruct.set_source(std::move(tmp_abstract_state_3));
        nnSatChecker->add_source_state();
        nnSatChecker->push();
        transStruct.set_action_label(0);
        nnSatChecker->add_output_interface();
        TS_ASSERT_EQUALS(nnSatChecker->check(), false)
        nnSatChecker->pop();
        nnSatChecker->push();
        transStruct.set_action_label(1);
        nnSatChecker->add_output_interface();
        TS_ASSERT_EQUALS(nnSatChecker->check(), false)
        nnSatChecker->pop();
        nnSatChecker->push();
        transStruct.set_action_label(2);
        nnSatChecker->add_output_interface();
        TS_ASSERT_EQUALS(nnSatChecker->check(), false)
        nnSatChecker->pop();
        nnSatChecker->pop();

    }

/**********************************************************************************************************************/

    void run_tests() override { RUN_TEST(test_nn_unsat_check()) }

};

TEST_MAIN(NNSatCheckerTest)

#endif //PLAJA_TESTS_NN_SAT_CHECKER_TEST_CPP
