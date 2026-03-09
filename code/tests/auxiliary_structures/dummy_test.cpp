//
// This file is part of the PlaJA code base.
// Copyright (c) (2019 - 2022) Marcel Vinzent.
// See README.md in the top-level directory for licensing information.
//

#include "../utils/test_header.h"

class DummyTest: public PLAJA_TEST::TestSuit {

public:
    DummyTest() = default;
    ~DummyTest() override = default;

    void testDummy() { TS_ASSERT(true); }

    void run_tests() override { RUN_TEST(testDummy()) }

};

TEST_MAIN(DummyTest)
