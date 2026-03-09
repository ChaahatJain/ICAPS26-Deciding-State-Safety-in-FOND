//
// This file is part of the PlaJA code base.
// Copyright (c) (2019 - 2020) Marcel Vinzent.
// See README.md in the top-level directory for licensing information.
//

#ifndef PLAJA_RELAXED_TESTS_NN_SAT_CHECKER_WITH_UNUSED_VARS_CPP
#define PLAJA_RELAXED_TESTS_NN_SAT_CHECKER_WITH_UNUSED_VARS_CPP

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

class NNSatCheckerUnusedVarsTest: public PLAJA_TEST::TestSuite {

private:
    // structures
    PLAJA::ModelConstructorForTests modelConstructor;
    std::unique_ptr<SearchPABase> base;

    PATransitionStructure transStruct;
    NNSatChecker* nnSatChecker;
    // to construct states:
    std::unique_ptr<AbstractState> abstractInitialState;

public:

    NNSatCheckerUnusedVarsTest() {
        // parse test Model
        modelConstructor.get_config().set_bool_option(PLAJA_OPTION::predicate_relations, false);
        modelConstructor.get_config().set_enum_option(PLAJA_OPTION::marabou_mode, PLAJA_OPTION::EnumOptionValuesSet::construct_singleton(PLAJA_OPTION::EnumOptionValue::Relaxed));
        modelConstructor.construct_model(filename);
        base = PLAJA_UTILS::cast_unique<SearchPABase>(modelConstructor.construct_instance(SearchEngineFactory::PaType, 1));

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

    ~NNSatCheckerUnusedVarsTest() override = default;

    DELETE_CONSTRUCTOR(NNSatCheckerUnusedVarsTest)

/**********************************************************************************************************************/

    void testRelaxedNNSatTransitionCheck() {
        PLAJA_LOG("\nTest relaxed NN-Sat check:")
        auto predicate_state = PredicateState(std::move(*abstractInitialState->to_pa_state_values(true)));

        nnSatChecker->push();
        transStruct.set_action_label(0);
        nnSatChecker->add_output_interface();
        transStruct.set_action_op(1);
        nnSatChecker->set_operator();
        transStruct.set_update(0);
        nnSatChecker->push();
        transStruct.set_source(abstractInitialState->to_ptr());
        nnSatChecker->add_source_state();
        TS_ASSERT_EQUALS(nnSatChecker->check(), true)
        nnSatChecker->add_guard();
        TS_ASSERT_EQUALS(nnSatChecker->check(), false) // guard is unsat (see bound of xx and predicate on x)
        nnSatChecker->pop();
        //
        predicate_state.unset(0);
        predicate_state.set(0, true);
        const auto tmp_abstract_state_1 = base->searchSpace->set_abstract_state(predicate_state);
        tmp_abstract_state_1->drop_entailment_information();
        transStruct.set_source(tmp_abstract_state_1->to_ptr());
        nnSatChecker->add_source_state();
        nnSatChecker->add_guard();
        TS_ASSERT_EQUALS(nnSatChecker->check(), true) // with x >= 1, guard becomes true
        nnSatChecker->add_update();
        TS_ASSERT_EQUALS(nnSatChecker->check(), true) // update adds no restriction
        nnSatChecker->push();
        auto predicate_state_entail = PredicateState(std::move(*abstractInitialState->to_pa_state_values(true)));
        predicate_state_entail.set_entailed(3, predicate_state_entail.predicate_value(3)); // this predicate is invariant
        PLAJA_ASSERT(predicate_state_entail.predicate_value(3) == tmp_abstract_state_1->predicate_value(3))
        transStruct.set_target(base->searchSpace->set_abstract_state(predicate_state_entail)); // hack to construct with (dummy) entailment information
        nnSatChecker->add_target_state();
        TS_ASSERT_EQUALS(nnSatChecker->check(), false) // as updates set x = xxx >= 2, ...
        nnSatChecker->pop();
        // ... x >= 1 must be true:
        nnSatChecker->push();
        auto predicate_state_entail_1 = PredicateState(std::move(*tmp_abstract_state_1->to_pa_state_values(true)));
        predicate_state_entail_1.set_entailed(3, predicate_state_entail_1.predicate_value(3)); // this predicate is invariant
        PLAJA_ASSERT(predicate_state_entail_1.predicate_value(3) == tmp_abstract_state_1->predicate_value(3))
        transStruct.set_target(base->searchSpace->set_abstract_state(predicate_state_entail_1));
        nnSatChecker->add_target_state();
        TS_ASSERT_EQUALS(nnSatChecker->check(), false)
        nnSatChecker->pop();
        // but also 2x >= 4 ...
        predicate_state.unset(1);
        predicate_state.set(1, true);
        predicate_state.set_entailed(3, predicate_state.predicate_value(3));
        PLAJA_ASSERT(predicate_state.predicate_value(3) == tmp_abstract_state_1->predicate_value(3))
        transStruct.set_target(base->searchSpace->set_abstract_state(predicate_state));
        nnSatChecker->add_target_state();
        TS_ASSERT_EQUALS(nnSatChecker->check(), true)
        nnSatChecker->pop();

    }

/**********************************************************************************************************************/

    void run_tests() override { RUN_TEST(testRelaxedNNSatTransitionCheck()) }

};

TEST_MAIN(NNSatCheckerUnusedVarsTest)

#endif //PLAJA_TESTS_RELAXED_NN_SAT_CHECKER_WITH_UNUSED_VARS_CPP
