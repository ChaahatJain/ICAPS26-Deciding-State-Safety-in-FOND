//
// This file is part of the PlaJA code base.
// Copyright (c) (2019 - 2020) Marcel Vinzent.
// See README.md in the top-level directory for licensing information.
//

#ifndef PLAJA_SUCCESSOR_GENERATION_TEST_CPP
#define PLAJA_SUCCESSOR_GENERATION_TEST_CPP

#include <memory>
#include "../../parser/ast/composition.h"
#include "../../parser/ast/model.h"
#include "../../parser/ast/synchronisation.h"
#include "../../search/successor_generation/action_op.h"
#include "../../search/successor_generation/successor_generator.h"
#include "../../search/successor_generation/successor_generator_c.h"
#include "../../search/fd_adaptions/state.h"
#include "../../search/information/model_information.h"
#include "../utils/test_header.h"

const std::string filename("../../../tests/test_instances/for_succ_gen.jani");  // NOLINT(*-avoid-non-const-global-variables, *-err58-cpp)

class SuccessorGenerationTest: public PLAJA_TEST::TestSuite {

private:
    // counters to check existence of certain successor states
    // non-sync
    unsigned A4_1 = 0;
    unsigned A_non_sync_2_z_1 = 0; // silent edge
    unsigned A_non_sync_2_z_2 = 0; // "pseudo"-silent "non_sync" synchronisation (vs. disabled "null_sync")
    // sync via destination location
    unsigned sync_label_1_1 = 0;
    unsigned sync_label_1_2 = 0;
    unsigned sync_label_1_3 = 0;
    unsigned sync_label_1_4 = 0;
    unsigned sync_label_2_1 = 0;
    unsigned sync_label_2_2 = 0;
    unsigned sync_label_2_3 = 0;
    unsigned sync_label_2_4 = 0;
    unsigned sync_label_3_1 = 0;
    unsigned sync_label_3_2 = 0;
    unsigned sync_label_3_3 = 0;
    unsigned sync_label_3_4 = 0;
    unsigned sync_label_4_1 = 0;
    unsigned sync_label_4_2 = 0;
    unsigned sync_label_4_3 = 0;
    unsigned sync_label_4_4 = 0;
    unsigned sync_label_7_1 = 0;
    unsigned sync_label_7_2 = 0;
    unsigned sync_label_7_3 = 0;
    unsigned sync_label_7_4 = 0;
    unsigned sync_label_alt_5_5 = 0;

    PLAJA::ModelConstructorForTests modelConstructor;
    std::unique_ptr<SuccessorGenerator> successorGeneratorR;
    std::unique_ptr<SuccessorGeneratorC> successorGeneratorC;
    std::unique_ptr<StateRegistry> stateRegistry;
    std::unique_ptr<State> initialState;

public:

    SuccessorGenerationTest() {
        modelConstructor.get_config().set_bool_option(PLAJA_OPTION::const_vars_to_consts, false); // Manual maintained local vars.
        modelConstructor.construct_model(filename);
        successorGeneratorR = std::make_unique<SuccessorGenerator>(modelConstructor.get_config(), *modelConstructor.constructedModel);
        successorGeneratorC = std::make_unique<SuccessorGeneratorC>(modelConstructor.get_config(), *modelConstructor.constructedModel);
        const ModelInformation& model_info = modelConstructor.constructedModel->get_model_information();
        stateRegistry = std::make_unique<StateRegistry>(model_info.get_ranges_int(), model_info.get_lower_bounds_int(), model_info.get_upper_bounds_int(), model_info.get_floating_state_size());
        initialState = stateRegistry->set_state(model_info.get_initial_values());
    }

    ~SuccessorGenerationTest() override = default;

/**********************************************************************************************************************/

    void testInitialState() {
        std::cout << std::endl << "Test on the initial state:" << std::endl;
        TS_ASSERT(initialState->size() == 15)
        const State& state = *initialState; // for readability;
        // all variables are initialised to 0, hence:
        TS_ASSERT(state.get_int(0) == 0 // A1
                  && state.get_int(1) == 0 // A2
                  && state.get_int(2) == 0 // A_non_sync
                  && state.get_int(3) == 0 // A_sync_enabled
                  && state.get_int(4) == 0 // A4
                  && state.get_int(5) == 0 // x
                  && state.get_int(6) == 0 // y
                  && state.get_int(7) == 0 // z
                  && state.get_int(8) == 0 // w
                  && state.get_int(9) == 0 // A[0]
                  && state.get_int(10) == 0 // A[1]
                  && state.get_int(11) == 0 // A[2]
                  && state.get_int(12) == 0 // A4_enabled
                  && state.get_int(13) == 0 // local_var in A_sync_enabled
                  && state.get_int(14) == 1 // local_var in A4
        )
    }

