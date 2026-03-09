//
// This file is part of the PlaJA code base.
// Copyright (c) (2019 - 2024) Marcel Vinzent.
// See README.md in the top-level directory for licensing information.
//

#include <memory>
#include "../../option_parser/enum_option_values_set.h"
#include "../../option_parser/plaja_options.h"
#include "../../search/factories/bmc/bmc_options.h"
#include "../../search/bmc/bmc.h"
#include "../../stats/stats_base.h"
#include "../utils/test_header.h"
#include "../utils/test_parser.h"

const std::string modelFile("../../../tests/test_instances/icaps22/racetrack/benchmarks/simple/EvaluationMMB/barto-small/prob1/prob1-x00y05acc_x00acc_y00.jani"); // NOLINT(*-err58-cpp)
const std::string propertyFile("../../../tests/test_instances/icaps22/racetrack/additional_properties/safety_verification/simple/EvaluationMMB/barto-small/random_starts_999/pa_simple_barto-small_random_starts_999_simple_barto-small_16_16.jani"); // NOLINT(*-err58-cpp)

class BmcRacetrackTest: public PLAJA_TEST::TestSuite {

private:
    PLAJA::ModelConstructorForTests modelConstructor;

public:

    BmcRacetrackTest() {

        modelConstructor.get_config().set_bool_option(PLAJA_OPTION::const_vars_to_consts, true);

        modelConstructor.get_config().set_bool_option(PLAJA_OPTION::find_shortest_path_only, true);

        modelConstructor.get_config().set_enum_option(PLAJA_OPTION::bmc_solver, PLAJA_OPTION::EnumOptionValuesSet::construct_singleton(PLAJA_OPTION::EnumOptionValue::Z3));

        modelConstructor.get_config().set_enum_option(PLAJA_OPTION::bmc_mode, PLAJA_OPTION::EnumOptionValuesSet::construct_singleton(PLAJA_OPTION::EnumOptionValue::BmcUnsafe));
    }

    ~BmcRacetrackTest() override = default;

    DELETE_CONSTRUCTOR(BmcRacetrackTest)

    void test_nn_16() {

        modelConstructor.get_config().set_value_option(PLAJA_OPTION::additional_properties, propertyFile);
        modelConstructor.construct_model(modelFile);
        auto bmc = (modelConstructor.construct_instance_cast<BoundedMC>(SearchEngineFactory::BmcType, 3));
        bmc->search();

        TS_ASSERT_EQUALS(bmc->get_stats().get_attr_int(PLAJA::StatsInt::BMC_STEPS), 3)
        TS_ASSERT_EQUALS(bmc->get_stats().get_attr_int(PLAJA::StatsInt::BMC_SHORTEST_UNSAFE_PATH), 3)
        TS_ASSERT_EQUALS(bmc->get_stats().get_attr_int(PLAJA::StatsInt::BMC_LONGEST_LOOP_FREE_PATH), -1)

    }

/**********************************************************************************************************************/

    void run_tests() override {
        RUN_TEST(test_nn_16())
    }

};

TEST_MAIN(BmcRacetrackTest)