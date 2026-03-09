//
// This file is part of the PlaJA code base.
// Copyright (c) (2019 - 2020) Marcel Vinzent.
// See README.md in the top-level directory for licensing information.
//

#ifndef PLAJA_PREDICATE_ABSTRACTION_TEST_H
#define PLAJA_PREDICATE_ABSTRACTION_TEST_H

#include <memory>
#include <z3++.h>
#include "../../search/predicate_abstraction/heuristic/state_queue.h"
#include "../../search/predicate_abstraction/search_space/search_space_pa_base.h"
#include "../../search/predicate_abstraction/successor_generation/successor_generator_pa.h"
#include "../../search/predicate_abstraction/predicate_abstraction.h"
#include "../utils/test_header.h"

const std::string filename("../../../tests/test_instances/for_pa2.jani"); // NOLINT(*-err58-cpp)

class PredicateAbstractionTest: public PLAJA_TEST::TestSuite {

private:
    PLAJA::ModelConstructorForTests modelConstructor;
    std::unique_ptr<PredicateAbstraction> predicateAbs;

public:

    PredicateAbstractionTest() {
        modelConstructor.construct_model(filename);
        predicateAbs = PLAJA_UTILS::cast_unique<PredicateAbstraction>(modelConstructor.construct_instance(SearchEngineFactory::PaType, 0));
        predicateAbs->initialize();
    }

    ~PredicateAbstractionTest() override = default;

/**********************************************************************************************************************/

    void test_initial_state() {
        std::cout << std::endl << "Test on the initial state:" << std::endl;
        const auto pa_state = predicateAbs->searchSpace->get_abstract_initial_state();

        TS_ASSERT(pa_state->size() == 3)

        TS_ASSERT(pa_state->location_value(0) == 0 // A1
                  and pa_state->predicate_value(0) // x==3
                  and not pa_state->predicate_value(1) // A[x] = 0
        )
    }

    void test_sync() {
        std::cout << std::endl << "Test on predicate abstraction and underlying successor gen. w.r.t. predicates," << std::endl;
        std::cout << "concretely: proper succ. gen. in case of array bounds in predicates." << std::endl;

        predicateAbs->step();
        TS_ASSERT(predicateAbs->frontier->size() == 1)
        for (StateID_type state_id: predicateAbs->frontier->to_list()) {
            const auto pa_state = predicateAbs->searchSpace->get_abstract_state(state_id);

            TS_ASSERT(pa_state->location_value(0) == 1 // A1
                      and pa_state->predicate_value(0) // x==3
                      and not pa_state->predicate_value(1) // A[x] = 0
            )
        }
    }

/**********************************************************************************************************************/

    void run_tests() override {
        RUN_TEST(test_initial_state())
        RUN_TEST(test_sync())
    }

};

TEST_MAIN(PredicateAbstractionTest)

#endif //PLAJA_PREDICATE_ABSTRACTION_TEST_H
