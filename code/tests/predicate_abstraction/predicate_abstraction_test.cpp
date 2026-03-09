//
// This file is part of the PlaJA code base.
// Copyright (c) (2019 - 2020) Marcel Vinzent.
// See README.md in the top-level directory for licensing information.
//

#ifndef PLAJA_PREDICATE_ABSTRACTION_TEST_H
#define PLAJA_PREDICATE_ABSTRACTION_TEST_H

#include <z3++.h>
#include <memory>
#include "../../parser/ast/composition.h"
#include "../../parser/ast/synchronisation.h"
#include "../../parser/ast/model.h"
#include "../../search/factories/predicate_abstraction/pa_options.h"
#include "../../search/information/property_information.h"
#include "../../search/predicate_abstraction/heuristic/state_queue.h"
#include "../../search/predicate_abstraction/pa_states/predicate_state.h"
#include "../../search/predicate_abstraction/search_space/search_space_pa_base.h"
#include "../../search/predicate_abstraction/predicate_abstraction.h"
#include "../utils/test_header.h"

const std::string filename("../../../tests/test_instances/for_pa.jani"); // NOLINT(*-err58-cpp)

/* Hack to suppress assertion in PredicateRelations that conflicts with legacy tests. */
extern bool allowTrivialPreds;

class PredicateAbstractionTest: public PLAJA_TEST::TestSuite {

private:
    // flag to check existence of certain successor states
    bool sync_1_2_1_3_y_neg;
    bool sync_1_2_1_3_y_is_1;
    bool sync_1_2_2_4_y_neg;
    bool sync_1_2_2_4_y_is_1;
    bool sync_2_1_4_1_w_neg;
    bool sync_2_1_4_2_w_neg;
    bool sync_2_2_4_3_w_neg;
    bool sync_2_2_4_4_w_neg;

    PLAJA::ModelConstructorForTests modelConstructor;
    std::unique_ptr<PredicateAbstraction> predicateAbs;

public:
    PredicateAbstractionTest():
        sync_1_2_1_3_y_neg(false)
        , sync_1_2_1_3_y_is_1(false)
        , sync_1_2_2_4_y_neg(false)
        , sync_1_2_2_4_y_is_1(false)
        , sync_2_1_4_1_w_neg(false)
        , sync_2_1_4_2_w_neg(false)
        , sync_2_2_4_3_w_neg(false)
        , sync_2_2_4_4_w_neg(false) {

        allowTrivialPreds = true;

        modelConstructor.get_config().set_bool_option(PLAJA_OPTION::const_vars_to_consts, false); // A4_enabled is manually set.
        modelConstructor.get_config().set_bool_option(PLAJA_OPTION::predicate_relations, false);
        modelConstructor.get_config().set_bool_option(PLAJA_OPTION::incremental_search, false);
        modelConstructor.construct_model(filename);
        predicateAbs = PLAJA_UTILS::cast_unique<PredicateAbstraction>(modelConstructor.construct_instance(SearchEngineFactory::PaType, 0));
        predicateAbs->initialize();

        State::suppress_written_to(true); // as tests modify state objects
    }

    ~PredicateAbstractionTest() override = default;

/**********************************************************************************************************************/

    void test_initial_state() {
        std::cout << std::endl << "Test on the initial state:" << std::endl;
        // all variables are initialised to 0, hence:
        const auto pa_state_ptr = predicateAbs->searchSpace->get_abstract_initial_state();
        const auto& pa_state = *pa_state_ptr;
        TS_ASSERT(pa_state.get_size_locs() + pa_state.get_size_predicates() == 17)
        auto offset = pa_state.get_size_locs();
        TS_ASSERT(pa_state.location_value(0) == 0
                  and pa_state.location_value(1) == 0
                  and pa_state.location_value(2) == 0
                  and pa_state.location_value(3) == 0
                  and pa_state.location_value(4) == 0
                  and not pa_state.predicate_value(5 - offset) // A4_enabled
                  and pa_state.predicate_value(6 - offset) // x >= 0
                  and pa_state.predicate_value(7 - offset) // y >= 0
                  and not pa_state.predicate_value(8 - offset) // x = 1
                  and not pa_state.predicate_value(9 - offset) // y = 1
                  and not pa_state.predicate_value(10 - offset) // A[0] = 1
                  and not pa_state.predicate_value(11 - offset) // A[1] = 1
                  and not pa_state.predicate_value(12 - offset) // w == -42
                  and not pa_state.predicate_value(13 - offset) // w < -42
                  and pa_state.predicate_value(14 - offset) // z = 0
                  and pa_state.predicate_value(15 - offset) // A[2] = 0
                  and not pa_state.predicate_value(16 - offset) // x > 3
        )
    }

