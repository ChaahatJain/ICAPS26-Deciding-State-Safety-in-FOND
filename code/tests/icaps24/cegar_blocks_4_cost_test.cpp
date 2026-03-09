//
// This file is part of the PlaJA code base.
// Copyright (c) (2019 - 2024) Marcel Vinzent.
// See README.md in the top-level directory for licensing information.
//

#include <memory>
#include "../../option_parser/enum_option_values_set.h"
#include "../../option_parser/plaja_options.h"
#include "../../search/factories/predicate_abstraction/pa_options.h"
#include "../../search/predicate_abstraction/cegar/pa_cegar.h"
#include "../../stats/stats_base.h"
#include "../utils/test_header.h"
#include "../utils/test_parser.h"

const std::string modelFile("../../../tests/test_instances/blocksworld/cost_terminal/models/legacy_enc/blocksworld_4_3.jani"); // NOLINT(*-err58-cpp)
// const std::string nnInterfaceFile("../../../tests/test_instances/rddps22/blocksworld/networks/learned_cost05_non_terminal/blocksworld_4_3_16_16.jani2nnet"); // NOLINT(*-err58-cpp)

class CegarBlock4CostTest: public PLAJA_TEST::TestSuite {

private:
    PLAJA::ModelConstructorForTests modelConstructor;

public:

    CegarBlock4CostTest() {
        modelConstructor.set_constant("failing_probability", "0.5");
        modelConstructor.set_constant("item_cost_bound", "50");
        // modelConstructor.get_config().set_value_option(PLAJA_OPTION::nn_interface, nnInterfaceFile);

        modelConstructor.get_config().set_bool_option(PLAJA_OPTION::check_policy_spuriousness, true);

        modelConstructor.construct_model(modelFile);
    }

    ~CegarBlock4CostTest() override = default;

    DELETE_CONSTRUCTOR(CegarBlock4CostTest)

    void test_nn_16_vea() {
        PLAJA_LOG("--- Running no-filtering ---\n")

        modelConstructor.config.set_int_option(PLAJA_OPTION::applicability_filtering, -1);
        modelConstructor.get_config().set_bool_option(PLAJA_OPTION::set_pa_goal_objective_terminal, false);
        modelConstructor.get_config().set_bool_option(PLAJA_OPTION::check_for_pa_terminal_states, false);

        auto cegar = modelConstructor.construct_instance_cast<PACegar>(SearchEngineFactory::PaCegarType, 1);
        cegar->search();
        TS_ASSERT_EQUALS(cegar->get_stats().get_attr_int(PLAJA::StatsInt::HAS_GOAL_PATH), 0)
        TS_ASSERT_EQUALS(cegar->get_stats().get_attr_unsigned(PLAJA::StatsUnsigned::ITERATIONS), 8)
        TS_ASSERT_EQUALS(cegar->get_stats().get_attr_unsigned(PLAJA::StatsUnsigned::REF_DUE_TO_GUARD_APP), 4)
        TS_ASSERT_EQUALS(cegar->get_stats().get_attr_unsigned(PLAJA::StatsUnsigned::REF_DUE_TO_ACTION_SEL), 1)
        TS_ASSERT_EQUALS(cegar->get_stats().get_attr_unsigned(PLAJA::StatsUnsigned::REF_DUE_TO_TARGET), 2)
        TS_ASSERT_EQUALS(cegar->get_stats().get_attr_unsigned(PLAJA::StatsUnsigned::PREDICATES_FOR_ACTION_SEL_REF), 11)
        TS_ASSERT_EQUALS(cegar->get_stats().get_attr_unsigned(PLAJA::StatsUnsigned::EntailedSplits), 12)
        /**/
        TS_ASSERT_EQUALS(cegar->get_stats_ppa().get_attr_unsigned(PLAJA::StatsUnsigned::PREDICATES), 22)
        TS_ASSERT_EQUALS(cegar->get_stats_ppa().get_attr_int(PLAJA::StatsInt::PATH_LENGTH), -1)
        TS_ASSERT_EQUALS(cegar->get_stats_ppa().get_attr_unsigned(PLAJA::StatsUnsigned::START_STATES), 14)
        TS_ASSERT_EQUALS(cegar->get_stats_ppa().get_attr_unsigned(PLAJA::StatsUnsigned::EXPANDED_STATES), 2167)
        TS_ASSERT_EQUALS(cegar->get_stats_ppa().get_attr_unsigned(PLAJA::StatsUnsigned::REACHABLE_STATES), 2167)
        TS_ASSERT_EQUALS(cegar->get_stats_ppa().get_attr_unsigned(PLAJA::StatsUnsigned::TRANSITIONS), 2153)
        TS_ASSERT_EQUALS(cegar->get_stats_ppa().get_attr_unsigned(PLAJA::StatsUnsigned::TERMINAL_STATES), 715)
        TS_ASSERT_EQUALS(cegar->get_stats_ppa().get_attr_unsigned(PLAJA::StatsUnsigned::GENERATED_STATES), 12775)

    }

