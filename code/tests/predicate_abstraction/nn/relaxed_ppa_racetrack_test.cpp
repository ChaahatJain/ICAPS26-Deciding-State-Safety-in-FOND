//
// This file is part of the PlaJA code base.
// Copyright (c) (2019 - 2020) Marcel Vinzent.
// See README.md in the top-level directory for licensing information.
//

#ifndef PLAJA_TESTS_RELAXED_PPA_RACETRACK_TEST_CPP
#define PLAJA_TESTS_RELAXED_PPA_RACETRACK_TEST_CPP

#include <memory>
#include "../../../search/predicate_abstraction/heuristic/state_queue.h"
#include "../../../search/predicate_abstraction/heuristic/abstract_heuristic.h"
#include "../../../search/predicate_abstraction/pa_states/predicate_state.h"
#include "../../../search/predicate_abstraction/predicate_abstraction.h"
#include "../../../search/predicate_abstraction/search_space/search_space_pa_base.h"
#include "../../../search/smt_nn/solver/smt_solver_marabou.h"
#include "../../utils/test_header.h"

const std::string filename("../../../../tests/test_instances/for_ppa_racetrack.jani"); // NOLINT(*-err58-cpp)

namespace PLAJA { extern bool define_division_by_zero; }

class RelaxedPPARacetrackTest: public PLAJA_TEST::TestSuite {

private:
    // structures
    PLAJA::ModelConstructorForTests modelConstructor;
    std::unique_ptr<PredicateAbstraction> relaxedPPA;

public:

    RelaxedPPARacetrackTest() {
        // parse test Model
        modelConstructor.get_config().set_value_option(PLAJA_OPTION::nn_for_racetrack, "car0_x,car0_y");
        modelConstructor.get_config().set_value_option(PLAJA_OPTION::additional_properties, "../../../../tests/test_instances/tiny_c_car_v_car_relaxed.jani");
        modelConstructor.get_config().set_value_option(PLAJA_OPTION::marabou_mode, "relaxed");
        modelConstructor.get_config().set_value_option(PLAJA_OPTION::heuristic_search, "none");
        modelConstructor.construct_model(filename);

        PLAJA::define_division_by_zero = true; // suppress some issues specific to this benchmark
    }

    ~RelaxedPPARacetrackTest() override = default;

    DELETE_CONSTRUCTOR(RelaxedPPARacetrackTest)

/**********************************************************************************************************************/

    void set_up() override {
        relaxedPPA = PLAJA_UTILS::cast_unique<PredicateAbstraction>(modelConstructor.construct_instance(SearchEngineFactory::PaType, 8));
        relaxedPPA->initialize();
        relaxedPPA->frontier->clear();
        // reset initial state to be unreached
        relaxedPPA->searchSpace->get_node(relaxedPPA->searchSpace->_initialState()).set_unreached();
    }

    void testRelaxedPPASimple() {
        PLAJA_LOG("\nSimple test on relaxed PPA:")

        // set initial state ...
        auto abstract_initial_state = relaxedPPA->searchSpace->get_abstract_initial_state();
        auto node = relaxedPPA->searchSpace->add_node(abstract_initial_state->to_ptr());
        node.set_reached_init();
        relaxedPPA->frontier->push(node);

        // expectation
        TS_ASSERT(not abstract_initial_state->predicate_value(2) and not abstract_initial_state->predicate_value(3))

        while (relaxedPPA->step() == SearchEngine::IN_PROGRESS) {
            StateID_type state_id = relaxedPPA->frontier->top();
            { // for (StateID_type state_id: ppa->frontier) {
                auto pa_state = relaxedPPA->searchSpace->get_abstract_state(state_id);
                // policy will always choose acc (0,0), hence velocity predicates should not change (note: velocity initially >= (0,0), hence also special case transitions due to change)
                TS_ASSERT(abstract_initial_state->predicate_value(2) == pa_state->predicate_value(2)) // not {"left": "car0_dx", "op": "≤", "right": -1}
                TS_ASSERT(abstract_initial_state->predicate_value(3) == pa_state->predicate_value(3)) // not {"left": "car0_dy", "op": "≤", "right": -1}
                if (!(abstract_initial_state->predicate_value(2) == pa_state->predicate_value(2) and abstract_initial_state->predicate_value(3) == pa_state->predicate_value(3))) { break; }
            }
        }

        relaxedPPA->print_statistics();
    }

    void testPPASimple2() {
        PLAJA_LOG("\n2nd simple test on relaxed PPA:")

        // construct new source state:
        auto abstract_state = relaxedPPA->searchSpace->get_abstract_initial_state();
        auto pa_values = abstract_state->to_pa_state_values(true);
        pa_values->set_predicate_value(2, true); // {"left": "car0_dx", "op": "≤", "right": -1}
        pa_values->set_predicate_value(3, true); // {"left": "car0_dy", "op": "≤", "right": -1}
        // set new source state:
        auto node = relaxedPPA->searchSpace->add_node(relaxedPPA->searchSpace->set_abstract_state(*pa_values));
        node.set_reached_init();
        relaxedPPA->frontier->push(node);

        bool velocity_preds_modified = false;
        while (relaxedPPA->step() == SearchEngine::IN_PROGRESS) {
            StateID_type state_id = relaxedPPA->frontier->top();
            { // for (StateID_type state_id: ppa->frontier) {
                auto pa_state = relaxedPPA->searchSpace->get_abstract_state(state_id);
                // policy will always choose acc (0,0), hence velocity predicates should not change besides for special case transitions which set (dx,dy) = (0,0) at once
                if (not pa_state->predicate_value(2) and not pa_state->predicate_value(3)) {
                    velocity_preds_modified = true;
                } else {
                    TS_ASSERT(pa_state->predicate_value(2)) // {"left": "car0_dx", "op": "≤", "right": -1}
                    TS_ASSERT(pa_state->predicate_value(3)) // {"left": "car0_dy", "op": "≤", "right": -1}
                    if (not(pa_state->predicate_value(2) and pa_state->predicate_value(3))) { break; }
                }
            }
        }

        TS_ASSERT(velocity_preds_modified) // clearly, if loop internal tests fail, this test may fail due to not all states being expanded
    }

/**********************************************************************************************************************/

    void run_tests() override {
        RUN_TEST(testRelaxedPPASimple())
        RUN_TEST(testPPASimple2())
    }

};

TEST_MAIN(RelaxedPPARacetrackTest)

#endif //PLAJA_TESTS_RELAXED_PPA_RACETRACK_TEST_CPP
