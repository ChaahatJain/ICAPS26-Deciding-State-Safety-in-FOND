//
// This file is part of the PlaJA code base.
// Copyright (c) (2019 - 2023) Marcel Vinzent.
// See README.md in the top-level directory for licensing information.
//

#ifndef PLAJA_TESTS_NN_EXPLORER_TEST_CPP
#define PLAJA_TESTS_NN_EXPLORER_TEST_CPP

#include <memory>
#include "../../option_parser/plaja_options.h"
#include "../../search/non_prob_search/nn_explorer.h"
#include "../../stats/stats_base.h"
#include "../utils/test_header.h"
#include "../utils/test_parser.h"

const std::string filename("../../../tests/test_instances/blocksworld/cost_terminal/models/legacy_enc/blocksworld_4_3.jani"); // NOLINT(*-err58-cpp)

class NnExplorerTest: public PLAJA_TEST::TestSuite {

private:
    PLAJA::ModelConstructorForTests modelConstructor;

public:

    NnExplorerTest() {
        modelConstructor.set_constant("failing_probability", "0");
        modelConstructor.set_constant("item_cost_bound", "1");
    }

    ~NnExplorerTest() override = default;

    DELETE_CONSTRUCTOR(NnExplorerTest)

    void test_nn_explorer_goal() {
        modelConstructor.get_config().set_flag(PLAJA_OPTION::pa_objective, true);
        modelConstructor.construct_model(filename);
        auto explorer = modelConstructor.construct_instance_cast<NN_Explorer>(SearchEngineFactory::NnExplorerType, 1);
        explorer->search();
        TS_ASSERT_EQUALS(explorer->get_stats().get_attr_unsigned(PLAJA::StatsUnsigned::GoalPathStates), 125)
        TS_ASSERT_EQUALS(explorer->get_stats().get_attr_unsigned(PLAJA::StatsUnsigned::InevitableGoalPathStates), 1)
        TS_ASSERT_EQUALS(explorer->get_stats().get_attr_unsigned(PLAJA::StatsUnsigned::InevitableGoalPathStatesStart), 0)
        TS_ASSERT_EQUALS(explorer->get_stats().get_attr_unsigned(PLAJA::StatsUnsigned::PolicyGoalPathStates), 125)
        TS_ASSERT_EQUALS(explorer->get_stats().get_attr_unsigned(PLAJA::StatsUnsigned::PolicyGoalPathStatesStart), 14)
        TS_ASSERT_EQUALS(explorer->get_stats().get_attr_unsigned(PLAJA::StatsUnsigned::EvitablePolicyGoalPathStates), 124)
    }

    void test_nn_explorer_avoid() {
        modelConstructor.get_config().set_flag(PLAJA_OPTION::pa_objective, false);
        modelConstructor.construct_model(filename);
        auto explorer = modelConstructor.construct_instance_cast<NN_Explorer>(SearchEngineFactory::NnExplorerType, 1);
        explorer->search();
        TS_ASSERT_EQUALS(explorer->get_stats().get_attr_unsigned(PLAJA::StatsUnsigned::GoalPathStates), 125)
        TS_ASSERT_EQUALS(explorer->get_stats().get_attr_unsigned(PLAJA::StatsUnsigned::InevitableGoalPathStates), 1)
        TS_ASSERT_EQUALS(explorer->get_stats().get_attr_unsigned(PLAJA::StatsUnsigned::InevitableGoalPathStatesStart), 0)
        TS_ASSERT_EQUALS(explorer->get_stats().get_attr_unsigned(PLAJA::StatsUnsigned::PolicyGoalPathStates), 1)
        TS_ASSERT_EQUALS(explorer->get_stats().get_attr_unsigned(PLAJA::StatsUnsigned::PolicyGoalPathStatesStart), 0)
        TS_ASSERT_EQUALS(explorer->get_stats().get_attr_unsigned(PLAJA::StatsUnsigned::EvitablePolicyGoalPathStates), 0)
    }

/**********************************************************************************************************************/

    void run_tests() override {
        RUN_TEST(test_nn_explorer_goal())
        RUN_TEST(test_nn_explorer_avoid())
    }

};

TEST_MAIN(NnExplorerTest)

#endif //PLAJA_TESTS_NN_EXPLORER_TEST_CPP