    /******************************************************************************************************************/

    // auxiliary tests for textSync, i.e., they are only valid for successors of the initial state
private:
    void check_existence_flags() {
        TS_ASSERT(sync_1_2_1_3_y_neg)
        TS_ASSERT(sync_1_2_1_3_y_is_1)
        TS_ASSERT(sync_1_2_2_4_y_neg)
        TS_ASSERT(sync_1_2_2_4_y_is_1)
        TS_ASSERT(sync_2_1_4_1_w_neg)
        TS_ASSERT(sync_2_1_4_2_w_neg)
        TS_ASSERT(sync_2_2_4_3_w_neg)
        TS_ASSERT(sync_2_2_4_4_w_neg)
    }

    void check_comb_guard(const AbstractState& pa_state) {
        // purpose: step must check whether combined guard is sat,
        // for the below sync cases (identified via successor locations), it is unsat: x=0 && x=1
        TS_ASSERT(not(pa_state.location_value(0) == 1 and pa_state.location_value(2) == 1))
        TS_ASSERT(not(pa_state.location_value(0) == 1 and pa_state.location_value(2) == 2))
        TS_ASSERT(not(pa_state.location_value(0) == 2 and pa_state.location_value(2) == 1))
        TS_ASSERT(not(pa_state.location_value(0) == 2 and pa_state.location_value(2) == 2))
    }

    void check_loc_guard(const AbstractState& pa_state) {
        // purpose: step/succ gen. must check whether edge is in current location,
        // for the sync edge. with destination loc7, this is not the case
        TS_ASSERT(pa_state.location_value(0) != 7)
    }

    void check_sync_alt(const AbstractState& pa_state) {
        // purpose succ gen must respect system composition: "label_alt" sync is independent of "label"-sync
        auto offset = pa_state.get_size_locs();
        TS_ASSERT(pa_state.location_value(0) == 5
                  and pa_state.location_value(1) == 0
                  and pa_state.location_value(2) == 5
                  and pa_state.location_value(3) == 0
                  and pa_state.location_value(4) == 0
                  and not pa_state.predicate_value(5 - offset)
                  and pa_state.predicate_value(6 - offset) // x = 10 >= 0
                  and pa_state.predicate_value(7 - offset)// y = 10 >= 0
                  and not pa_state.predicate_value(8 - offset) // x = 10 != 1
                  and not pa_state.predicate_value(9 - offset) // y = 10 != 1
                  and pa_state.predicate_value(10 - offset) // A[0] = 1
                  and pa_state.predicate_value(11 - offset) // A[1] = 1
                  and not pa_state.predicate_value(12 - offset) // unaffected
                  and not pa_state.predicate_value(13 - offset) // unaffected
                  and pa_state.predicate_value(14 - offset) // unaffected z = 0
                  and pa_state.predicate_value(15 - offset) // unaffected A[2] = 0
                  and pa_state.predicate_value(16 - offset) // x = 10 > 3
        )
    }

    void check_silent(const AbstractState& pa_state) {
        // purpose silent edges must be executed independent of system composition
        // focus on silent edges in A_non_sync
        // additionally this checks whether the variable source (upper) bounds are respected, (edge with x <= 42 is sat, but x > 42 not, thus we must have z=1)
        auto offset = pa_state.get_size_locs();
        TS_ASSERT(pa_state.location_value(0) == 0
                  and pa_state.location_value(1) == 2
                  and pa_state.location_value(2) == 0
                  and pa_state.location_value(3) == 0
                  and pa_state.location_value(4) == 0
                  and not pa_state.predicate_value(5 - offset)
                  and pa_state.predicate_value(6 - offset) // x >= 0
                  and pa_state.predicate_value(7 - offset) // y >= 0
                  and not pa_state.predicate_value(8 - offset) // unaffected
                  and not pa_state.predicate_value(9 - offset) // unaffected
                  and not pa_state.predicate_value(10 - offset) // unaffected
                  and not pa_state.predicate_value(11 - offset) // unaffected
                  and not pa_state.predicate_value(12 - offset) // unaffected
                  and not pa_state.predicate_value(13 - offset) // unaffected
                  and not pa_state.predicate_value(14 - offset) //  z = 1
                  and pa_state.predicate_value(15 - offset) // unaffected
                  and not pa_state.predicate_value(16 - offset) // unaffected
        )
    }

