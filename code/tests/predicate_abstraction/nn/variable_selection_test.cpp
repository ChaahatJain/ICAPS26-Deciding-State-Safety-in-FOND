//
// This file is part of the PlaJA code base.
// Copyright (c) (2019 - 2023) Marcel Vinzent.
// See README.md in the top-level directory for licensing information.
//

#ifndef PLAJA_TESTS_VARIABLE_SELECTION_TEST_CPP
#define PLAJA_TESTS_VARIABLE_SELECTION_TEST_CPP

#include "../../../search/information/jani2nnet/jani_2_nnet.h"
#include "../../../search/information/model_information.h"
#include "../../../search/information/property_information.h"
#include "../../../search/predicate_abstraction/cegar/variable_selection.h"
#include "../../../search/states/state_values.h"
#include "../../utils/test_header.h"

const std::string filename("../../../../tests/test_instances/blocksworld/cost_terminal/models/legacy_enc/blocksworld_4_3.jani"); // NOLINT(*-err58-cpp)

namespace PLAJA_OPTION { extern const std::string perturbation_bound; }

class VariableSelectionTest: public PLAJA_TEST::TestSuite {

private:
    // structures
    PLAJA::ModelConstructorForTests modelConstructor;
    std::unique_ptr<PropertyInformation> propInfo;

public:
    VariableSelectionTest() {
        modelConstructor.set_constant("failing_probability", "0.5");
        modelConstructor.set_constant("item_cost_bound", "50");
        modelConstructor.construct_model(filename);
        propInfo = modelConstructor.analyze_property(1);
        modelConstructor.construct_instance(SearchEngineFactory::PaCegarType, 1); // dummy to add options
    }
    DELETE_CONSTRUCTOR(VariableSelectionTest)

    ~VariableSelectionTest() override = default;

    void testVariableSelection() {
        const auto& nn_interface = *propInfo->get_nn_interface();
        const auto& model_info = nn_interface.get_model_info();

        auto& config = modelConstructor.get_config();
        config.set_sharable_const(PLAJA::SharableKey::PROP_INFO, propInfo.get());
        config.set_double_option(PLAJA_OPTION::perturbation_bound, 1.0);
        VariableSelection var_sel(config);

        // customize states
        const auto& con_state = model_info.get_initial_values();
        auto witness_state = con_state.to_state_values();
        witness_state.set_int(1, 0);
        witness_state.set_int(3, 0); // block 0 in-hand
        witness_state.set_int(7, 0); // block 0 not clear // NOLINT(*-avoid-magic-numbers)
        witness_state.set_int(8, 1); // block 1 clear // NOLINT(*-avoid-magic-numbers)

        auto rel_features = var_sel.compute_relevant_vars(witness_state, con_state, nn_interface.eval_policy(witness_state));

        TS_ASSERT(rel_features.size() == 1)
    }

/**********************************************************************************************************************/

    void run_tests() override {
        RUN_TEST(testVariableSelection())
    }

};

TEST_MAIN(VariableSelectionTest)

#endif // PLAJA_TESTS_VARIABLE_SELECTION_TEST_CPP