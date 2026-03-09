//
// This file is part of the PlaJA code base.
// Copyright (c) (2019 - 2023) Marcel Vinzent.
// See README.md in the top-level directory for licensing information.
//

#ifndef PLAJA_TESTS_NN_EXPLORER_TEST_CPP
#define PLAJA_TESTS_NN_EXPLORER_TEST_CPP

#include <memory>
#include "../../option_parser/plaja_options.h"
#include "../../search/non_prob_search/space_explorer.h"
#include "../../search/states/state_values.h"
#include "../../stats/stats_base.h"
#include "../../utils/structs_utils/update_op_structure.h"
#include "../utils/test_header.h"
#include "../utils/test_parser.h"

const std::string filename("../../../tests/test_instances/blocksworld/cost_terminal/models/legacy_enc/blocksworld_4_3.jani"); // NOLINT(*-err58-cpp)

class ExplorerTest: public PLAJA_TEST::TestSuite {

private:
    PLAJA::ModelConstructorForTests modelConstructor;

public:

    ExplorerTest() {
        modelConstructor.set_constant("failing_probability", "0");
        modelConstructor.set_constant("item_cost_bound", "1");
    }

    ~ExplorerTest() override = default;

    DELETE_CONSTRUCTOR(ExplorerTest)

    void test_explorer_goal() {

        modelConstructor.get_config().set_flag(PLAJA_OPTION::pa_objective, true);
        modelConstructor.get_config().set_flag(PLAJA_OPTION::ignore_nn, true);
        modelConstructor.construct_model(filename);
        auto explorer = (modelConstructor.construct_instance_cast<SpaceExplorer>(SearchEngineFactory::ExplorerType, 1));
        explorer->search();
        explorer->compute_goal_path_states();
        TS_ASSERT_EQUALS(explorer->get_stats().get_attr_unsigned(PLAJA::StatsUnsigned::GOAL_STATES), 1)
        TS_ASSERT_EQUALS(explorer->get_stats().get_attr_unsigned(PLAJA::StatsUnsigned::GoalPathStates), 125)

        const StateValues goal_state(std::vector<PLAJA::integer> { 0, /* */ 1, 1, /* */  1, 2, 3, 4, /**/ 0, 0, 0, 1, /**/ 0, 0, 0, 0, 0 });
        TS_ASSERT_EQUALS(explorer->get_goal_distance(explorer->search_goal_path(goal_state)), 0)

        const StateValues initial_state(std::vector<PLAJA::integer> { 0, /* */ 1, 1, /* */ 2, 3, 4, 1, /**/ 1, 0, 0, 0, /**/ 0, 0, 0, 0, 0 });
        TS_ASSERT_EQUALS(explorer->get_goal_distance(explorer->search_goal_path(initial_state)), 8)
        TS_ASSERT_EQUALS(explorer->get_label_name(explorer->get_label(explorer->get_update_op(explorer->get_goal_op(explorer->search_goal_path(initial_state))).actionOpID)), "choose_block_0");

    }

/**********************************************************************************************************************/

    void run_tests() override {
        RUN_TEST(test_explorer_goal())
    }

};

TEST_MAIN(ExplorerTest)

#endif //PLAJA_TESTS_NN_EXPLORER_TEST_CPP
