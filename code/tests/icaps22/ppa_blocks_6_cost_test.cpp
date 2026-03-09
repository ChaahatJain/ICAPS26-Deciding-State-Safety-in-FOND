//
// This file is part of the PlaJA code base.
// Copyright (c) (2019 - 2024) Marcel Vinzent.
// See README.md in the top-level directory for licensing information.
//

#include <memory>
#include "../../option_parser/enum_option_values_set.h"
#include "../../option_parser/plaja_options.h"
#include "../../search/factories/predicate_abstraction/pa_options.h"
#include "../../search/predicate_abstraction/predicate_abstraction.h"
#include "../../stats/stats_base.h"
#include "../utils/test_header.h"
#include "../utils/test_parser.h"

const std::string modelFile("../../../tests/test_instances/icaps22/blocksworld/models/cost05/blocksworld_6_5.jani"); // NOLINT(*-err58-cpp)
const std::string propertyFile("../../../tests/test_instances/icaps22/blocksworld/additional_properties/safety_verification/cost05/blocksworld_6_5/compact_starts_cost_predicates_afterwards/learned_cost05_non_terminal/pa_compact_starts_cost_predicates_afterwards_blocksworld_6_5_32_32.jani"); // NOLINT(*-err58-cpp)

class PpaBlock6CostTest: public PLAJA_TEST::TestSuite {

private:
    PLAJA::ModelConstructorForTests modelConstructor;

public:

    PpaBlock6CostTest() {
        modelConstructor.get_config().set_enum_option(PLAJA_OPTION::pa_state_reachability, PLAJA_OPTION::EnumOptionValuesSet::construct_singleton(PLAJA_OPTION::EnumOptionValue::PaOnlyReachabilityPerSrc));
    }

    ~PpaBlock6CostTest() override = default;

    DELETE_CONSTRUCTOR(PpaBlock6CostTest)

    void test_nn_32_prop_5() {

        modelConstructor.get_config().set_value_option(PLAJA_OPTION::additional_properties, propertyFile);
        modelConstructor.construct_model(modelFile);
        auto ppa = (modelConstructor.construct_instance_cast<PredicateAbstraction>(SearchEngineFactory::PaType, 1));
        ppa->search();
        TS_ASSERT_EQUALS(ppa->get_stats().get_attr_unsigned(PLAJA::StatsUnsigned::START_STATES), 171)
        TS_ASSERT_EQUALS(ppa->get_stats().get_attr_unsigned(PLAJA::StatsUnsigned::EXPANDED_STATES), 433)
        TS_ASSERT_EQUALS(ppa->get_stats().get_attr_unsigned(PLAJA::StatsUnsigned::TRANSITIONS), 1005)
        TS_ASSERT_EQUALS(ppa->get_stats().get_attr_unsigned(PLAJA::StatsUnsigned::REACHABLE_STATES), 433)
        TS_ASSERT_EQUALS(ppa->get_stats().get_attr_unsigned(PLAJA::StatsUnsigned::GOAL_STATES), 0)
        TS_ASSERT_EQUALS(ppa->get_stats().get_attr_unsigned(PLAJA::StatsUnsigned::PREDICATES), 43)

    }

    void test_nn_32_prop_6() {

        modelConstructor.get_config().set_value_option(PLAJA_OPTION::additional_properties, propertyFile);
        modelConstructor.construct_model(modelFile);
        auto ppa = (modelConstructor.construct_instance_cast<PredicateAbstraction>(SearchEngineFactory::PaType, 2));
        ppa->search();

        TS_ASSERT_EQUALS(ppa->get_stats().get_attr_unsigned(PLAJA::StatsUnsigned::PREDICATES), 49)
        TS_ASSERT_EQUALS(ppa->get_stats().get_attr_unsigned(PLAJA::StatsUnsigned::START_STATES), 202)
        TS_ASSERT_EQUALS(ppa->get_stats().get_attr_unsigned(PLAJA::StatsUnsigned::EXPANDED_STATES), 349)
        TS_ASSERT_EQUALS(ppa->get_stats().get_attr_unsigned(PLAJA::StatsUnsigned::TRANSITIONS), 738)
        TS_ASSERT_EQUALS(ppa->get_stats().get_attr_unsigned(PLAJA::StatsUnsigned::REACHABLE_STATES), 349)
        TS_ASSERT_EQUALS(ppa->get_stats().get_attr_unsigned(PLAJA::StatsUnsigned::GOAL_STATES), 0)

    }

/**********************************************************************************************************************/

    void run_tests() override {
        RUN_TEST(test_nn_32_prop_5())
        RUN_TEST(test_nn_32_prop_6())
    }

};

TEST_MAIN(PpaBlock6CostTest)