    void check_non_sync(const AbstractState& pa_state) {
        // purpose single-edge syncs must be executed if in system composition
        // focus on silent edges in A_non_sync
        // when transitioning to loc3 z=1 is set ("non_sync" vs. "null_sync", whereby the latter is not possible as not in system composition)
        auto offset = pa_state.get_size_locs();
        TS_ASSERT(pa_state.location_value(0) == 0
                  and pa_state.location_value(1) == 3
                  and pa_state.location_value(2) == 0
                  and pa_state.location_value(3) == 0
                  and pa_state.location_value(4) == 0
                  and not pa_state.predicate_value(5 - offset)
                  and pa_state.predicate_value(6 - offset) // x >= 0
                  and pa_state.predicate_value(7 - offset) // y >= 0
                  and not pa_state.predicate_value(8 - offset) // unaffected
                  and not pa_state.predicate_value(9 - offset) // unaffected
                  and not pa_state.predicate_value(10 - offset) // unaffected
                  and not pa_state.predicate_value(11 - offset) // unaffected
                  and not pa_state.predicate_value(12 - offset) // unaffected
                  and not pa_state.predicate_value(13 - offset) // unaffected
                  and not pa_state.predicate_value(14 - offset) //  z = 1
                  and pa_state.predicate_value(15 - offset) // unaffected
                  and not pa_state.predicate_value(16 - offset) // unaffected
        )
    }

    void check_edge_sync_1_2(const AbstractState& pa_state) {
        // x=0: A[x]=1 | x++ and y>=x: A[1]=2 | y--
        auto offset = pa_state.get_size_locs();
        TS_ASSERT((pa_state.location_value(0) == 1 or pa_state.location_value(0) == 2)
                  and pa_state.location_value(1) == 0
                  and (pa_state.location_value(2) == 3 or pa_state.location_value(2) == 4)
                  and pa_state.location_value(3) == 0
                  and pa_state.location_value(4) == 1
                  and not pa_state.predicate_value(5 - offset)
                  and pa_state.predicate_value(6 - offset) // x >= 0
                  and not pa_state.predicate_value(12 - offset) // w == -42 (unaffected)
                  and not pa_state.predicate_value(13 - offset) // w == -42 (unaffected)
                  and pa_state.predicate_value(14 - offset) // w == -42 (unaffected)
                  and pa_state.predicate_value(15 - offset) // A[2] = 0 (unaffected)
                  and not pa_state.predicate_value(16 - offset) // x > 3 (unaffected)
        )
        // case analysis
        if (pa_state.location_value(0) == 1 and pa_state.location_value(2) == 3) { // A[x] = 1, A[1] = 2
            TS_ASSERT(
                pa_state.predicate_value(7 - offset) // y >= 0 (unaffected)
                and not pa_state.predicate_value(8 - offset) // !(x = 1) (x=0)
                and not pa_state.predicate_value(9 - offset) // !(y = 1) (unaffected)
                and pa_state.predicate_value(10 - offset) // A[0] = 1
                and not pa_state.predicate_value(11 - offset) // !(A[1] = 1) (A[1] = 2)
            )
        }
        if (pa_state.location_value(0) == 1 && pa_state.location_value(2) == 4) { // A[x] = 1, y = y-1
            TS_ASSERT(
                (pa_state.predicate_value(7 - offset) ? true : not pa_state.predicate_value(9 - offset)) // sanity check y < 0 -> y != 1
                and not pa_state.predicate_value(8 - offset) // !(x = 1) (unaffected)
                and pa_state.predicate_value(10 - offset) // A[0] = 1
                and not pa_state.predicate_value(11 - offset) // !(A[1] = 1) (unaffected)
            )
            // additional: check that there exist some state where y = 1 and some state where y <= 0
            if (not pa_state.predicate_value(7 - offset)) { sync_1_2_1_3_y_neg = true; }
            if (pa_state.predicate_value(9 - offset)) { sync_1_2_1_3_y_is_1 = true; }
        }
        if (pa_state.location_value(0) == 2 && pa_state.location_value(2) == 3) { // x = x + 1, A[1] = 2
            TS_ASSERT(
                pa_state.predicate_value(7 - offset) // y  >= 0 (unaffected)
                and pa_state.predicate_value(8 - offset) == 1 // x = 1 (x=0 && x++ -> x' = 1)
                and not pa_state.predicate_value(9 - offset) // !(y = 1) (unaffected)
                and not pa_state.predicate_value(10 - offset) // !(A[0] = 1)  (unaffected)
                and not pa_state.predicate_value(11 - offset) // !(A[1] = 1) (A[1] = 2)
            )
        }
        if (pa_state.location_value(0) == 2 && pa_state.location_value(2) == 4) { // x = x + 1, y = y-1
            TS_ASSERT(
                (pa_state.predicate_value(7 - offset) ? true : not pa_state.predicate_value(9 - offset)) // sanity check  y < 0 -> y != 1
                and pa_state.predicate_value(8 - offset) == 1 // (x = 1) (x=0 && x++ -> x' = 1)
                and not pa_state.predicate_value(10 - offset) // !(A[0] = 1) (unaffected)
                and not pa_state.predicate_value(11 - offset) // !(A[1] = 1) (unaffected)
            )
            // additional: check that there exist some state where y = 1 and some state where y <= 0
            if (not pa_state.predicate_value(7 - offset)) { sync_1_2_2_4_y_neg = true; }
            if (pa_state.predicate_value(9 - offset)) { sync_1_2_2_4_y_is_1 = true; }
        }

    }