    /******************************************************************************************************************/

private:

    // auxiliaries

    void reset_counters() {
        // non-sync
        A4_1 = 0;
        A_non_sync_2_z_1 = 0;
        A_non_sync_2_z_2 = 0;
        // sync via destination location
        sync_label_1_1 = 0;
        sync_label_1_2 = 0;
        sync_label_1_3 = 0;
        sync_label_1_4 = 0;
        sync_label_2_1 = 0;
        sync_label_2_2 = 0;
        sync_label_2_3 = 0;
        sync_label_2_4 = 0;
        sync_label_3_1 = 0;
        sync_label_3_2 = 0;
        sync_label_3_3 = 0;
        sync_label_3_4 = 0;
        sync_label_4_1 = 0;
        sync_label_4_2 = 0;
        sync_label_4_3 = 0;
        sync_label_4_4 = 0;
        sync_label_7_1 = 0;
        sync_label_7_2 = 0;
        sync_label_7_3 = 0;
        sync_label_7_4 = 0;
        sync_label_alt_5_5 = 0;
    }

    // per successor checks
    template<typename State_t1, typename State_t2>
    bool check_A4_1(const State_t1& source, const State_t2& target, PLAJA::Prob_type prob) {
        StateValues state_values = source.to_state_values();
        state_values.assign_int(4, 1); // A4
        state_values.assign_int(5, 2); // x
        state_values.assign_int(11, 2); // A[2]
        if (prob == 1 && target == state_values) {
            ++A4_1;
            return true;
        } else {
            return false;
        }
    }

    template<typename State_t1, typename State_t2>
    bool check_A_sync_2_z_1(const State_t1& source, const State_t2& target, PLAJA::Prob_type prob) {
        StateValues state_values = source.to_state_values();
        state_values.assign_int(2, 2); // A_non_sync
        state_values.assign_int(7, 1); // z
        if (prob == 1 && target == state_values) {
            ++A_non_sync_2_z_1;
            return true;
        } else {
            return false;
        }
    }

    template<typename State_t1, typename State_t2>
    bool check_A_sync_2_z_2(const State_t1& source, const State_t2& target, PLAJA::Prob_type prob) {
        StateValues state_values = source.to_state_values();
        state_values.assign_int(2, 2); // A_non_sync
        state_values.assign_int(7, 2); // z
        if (prob == 1 && target == state_values) {
            ++A_non_sync_2_z_2;
            return true;
        } else {
            return false;
        }
    }

    //

    template<typename State_t1, typename State_t2>
    bool check_sync_label_1_1(const State_t1& source, const State_t2& target, PLAJA::Prob_type prob) {
        StateValues state_values = source.to_state_values();
        state_values.assign_int(0, 1); // A1
        state_values.assign_int(1, 1); // A2
        state_values.assign_int(3, 1); // A_sync_enabled
        PLAJA_ASSERT(source.get_int(8) == 0) //  w should be 0
        state_values.assign_int(9 + source.get_int(8), 1); // A[w] = 1
        state_values.assign_int(10, 1); // A[1]
        state_values.assign_int(13, 1); // local_var
        if (std::abs(prob - 0.03) <= PLAJA::probabilityPrecision && target == state_values) {
            ++sync_label_1_1;
            return true;
        } else {
            return false;
        }
    }

    template<typename State_t1, typename State_t2>
    bool check_sync_label_1_2(const State_t1& source, const State_t2& target, PLAJA::Prob_type prob) {
        StateValues state_values = source.to_state_values();
        state_values.assign_int(0, 1); // A1
        state_values.assign_int(1, 2); // A2
        state_values.assign_int(3, 1); // A_sync_enabled
        state_values.assign_int(6, source.get_int(6) + 1); // y
        PLAJA_ASSERT(source.get_int(8) == 0) //  w should be 0
        state_values.assign_int(9 + source.get_int(8), 1); // A[w] = 1
        state_values.assign_int(13, 1); // local_var
        if (std::abs(prob - 0.07) <= PLAJA::probabilityPrecision && target == state_values) {
            ++sync_label_1_2;
            return true;
        } else {
            return false;
        }
    }

