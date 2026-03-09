//
// This file is part of the PlaJA code base.
// Copyright (c) (2019 - 2024) Marcel Vinzent.
// See README.md in the top-level directory for licensing information.
//

#include <memory>
#include "../../option_parser/enum_option_values_set.h"
#include "../../option_parser/plaja_options.h"
#include "../../search/factories/predicate_abstraction/pa_options.h"
#include "../../search/factories/smt_base/smt_options.h"
#include "../../search/predicate_abstraction/predicate_abstraction.h"
#include "../../stats/stats_base.h"
#include "../utils/test_header.h"
#include "../utils/test_parser.h"

const std::string modelFile("../../../tests/test_instances/icaps22/racetrack/benchmarks/simple/EvaluationMMB/barto-small/prob1/prob1-x00y05acc_x00acc_y00.jani"); // NOLINT(*-err58-cpp)
const std::string propertyFile("../../../tests/test_instances/icaps22/racetrack/additional_properties/safety_verification/simple/EvaluationMMB/barto-small/random_starts_999/pa_simple_barto-small_random_starts_999_simple_barto-small_16_16.jani"); // NOLINT(*-err58-cpp)

class PpaRacetrackTest: public PLAJA_TEST::TestSuite {

private:
    PLAJA::ModelConstructorForTests modelConstructor;

public:

    PpaRacetrackTest() {
        modelConstructor.get_config().set_bool_option(PLAJA_OPTION::const_vars_to_consts, true);
        modelConstructor.get_config().set_enum_option(PLAJA_OPTION::pa_state_reachability, PLAJA_OPTION::EnumOptionValuesSet::construct_singleton(PLAJA_OPTION::EnumOptionValue::PaOnlyReachabilityPerSrc));

        if (PLAJA_GLOBAL::enableApplicabilityCache) {
            modelConstructor.get_config().set_value_option("cache-op-app", "none");
            modelConstructor.get_config().set_bool_option("inc-op-app-test", true);
        }

    }

    ~PpaRacetrackTest() override = default;

    DELETE_CONSTRUCTOR(PpaRacetrackTest)

    void test_nn_16() {

        modelConstructor.get_config().set_value_option(PLAJA_OPTION::additional_properties, propertyFile);
        modelConstructor.construct_model(modelFile);
        auto ppa = (modelConstructor.construct_instance_cast<PredicateAbstraction>(SearchEngineFactory::PaType, 3));
        ppa->search();

        TS_ASSERT_EQUALS(ppa->get_stats().get_attr_unsigned(PLAJA::StatsUnsigned::PREDICATES), 6)
        TS_ASSERT_EQUALS(ppa->get_stats().get_attr_unsigned(PLAJA::StatsUnsigned::START_STATES), 15)
        TS_ASSERT_EQUALS(ppa->get_stats().get_attr_unsigned(PLAJA::StatsUnsigned::EXPANDED_STATES), 68)
        TS_ASSERT_EQUALS(ppa->get_stats().get_attr_unsigned(PLAJA::StatsUnsigned::TRANSITIONS), 134)
        TS_ASSERT_EQUALS(ppa->get_stats().get_attr_unsigned(PLAJA::StatsUnsigned::REACHABLE_STATES), 68)
        TS_ASSERT_EQUALS(ppa->get_stats().get_attr_unsigned(PLAJA::StatsUnsigned::GOAL_STATES), 16)
        TS_ASSERT_EQUALS(ppa->get_stats().get_attr_unsigned(PLAJA::StatsUnsigned::GoalPathStates), 64)

    }

/**********************************************************************************************************************/

    void run_tests() override {
        RUN_TEST(test_nn_16())
    }

};

TEST_MAIN(PpaRacetrackTest)