    void test_nn_16_app() {
        PLAJA_LOG("\n--- Running applicability-filtering ---\n")

        modelConstructor.config.set_int_option(PLAJA_OPTION::applicability_filtering, 1);
        modelConstructor.get_config().set_bool_option(PLAJA_OPTION::set_pa_goal_objective_terminal, true);
        modelConstructor.get_config().set_bool_option(PLAJA_OPTION::check_for_pa_terminal_states, true);

        auto cegar = modelConstructor.construct_instance_cast<PACegar>(SearchEngineFactory::PaCegarType, 1);
        cegar->search();
        TS_ASSERT_EQUALS(cegar->get_stats().get_attr_int(PLAJA::StatsInt::HAS_GOAL_PATH), 0)
        TS_ASSERT_EQUALS(cegar->get_stats().get_attr_unsigned(PLAJA::StatsUnsigned::ITERATIONS), 8)
        TS_ASSERT_EQUALS(cegar->get_stats().get_attr_unsigned(PLAJA::StatsUnsigned::REF_DUE_TO_GUARD_APP), 4)
        TS_ASSERT_EQUALS(cegar->get_stats().get_attr_unsigned(PLAJA::StatsUnsigned::REF_DUE_TO_ACTION_SEL), 1)
        TS_ASSERT_EQUALS(cegar->get_stats().get_attr_unsigned(PLAJA::StatsUnsigned::REF_DUE_TO_TARGET), 2)
        TS_ASSERT_EQUALS(cegar->get_stats().get_attr_unsigned(PLAJA::StatsUnsigned::PREDICATES_FOR_ACTION_SEL_REF), 11)
        TS_ASSERT_EQUALS(cegar->get_stats().get_attr_unsigned(PLAJA::StatsUnsigned::EntailedSplits), 12)
        /**/
        TS_ASSERT_EQUALS(cegar->get_stats_ppa().get_attr_unsigned(PLAJA::StatsUnsigned::PREDICATES), 22)
        TS_ASSERT_EQUALS(cegar->get_stats_ppa().get_attr_int(PLAJA::StatsInt::PATH_LENGTH), -1)
        TS_ASSERT_EQUALS(cegar->get_stats_ppa().get_attr_unsigned(PLAJA::StatsUnsigned::START_STATES), 14)
        TS_ASSERT_EQUALS(cegar->get_stats_ppa().get_attr_unsigned(PLAJA::StatsUnsigned::EXPANDED_STATES), 3026)
        TS_ASSERT_EQUALS(cegar->get_stats_ppa().get_attr_unsigned(PLAJA::StatsUnsigned::REACHABLE_STATES), 3026)
        TS_ASSERT_EQUALS(cegar->get_stats_ppa().get_attr_unsigned(PLAJA::StatsUnsigned::TRANSITIONS), 3012)
        TS_ASSERT_EQUALS(cegar->get_stats_ppa().get_attr_unsigned(PLAJA::StatsUnsigned::TERMINAL_STATES), 1076)
        TS_ASSERT_EQUALS(cegar->get_stats_ppa().get_attr_unsigned(PLAJA::StatsUnsigned::GENERATED_STATES), 17843)

    }

/**********************************************************************************************************************/

    void run_tests() override {
        RUN_TEST(test_nn_16_vea())
        RUN_TEST(test_nn_16_app())
    }

};

TEST_MAIN(CegarBlock4CostTest)