    template<typename State_t1, typename State_t2>
    bool check_sync_label_1_3(const State_t1& source, const State_t2& target, PLAJA::Prob_type prob) {
        StateValues state_values = source.to_state_values();
        state_values.assign_int(0, 1); // A1
        state_values.assign_int(1, 3); // A2
        state_values.assign_int(3, 1); // A_sync_enabled
        PLAJA_ASSERT(source.get_int(8) == 0) //  w should be 0
        state_values.assign_int(9 + source.get_int(8), 1); // A[w] = 1
        state_values.assign_int(11, 2); // A[2]
        state_values.assign_int(13, 1); // local_var
        if (std::abs(prob - 0.04) <= PLAJA::probabilityPrecision && target == state_values) {
            ++sync_label_1_3;
            return true;
        } else {
            return false;
        }
    }

    template<typename State_t1, typename State_t2>
    bool check_sync_label_1_4(const State_t1& source, const State_t2& target, PLAJA::Prob_type prob) {
        StateValues state_values = source.to_state_values();
        state_values.assign_int(0, 1); // A1
        state_values.assign_int(1, 4); // A2
        state_values.assign_int(3, 1); // A_sync_enabled
        state_values.assign_int(6, source.get_int(6) - 1); // y
        PLAJA_ASSERT(source.get_int(8) == 0) //  w should be 0
        state_values.assign_int(9 + source.get_int(8), 1); // A[w] = 1
        state_values.assign_int(13, 1); // local_var
        if (std::abs(prob - 0.06) <= PLAJA::probabilityPrecision && target == state_values) {
            ++sync_label_1_4;
            return true;
        } else {
            return false;
        }
    }

    //

    template<typename State_t1, typename State_t2>
    bool check_sync_label_2_1(const State_t1& source, const State_t2& target, PLAJA::Prob_type prob) {
        StateValues state_values = source.to_state_values();
        state_values.assign_int(0, 2); // A1
        state_values.assign_int(1, 1); // A2
        state_values.assign_int(3, 1); // A_sync_enabled
        state_values.assign_int(5, source.get_int(5) + 1); // x
        state_values.assign_int(10, 1); // A[1]
        state_values.assign_int(13, 1); // local_var
        if (std::abs(prob - 0.27) <= PLAJA::probabilityPrecision && target == state_values) {
            ++sync_label_2_1;
            return true;
        } else {
            return false;
        }
    }

    template<typename State_t1, typename State_t2>
    bool check_sync_label_2_2(const State_t1& source, const State_t2& target, PLAJA::Prob_type prob) {
        StateValues state_values = source.to_state_values();
        state_values.assign_int(0, 2); // A1
        state_values.assign_int(1, 2); // A2
        state_values.assign_int(3, 1); // A_sync_enabled
        state_values.assign_int(5, source.get_int(5) + 1); // x
        state_values.assign_int(6, source.get_int(6) + 1); // y
        state_values.assign_int(13, 1); // local_var
        if (std::abs(prob - 0.63) <= PLAJA::probabilityPrecision && target == state_values) {
            ++sync_label_2_2;
            return true;
        } else {
            return false;
        }
    }

    template<typename State_t1, typename State_t2>
    bool check_sync_label_2_3(const State_t1& source, const State_t2& target, PLAJA::Prob_type prob) {
        StateValues state_values = source.to_state_values();
        state_values.assign_int(0, 2); // A1
        state_values.assign_int(1, 3); // A2
        state_values.assign_int(3, 1); // A_sync_enabled
        state_values.assign_int(5, source.get_int(5) + 1); // x
        state_values.assign_int(11, 2); // A[2]
        state_values.assign_int(13, 1); // local_var
        if (std::abs(prob - 0.36) <= PLAJA::probabilityPrecision && target == state_values) {
            ++sync_label_2_3;
            return true;
        } else {
            return false;
        }
    }

    template<typename State_t1, typename State_t2>
    bool check_sync_label_2_4(const State_t1& source, const State_t2& target, PLAJA::Prob_type prob) {
        StateValues state_values = source.to_state_values();
        state_values.assign_int(0, 2); // A1
        state_values.assign_int(1, 4); // A2
        state_values.assign_int(3, 1); // A_sync_enabled
        state_values.assign_int(5, source.get_int(5) + 1); // x
        state_values.assign_int(6, source.get_int(6) - 1); // y
        state_values.assign_int(13, 1); // local_var
        if (std::abs(prob - 0.54) <= PLAJA::probabilityPrecision && target == state_values) {
            ++sync_label_2_4;
            return true;
        } else {
            return false;
        }
    }