    void check_edge_sync_2_1(const AbstractState& pa_state) {
        // y=0: A[y]=2 | w-- and x>=1: A[x]=1 | y++
        auto offset = pa_state.get_size_locs();
        TS_ASSERT((pa_state.location_value(0) == 3 or pa_state.location_value(0) == 4)
                  and pa_state.location_value(1) == 0
                  and (pa_state.location_value(2) == 1 or pa_state.location_value(2) == 2)
                  and pa_state.location_value(3) == 0
                  and pa_state.location_value(4) == 1
                  and not pa_state.predicate_value(5 - offset)
                  and pa_state.predicate_value(6 - offset) // x >= 0 (unaffected)
                  and pa_state.predicate_value(7 - offset) // y  >= 0
                  and not pa_state.predicate_value(8 - offset) // x = 1 (unaffected)
                  and not pa_state.predicate_value(10 - offset) // A[0] = 1 (A[y] = 2 or unaffected)
                  and not pa_state.predicate_value(11 - offset) // A[1] = 1 (note predicate x=1 is false in the initial state)
                  and not pa_state.predicate_value(13 - offset) // w < -42 (test on src/target bounds of variables)
                  and pa_state.predicate_value(14 - offset) // w == -42 (unaffected)
                  and not pa_state.predicate_value(16 - offset) // x > 3 (unaffected)
        )
        // case analysis
        if (pa_state.location_value(0) == 3 and pa_state.location_value(2) == 1) { // A[y] = 2, A[x] = 1
            TS_ASSERT(
                not pa_state.predicate_value(9 - offset) // !(y = 1) (unaffected)
                and not pa_state.predicate_value(12 - offset) // !(w == -42) (unaffected)
                and not pa_state.predicate_value(15 - offset) // !(A[2] = 0) (note: we know x >= 1, as it is used as an array index, we conclude x <= 2, therewith predicate (x=1) being false tells us x=2 initially)
            )
        }
        if (pa_state.location_value(0) == 3 and pa_state.location_value(2) == 2) { // A[y] = 2, y++
            TS_ASSERT(
                pa_state.predicate_value(9 - offset)  // y = 1 (y=0 and y++)
                and not pa_state.predicate_value(12 - offset) // !(w == -42) (unaffected)
                and pa_state.predicate_value(15 - offset) // A[2] = 0 (note: unaffected as y=0)
            )
        }
        if (pa_state.location_value(0) == 4 and pa_state.location_value(2) == 1) { // w--, A[x] = 1
            TS_ASSERT(
                not pa_state.predicate_value(9 - offset) // !(y = 1) (unaffected)
                and not pa_state.predicate_value(15 - offset) // !(A[2] = 0) (note: we know x >= 1, as it is used as an array index, we conclude x <= 2, therewith predicate (x=1) being false tells us x=2 initially)
            )
            if (pa_state.predicate_value(12 - offset)) { sync_2_1_4_1_w_neg = true; } // check that lower bound on w is hit at least once
        }
        if (pa_state.location_value(0) == 4 and pa_state.location_value(2) == 2) { // w--, , y++
            TS_ASSERT(
                pa_state.predicate_value(9 - offset) // y = 1 (y=0 and y++)
                and pa_state.predicate_value(15 - offset) // A[2] = 0 (unaffected)
            )
            if (pa_state.predicate_value(12 - offset)) { sync_2_1_4_2_w_neg = true; } // check that lower bound on w is hit at least once
        }

    }