    //

    template<typename State_t1, typename State_t2>
    bool check_sync_label_3_1(const State_t1& source, const State_t2& target, PLAJA::Prob_type prob) {
        StateValues state_values = source.to_state_values();
        state_values.assign_int(0, 3); // A1
        state_values.assign_int(1, 1); // A2
        state_values.assign_int(3, 1); // A_sync_enabled
        PLAJA_ASSERT(source.get_int(7) == 0) //  z should be 0
        state_values.assign_int(9 + source.get_int(7), 2); // A[z] = 2
        state_values.assign_int(10, 1); // A[1]
        state_values.assign_int(13, 1); // local_var
        if (std::abs(prob - 0.06) <= PLAJA::probabilityPrecision && target == state_values) {
            ++sync_label_3_1;
            return true;
        } else {
            return false;
        }
    }

    template<typename State_t1, typename State_t2>
    bool check_sync_label_3_2(const State_t1& source, const State_t2& target, PLAJA::Prob_type prob) {
        StateValues state_values = source.to_state_values();
        state_values.assign_int(0, 3); // A1
        state_values.assign_int(1, 2); // A2
        state_values.assign_int(3, 1); // A_sync_enabled
        state_values.assign_int(6, source.get_int(6) + 1); // y
        PLAJA_ASSERT(source.get_int(7) == 0) //  z should be 0
        state_values.assign_int(9 + source.get_int(7), 2); // A[z] = 2
        state_values.assign_int(13, 1); // local_var
        if (std::abs(prob - 0.14) <= PLAJA::probabilityPrecision && target == state_values) {
            ++sync_label_3_2;
            return true;
        } else {
            return false;
        }
    }

    template<typename State_t1, typename State_t2>
    bool check_sync_label_3_3(const State_t1& source, const State_t2& target, PLAJA::Prob_type prob) {
        StateValues state_values = source.to_state_values();
        state_values.assign_int(0, 3); // A1
        state_values.assign_int(1, 3); // A2
        state_values.assign_int(3, 1); // A_sync_enabled
        PLAJA_ASSERT(source.get_int(7) == 0) //  z should be 0
        state_values.assign_int(9 + source.get_int(7), 2); // A[z] = 2
        state_values.assign_int(11, 2); // A[2]
        state_values.assign_int(13, 1); // local_var
        if (std::abs(prob - 0.08) <= PLAJA::probabilityPrecision && target == state_values) {
            ++sync_label_3_3;
            return true;
        } else {
            return false;
        }
    }

    template<typename State_t1, typename State_t2>
    bool check_sync_label_3_4(const State_t1& source, const State_t2& target, PLAJA::Prob_type prob) {
        StateValues state_values = source.to_state_values();
        state_values.assign_int(0, 3); // A1
        state_values.assign_int(1, 4); // A2
        state_values.assign_int(3, 1); // A_sync_enabled
        state_values.assign_int(6, source.get_int(6) - 1); // y
        PLAJA_ASSERT(source.get_int(7) == 0) //  z should be 0
        state_values.assign_int(9 + source.get_int(7), 2); // A[z] = 2
        state_values.assign_int(13, 1); // local_var
        if (std::abs(prob - 0.12) <= PLAJA::probabilityPrecision && target == state_values) {
            ++sync_label_3_4;
            return true;
        } else {
            return false;
        }
    }

    //

    template<typename State_t1, typename State_t2>
    bool check_sync_label_4_1(const State_t1& source, const State_t2& target, PLAJA::Prob_type prob) {
        StateValues state_values = source.to_state_values();
        state_values.assign_int(0, 4); // A1
        state_values.assign_int(1, 1); // A2
        state_values.assign_int(3, 1); // A_sync_enabled
        state_values.assign_int(5, source.get_int(5) - 1); // x
        state_values.assign_int(10, 1); // A[1]
        state_values.assign_int(13, 1); // local_var
        if (std::abs(prob - 0.24) <= PLAJA::probabilityPrecision && target == state_values) {
            ++sync_label_4_1;
            return true;
        } else {
            return false;
        }
    }

    template<typename State_t1, typename State_t2>
    bool check_sync_label_4_2(const State_t1& source, const State_t2& target, PLAJA::Prob_type prob) {
        StateValues state_values = source.to_state_values();
        state_values.assign_int(0, 4); // A1
        state_values.assign_int(1, 2); // A2
        state_values.assign_int(3, 1); // A_sync_enabled
        state_values.assign_int(5, source.get_int(5) - 1); // x
        state_values.assign_int(6, source.get_int(6) + 1); // y
        state_values.assign_int(13, 1); // local_var
        if (std::abs(prob - 0.56) <= PLAJA::probabilityPrecision && target == state_values) {
            ++sync_label_4_2;
            return true;
        } else {
            return false;
        }
    }

    template<typename State_t1, typename State_t2>
    bool check_sync_label_4_3(const State_t1& source, const State_t2& target, PLAJA::Prob_type prob) {
        StateValues state_values = source.to_state_values();
        state_values.assign_int(0, 4); // A1
        state_values.assign_int(1, 3); // A2
        state_values.assign_int(3, 1); // A_sync_enabled
        state_values.assign_int(5, source.get_int(5) - 1); // x
        state_values.assign_int(11, 2); // A[2]
        state_values.assign_int(13, 1); // local_var
        if (std::abs(prob - 0.32) <= PLAJA::probabilityPrecision && target == state_values) {
            ++sync_label_4_3;
            return true;
        } else {
            return false;
        }
    }

    template<typename State_t1, typename State_t2>
    bool check_sync_label_4_4(const State_t1& source, const State_t2& target, PLAJA::Prob_type prob) {
        StateValues state_values = source.to_state_values();
        state_values.assign_int(0, 4); // A1
        state_values.assign_int(1, 4); // A2
        state_values.assign_int(3, 1); // A_sync_enabled
        state_values.assign_int(5, source.get_int(5) - 1); // x
        state_values.assign_int(6, source.get_int(6) - 1); // y
        state_values.assign_int(13, 1); // local_var
        if (std::abs(prob - 0.48) <= PLAJA::probabilityPrecision && target == state_values) {
            ++sync_label_4_4;
            return true;
        } else {
            return false;
        }
    }

    //

    template<typename State_t1, typename State_t2>
    bool check_sync_label_7_1(const State_t1& source, const State_t2& target, PLAJA::Prob_type prob) {
        StateValues state_values = source.to_state_values();
        state_values.assign_int(0, 7); // A1
        state_values.assign_int(1, 1); // A2
        state_values.assign_int(3, 1); // A_sync_enabled
        state_values.assign_int(10, 1); // A[1]
        state_values.assign_int(13, 1); // local_var
        if (std::abs(prob - 0.3) <= PLAJA::probabilityPrecision && target == state_values) {
            ++sync_label_7_1;
            return true;
        } else {
            return false;
        }
    }

    template<typename State_t1, typename State_t2>
    bool check_sync_label_7_2(const State_t1& source, const State_t2& target, PLAJA::Prob_type prob) {
        StateValues state_values = source.to_state_values();
        state_values.assign_int(0, 7); // A1
        state_values.assign_int(1, 2); // A2
        state_values.assign_int(3, 1); // A_sync_enabled
        state_values.assign_int(6, source.get_int(6) + 1); // y
        state_values.assign_int(13, 1); // local_var
        if (std::abs(prob - 0.7) <= PLAJA::probabilityPrecision && target == state_values) {
            ++sync_label_7_2;
            return true;
        } else {
            return false;
        }
    }

    template<typename State_t1, typename State_t2>
    bool check_sync_label_7_3(const State_t1& source, const State_t2& target, PLAJA::Prob_type prob) {
        StateValues state_values = source.to_state_values();
        state_values.assign_int(0, 7); // A1
        state_values.assign_int(1, 3); // A2
        state_values.assign_int(3, 1); // A_sync_enabled
        state_values.assign_int(11, 2); // A[2]
        state_values.assign_int(13, 1); // local_var
        if (std::abs(prob - 0.4) <= PLAJA::probabilityPrecision && target == state_values) {
            ++sync_label_7_3;
            return true;
        } else {
            return false;
        }
    }