    void check_edge_sync_2_2(const AbstractState& pa_state) {
        // y=0: A[y]=2 | w-- and y>=x: A[1]=2 | y--
        auto offset = pa_state.get_size_locs();
        TS_ASSERT((pa_state.location_value(0) == 3 or pa_state.location_value(0) == 4)
                  and pa_state.location_value(1) == 0
                  and (pa_state.location_value(2) == 3 or pa_state.location_value(2) == 4)
                  and pa_state.location_value(3) == 0
                  and pa_state.location_value(4) == 1
                  and not pa_state.predicate_value(5 - offset)
                  and pa_state.predicate_value(6 - offset) // x >= 0 (unaffected)
                  and not pa_state.predicate_value(8 - offset) // x = 1 (unaffected)
                  and not pa_state.predicate_value(9 - offset) // y = 1 (y = 0; unaffected or decrement)
                  and not pa_state.predicate_value(10 - offset) // A[0] = 1 (A[y] = 2 or unaffected)
                  and not pa_state.predicate_value(11 - offset) // A[1] = 1 (A[1] = 2 or unaffected)
                  and not pa_state.predicate_value(13 - offset) // w < -42 (test on src/target bounds of variables)
                  and pa_state.predicate_value(14 - offset) // z = 0 (unaffected)
                  and pa_state.predicate_value(15 - offset) // A[2] = 0 (unaffected)
                  and not pa_state.predicate_value(16 - offset) // x > 3 (unaffected)
        )
        // case analysis
        if (pa_state.location_value(0) == 3 and pa_state.location_value(2) == 3) { // A[y] = 2, A[1]=2
            TS_ASSERT(
                pa_state.predicate_value(7 - offset) // y >= 0 (unaffected)
                and not pa_state.predicate_value(12 - offset) // !(w == -42) (unaffected)
            )
        }
        if (pa_state.location_value(0) == 3 and pa_state.location_value(2) == 4) { // A[y] = 2, y--
            TS_ASSERT(
                not pa_state.predicate_value(7 - offset) // !(y >= 0) (y = 0 and y--)
                and not pa_state.predicate_value(12 - offset) // !(w == -42) (unaffected)
            )
        }
        if (pa_state.location_value(0) == 4 and pa_state.location_value(2) == 3) { // w--, A[1]=2
            TS_ASSERT(
                pa_state.predicate_value(7 - offset) // (y >= 0) (unaffected)
            )
            if (pa_state.predicate_value(12 - offset)) { sync_2_2_4_3_w_neg = true; } // check that lower bound on w is hit at least once
        }
        if (pa_state.location_value(0) == 4 and pa_state.location_value(2) == 4) { // w--, , y--
            TS_ASSERT(
                not pa_state.predicate_value(7 - offset) // !(y >= 0) (y = 0 and y--)
            )
            if (pa_state.predicate_value(12 - offset)) { sync_2_2_4_4_w_neg = true; } // check that lower bound on w is hit at least once
        }

    }

    /******************************************************************************************************************/

public:

    void set_up() override {
        // Reset successors from last test to be unreached.
        for (auto state_id: predicateAbs->frontier->to_list()) { predicateAbs->searchSpace->get_node(state_id).set_unreached(); }
        predicateAbs->frontier->clear();

        // Reset initial state to be unreached.
        auto node_initial_state = predicateAbs->searchSpace->get_node(predicateAbs->searchSpace->_initialState());
        if (node_initial_state.is_reached()) { node_initial_state.set_unreached(); }
    }