    template<typename State_t1, typename State_t2>
    bool check_sync_label_7_4(const State_t1& source, const State_t2& target, PLAJA::Prob_type prob) {
        StateValues state_values = source.to_state_values();
        state_values.assign_int(0, 7); // A1
        state_values.assign_int(1, 4); // A2
        state_values.assign_int(3, 1); // A_sync_enabled
        state_values.assign_int(6, source.get_int(6) - 1); // y
        state_values.assign_int(13, 1); // local_var
        if (std::abs(prob - 0.6) <= PLAJA::probabilityPrecision && target == state_values) {
            ++sync_label_7_4;
            return true;
        } else {
            return false;
        }
    }

    //

    template<typename State_t1, typename State_t2>
    bool check_sync_label_alt_5_5(const State_t1& source, const State_t2& target, PLAJA::Prob_type prob) {
        StateValues state_values = source.to_state_values();
        state_values.assign_int(0, 5); // A1
        state_values.assign_int(1, 5); // A2
        state_values.assign_int(7, 10); // y
        PLAJA_ASSERT(source.get_int(8) == 0)
        state_values.assign_int(8, source.get_int(8) + 1);
        state_values.assign_int(9, 1); // A[z] = w (z = 0 due to sequence indexes)
        if (prob == 1 && target == state_values) {
            ++sync_label_alt_5_5;
            return true;
        } else {
            return false;
        }
    }

    //

    template<typename State_t1, typename State_t2>
    void check_successor(const State_t1& source, const State_t2& target, PLAJA::Prob_type prob) {
        check_global_invariants(target);
        // asa one successor flag is incremented, we return ...
        if (check_A4_1(source, target, prob)) { return; }
        if (check_A_sync_2_z_1(source, target, prob)) { return; }
        if (check_A_sync_2_z_2(source, target, prob)) { return; }
        if (check_sync_label_1_1(source, target, prob)) { return; }
        if (check_sync_label_1_2(source, target, prob)) { return; }
        if (check_sync_label_1_3(source, target, prob)) { return; }
        if (check_sync_label_1_4(source, target, prob)) { return; }
        if (check_sync_label_2_1(source, target, prob)) { return; }
        if (check_sync_label_2_2(source, target, prob)) { return; }
        if (check_sync_label_2_3(source, target, prob)) { return; }
        if (check_sync_label_2_4(source, target, prob)) { return; }
        if (check_sync_label_3_1(source, target, prob)) { return; }
        if (check_sync_label_3_2(source, target, prob)) { return; }
        if (check_sync_label_3_3(source, target, prob)) { return; }
        if (check_sync_label_3_4(source, target, prob)) { return; }
        if (check_sync_label_4_1(source, target, prob)) { return; }
        if (check_sync_label_4_2(source, target, prob)) { return; }
        if (check_sync_label_4_3(source, target, prob)) { return; }
        if (check_sync_label_4_4(source, target, prob)) { return; }
        if (check_sync_label_7_1(source, target, prob)) { return; }
        if (check_sync_label_7_2(source, target, prob)) { return; }
        if (check_sync_label_7_3(source, target, prob)) { return; }
        if (check_sync_label_7_4(source, target, prob)) { return; }
        check_sync_label_alt_5_5(source, target, prob);
    }

    template<typename State_t>
    void check_global_invariants(const State_t& state) {
        TS_ASSERT(state.get_int(2) != 1) // A_non_sync never transitions into loc1, as its "label"-edge is not participating in "label"-transitions
        TS_ASSERT(state.get_int(4) != 2) // A4 can not transition to loc2 as probability = 0
        TS_ASSERT(state.get_int(14) == true) // local_var in A4 is untouched
        TS_ASSERT(state.get_int(8) != 42) // assignment w := 42 in A_unused is never executed, same holds for second destination in A_sync_enabled (prob=0)
    }

    template<class SuccessorGenerator_t>
    void extract_successor_states(SuccessorGenerator_t& successor_generator, const State& source, std::list<std::pair<std::unique_ptr<State>, PLAJA::Prob_type>>& successors) {
        successor_generator.explore(source);
        for (auto op_it = successor_generator.init_action_op_it_per_explore(); !op_it.end(); ++op_it) {
            const auto& action_op = *op_it;
            if constexpr (std::is_same_v<SuccessorGenerator_t, SuccessorGenerator>) {
                for (auto ch_it = action_op.transitionIterator(); !ch_it.end(); ++ch_it) {
                    TS_ASSERT(ch_it.prob() > 0)
                    successors.emplace_back(successor_generator.compute_successor(ch_it.transition(), source), ch_it.prob());
                }
            } else {
                for (auto update_it = action_op.updateIterator(); !update_it.end(); ++update_it) {
                    TS_ASSERT(update_it.prob() > 0)
                    successors.emplace_back(successor_generator.compute_successor(update_it.update(), source), update_it.prob());
                }
            }
        }
    }