    void test_sync() {
        PLAJA_LOG("\nComplex test on predicate abstraction and underlying successor gen. w.r.t. synchronisation,")
        PLAJA_LOG("test on unassigned variables and variable bounds on-the-fly:")

        // set initial state ...
        auto node = predicateAbs->searchSpace->add_node(predicateAbs->searchSpace->get_abstract_initial_state());
        node.set_reached_init();
        predicateAbs->frontier->push(node);

        predicateAbs->step();
        TS_ASSERT_EQUALS(predicateAbs->frontier->size(), 23) // regression test

        // check successors ...
        for (const StateID_type state_id: predicateAbs->frontier->to_list()) {
            const auto pa_state_ptr = predicateAbs->searchSpace->get_abstract_state(state_id);
            const auto& pa_state = *pa_state_ptr;
            check_comb_guard(pa_state);
            check_loc_guard(pa_state);

            if (pa_state.location_value(0) == 5) { // alternative synchronisation: label_alt{1,2}
                check_sync_alt(pa_state);
                continue;
            }
            if (pa_state.location_value(1) == 2) { // silent edges in A_non_sync
                check_silent(pa_state);
                continue;
            }
            if (pa_state.location_value(1) == 3) { // "non_sync" (vs. "null_sync") in A_non_sync
                check_non_sync(pa_state);
                continue;
            }
            // edge A1.1 and A2.2 (identified via destination locations)
            if ((pa_state.location_value(0) == 1 && pa_state.location_value(2) == 3) || (pa_state.location_value(0) == 1 && pa_state.location_value(2) == 4) || (pa_state.location_value(0) == 2 && pa_state.location_value(2) == 3) || (pa_state.location_value(0) == 2 && pa_state.location_value(2) == 4)) {
                check_edge_sync_1_2(pa_state);
                continue;
            }
            // edge A1.2 and A2.1 (identified via destination locations)
            if ((pa_state.location_value(0) == 3 && pa_state.location_value(2) == 1) || (pa_state.location_value(0) == 3 && pa_state.location_value(2) == 2) || (pa_state.location_value(0) == 4 && pa_state.location_value(2) == 1) || (pa_state.location_value(0) == 4 && pa_state.location_value(2) == 2)) {
                check_edge_sync_2_1(pa_state);
                continue;
            }
            // edge A1.2 and A2.2 (identified via destination locations)
            if ((pa_state.location_value(0) == 3 && pa_state.location_value(2) == 3) || (pa_state.location_value(0) == 3 && pa_state.location_value(2) == 4) || (pa_state.location_value(0) == 4 && pa_state.location_value(2) == 3) || (pa_state.location_value(0) == 4 && pa_state.location_value(2) == 4)) {
                check_edge_sync_2_2(pa_state);
                continue;
            }
            TS_ASSERT(false);
        }
        check_existence_flags();

        // check whether nodes are identified to be reached by re-stepping over initial state
        predicateAbs->frontier->clear();
        predicateAbs->frontier->push(node);
        predicateAbs->step();
        TS_ASSERT_EQUALS(predicateAbs->frontier->size(), 0)
    }

    void test_bounds_on_arrays() {
        PLAJA_LOG_DEBUG("\nTest whether size and range bounds of arrays are respected:")

        // construct new source state:
        auto abstract_state = predicateAbs->searchSpace->get_abstract_initial_state();
        auto pa_values = abstract_state->to_pa_state_values(true);
        pa_values->set_predicate_value(0, true); // "A4_enabled = true"
        // set new source state:
        auto node = predicateAbs->searchSpace->add_node(predicateAbs->searchSpace->set_abstract_state(*pa_values));
        node.set_reached_init();
        predicateAbs->frontier->push(node);

        // step
        predicateAbs->step();
        TS_ASSERT_EQUALS(predicateAbs->frontier->size(), 4) // we expect two targets from first res. third edge of A4
        int flag = 0;
        bool x_1_seen = false; // for third edge, we have x==1 fulfilled exactly once
        for (const StateID_type state_id: predicateAbs->frontier->to_list()) {
            auto pa_state_ptr = predicateAbs->searchSpace->get_abstract_state(state_id);
            const auto& pa_state = *pa_state_ptr;
            auto offset = pa_state.get_size_locs();
            TS_ASSERT_EQUALS(pa_state.location_value(3), 1) // for both edges A4 goes to location 1
            TS_ASSERT(not pa_state.predicate_value(16 - offset)) // key aspect is that !(x > 3) holds (note that x = 3 due to update on x, which we do to verify the effect on the target value, while in guards we do it solely on source)
            // on the fly test for unassigned
            TS_ASSERT(pa_state.predicate_value(14 - offset)) // z==0
            if (flag++ < 2) { // checks on edge 1
                TS_ASSERT_EQUALS(pa_state.predicate_value(8 - offset), pa_state.predicate_value(10 - offset)) // x == 1 iff A[0] = 1; note: we know in the source !(x==1) hence either x==0 or x==2
                if (pa_state.predicate_value(8 - offset)) {
                    TS_ASSERT(pa_state.predicate_value(15 - offset)) // then still A[2] = 0
                } else {
                    TS_ASSERT(not pa_state.predicate_value(16 - offset))
                }
                // check via output if needed: all other predicates remain unchanged
            } else { // checks on edge 3
                // state differ only w.r.t. x=1
                if (x_1_seen) { TS_ASSERT(not pa_state.predicate_value(8 - offset)) }
                else {
                    if (pa_state.predicate_value(8 - offset)) { x_1_seen = true; }
                }
                // on the fly test for unassigned
                TS_ASSERT(pa_state.predicate_value(15 - offset)) // A[2]==0 for edge 3 always
                // check via output if needed: all other predicates remain unchanged
            }
        }
        TS_ASSERT(x_1_seen) // final check for third edge

    }

    void test_on_sync_blocked_due_disabled_edges() {
        PLAJA_LOG_DEBUG("\nTest on sync blocked due to disabled edges:")

        auto abstract_state = predicateAbs->searchSpace->get_abstract_initial_state();
        auto pa_values = abstract_state->to_pa_state_values(true);
        pa_values->set_location(2, 5); // A2
        // set new source state:
        auto node = predicateAbs->searchSpace->add_node(predicateAbs->searchSpace->set_abstract_state(*pa_values));
        node.set_reached_init();
        predicateAbs->frontier->push(node);

        // step
        predicateAbs->step();
        TS_ASSERT_EQUALS(predicateAbs->frontier->size(), 2) // regression test
        for (const StateID_type state_id: predicateAbs->frontier->to_list()) {
            const auto pa_state = predicateAbs->searchSpace->get_abstract_state(state_id);
            TS_ASSERT(pa_state->location_value(0) == 0 && pa_state->location_value(4) == 0) // "label" sync must *not* be possible (also "label_alt")
        }
    }

    void test_on_sync_blocked_due_to_missing_label() {
        PLAJA_LOG("\nTest on sync blocked due to a participating automaton without a respectively labeled edge:")

        // Adapt sync directive to disable "label_alt"-sync:
        auto model = modelConstructor.constructedModel->deep_copy();
        Synchronisation* sync = model->get_system()->get_sync(1);
        sync->set_syncAction("label_alt1", 1); // *but* A_non_sync has no such labeled edge
        sync->set_syncActionID(1, 1);

        model->compute_model_information();

        auto predicate_abs = PLAJA_UTILS::cast_unique<PredicateAbstraction>(modelConstructor.construct_instance(SearchEngineFactory::PaType, *model, *modelConstructor.cachedPropertyInfo->get_property()));
        predicate_abs->initialize();
        predicate_abs->step();
        TS_ASSERT_EQUALS(predicate_abs->frontier->size(), 22) // regression test
        for (const StateID_type state_id: predicate_abs->frontier->to_list()) {
            auto abstract_state_n = predicate_abs->searchSpace->get_abstract_state(state_id);
            TS_ASSERT(abstract_state_n->location_value(0) != 5 && abstract_state_n->location_value(2) != 5) // "label_alt" sync must not be possible
        }
    }

/**********************************************************************************************************************/

    void run_tests() override {
        RUN_TEST(test_initial_state())
        RUN_TEST(test_sync())
        RUN_TEST(test_initial_state())
        RUN_TEST(test_bounds_on_arrays())
        RUN_TEST(test_on_sync_blocked_due_disabled_edges())
        RUN_TEST(test_on_sync_blocked_due_to_missing_label())
    }

};

TEST_MAIN(PredicateAbstractionTest)

#endif //PLAJA_PREDICATE_ABSTRACTION_TEST_H