    template<class SuccessorGenerator_t>
    void analyze_successors(SuccessorGenerator_t& successor_generator, const State& source, unsigned int expected_num_ops, unsigned int expected_num_successors) {
        // Explore & Extract
        std::list<std::pair<std::unique_ptr<State>, PLAJA::Prob_type>> successors;
        extract_successor_states(successor_generator, source, successors);
        TS_ASSERT(successor_generator._number_of_enabled_ops() == expected_num_ops)
        TS_ASSERT(successors.size() == expected_num_successors)
        // Analyze successors ...
        reset_counters();
        for (const auto& state_prob: successors) {
            check_successor(source, *state_prob.first, state_prob.second);
        }
    }

    template<class SuccessorGenerator_t>
    void test_sync_in_initial_state(SuccessorGenerator_t& successor_generator) {
        analyze_successors(successor_generator, *initialState, 5, 11);

        TS_ASSERT(A_non_sync_2_z_1 == 1)
        TS_ASSERT(A_non_sync_2_z_2 == 1)
        TS_ASSERT(sync_label_1_3 == 1)
        TS_ASSERT(sync_label_1_4 == 1)
        TS_ASSERT(sync_label_2_3 == 1)
        TS_ASSERT(sync_label_2_4 == 1)
        TS_ASSERT(sync_label_3_3 == 1)
        TS_ASSERT(sync_label_3_4 == 1)
        TS_ASSERT(sync_label_4_3 == 1)
        TS_ASSERT(sync_label_4_4 == 1)
        TS_ASSERT(sync_label_alt_5_5 == 1)
    }

    template<class SuccessorGenerator_t>
    void test_label_sync_in_2_2_1(SuccessorGenerator_t& successor_generator, const State& source) {
        analyze_successors(successor_generator, source, 7, 19);

        TS_ASSERT(A_non_sync_2_z_1 == 1)
        TS_ASSERT(A_non_sync_2_z_2 == 1)
        TS_ASSERT(sync_label_1_1 == 1)
        TS_ASSERT(sync_label_1_2 == 1)
        TS_ASSERT(sync_label_1_3 == 1)
        TS_ASSERT(sync_label_1_4 == 1)
        TS_ASSERT(sync_label_2_1 == 1)
        TS_ASSERT(sync_label_2_2 == 1)
        TS_ASSERT(sync_label_2_3 == 1)
        TS_ASSERT(sync_label_2_4 == 1)
        TS_ASSERT(sync_label_3_1 == 1)
        TS_ASSERT(sync_label_3_2 == 1)
        TS_ASSERT(sync_label_3_3 == 1)
        TS_ASSERT(sync_label_3_4 == 1)
        TS_ASSERT(sync_label_4_1 == 1)
        TS_ASSERT(sync_label_4_2 == 1)
        TS_ASSERT(sync_label_4_3 == 1)
        TS_ASSERT(sync_label_4_4 == 1)
        TS_ASSERT(sync_label_alt_5_5 == 1)
    }

    template<class SuccessorGenerator_t>
    void test_label_sync_with_A1_in_6(SuccessorGenerator_t& successor_generator, const State& source) {
        analyze_successors(successor_generator, source, 3, 4);

        TS_ASSERT(A_non_sync_2_z_1 == 1)
        TS_ASSERT(A_non_sync_2_z_2 == 1)
        TS_ASSERT(sync_label_7_1 == 1)
        TS_ASSERT(sync_label_7_2 == 1)
        TS_ASSERT(sync_label_alt_5_5 == 0) // explicit test to put focus on
    }

    template<class SuccessorGenerator_t>
    void test_label_sync_disabled_by_guards(SuccessorGenerator_t& successor_generator, const State& source) {
        analyze_successors(successor_generator, source, 2, 2);
        TS_ASSERT(A_non_sync_2_z_1 == 1)
        TS_ASSERT(A_non_sync_2_z_2 == 1)
    }

    template<class SuccessorGenerator_t>
    void test_label_sync_with_A4_enabled(SuccessorGenerator_t& successor_generator, const State& source) {
        analyze_successors(successor_generator, source, 1, 1);
        TS_ASSERT(A4_1 == 1)
    }

/**********************************************************************************************************************/

public:

    void testSyncInInitialState() {
        std::cout << std::endl << "Complex test on successor generators at initial state:" << std::endl;
        // (whitebox: // Case where there are multiple edges  + // Case where there are no existing operators)
        // (whitebox: // Case where there is only 1 edge for this automaton  + // Case where there are existing operators)
        test_sync_in_initial_state(*successorGeneratorR);
        test_sync_in_initial_state(*successorGeneratorC);
    }

    void testLabelSyncIn2_2_1() {
        std::cout << std::endl << "Complex test on successor generators at non-initial state:" << std::endl;
        // initial state only has two enabled edges in A1,
        // now we test with 2 edges enabled in A1 and A2 for "label"-sync (whitebox: // Case where there are multiple edges  + // Case where there are existing operators)

        // make all "label"-edges in A1 and A2 enabled
        auto constructor = initialState->to_state_values();
        constructor.assign_int(5, 1); // x
        constructor.assign_int(6, 1); // y
        auto source = initialState->to_state(constructor);

        test_label_sync_in_2_2_1(*successorGeneratorR, *source);
        test_label_sync_in_2_2_1(*successorGeneratorC, *source);
    }

    void testLabelSyncWithA1In6() {
        std::cout << std::endl << "Complex test on successor generators at non-initial state:" << std::endl;
        // test with A1 in location 6
        // now we test with only 1 edge enabled in A1 for "label"-sync (whitebox: // Case where there is only 1 edge for this automaton  +  Case where there are no existing operators)
        // additionally, we disable "label_alt" via changing the synchronisation directive to include A_non_sync

        // adapt sync directive to disable "label_alt"-sync
        std::unique_ptr<Model> model = modelConstructor.constructedModel->deep_copy();
        Synchronisation* sync = model->get_system()->get_sync(1);
        sync->set_syncAction("label_alt1", 2); // A_non_sync has no such labeled edge
        sync->set_syncActionID(1, 2);
        model->compute_model_information();

        // must use different successor generators:
        std::unique_ptr<SuccessorGenerator> successor_generator_r(new SuccessorGenerator(modelConstructor.get_config(), *model));
        std::unique_ptr<SuccessorGeneratorC> successor_generator_c(new SuccessorGeneratorC(modelConstructor.get_config(), *model));

        // however, can use same state registry
        auto constructor = initialState->to_state_values();
        constructor.assign_int(0, 6); // x
        constructor.assign_int(5, 1); // x // this does enable (loc1,loc2) and disables (loc3,loc4) in A2
        auto source = initialState->to_state(constructor);

        test_label_sync_with_A1_in_6(*successor_generator_r, *source);
        test_label_sync_with_A1_in_6(*successor_generator_c, *source);
    }

    void testLabelSyncDisabledByGuards() {
        std::cout << std::endl << "Test on successor generators disabling synchronisation up to 1 automaton:" << std::endl;
        // test where "label"-edges in A1, A2 are all disabled, i.e. only A3 is enabled

        // make all "label"-edges in A1 and A2 disabled
        auto constructor = initialState->to_state_values();
        constructor.assign_int(6, -2); // y // A2 via guard
        constructor.assign_int(0, 1); // A1 // via location (this also disabled "label_alt")
        auto source = initialState->to_state(constructor);

        test_label_sync_disabled_by_guards(*successorGeneratorR, *source);
        test_label_sync_disabled_by_guards(*successorGeneratorC, *source);
    }

    void testLabelSyncWithA4Enabled() {
        std::cout << std::endl << "Test on successor generators with A4 enabled ..." << std::endl;

        auto constructor = initialState->to_state_values();
        constructor.assign_int(12, 1); // A4_enabled
        auto source = initialState->to_state(constructor);

        test_label_sync_with_A4_enabled(*successorGeneratorR, *source);
        test_label_sync_with_A4_enabled(*successorGeneratorC, *source);
    }

/**********************************************************************************************************************/

    void run_tests() override {
        RUN_TEST(testInitialState())
        RUN_TEST(testSyncInInitialState())
        RUN_TEST(testLabelSyncIn2_2_1())
        RUN_TEST(testLabelSyncWithA1In6())
        RUN_TEST(testLabelSyncDisabledByGuards())
        RUN_TEST(testLabelSyncWithA4Enabled())
    }

};

TEST_MAIN(SuccessorGenerationTest)

#endif //PLAJA_SUCCESSOR_GENERATION_TEST_CPP
