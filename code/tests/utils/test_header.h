//
// This file is part of the PlaJA code base.
// Copyright (c) (2019 - 2022) Marcel Vinzent.
// See README.md in the top-level directory for licensing information.
//

#ifndef PLAJA_TEST_HEADER_H
#define PLAJA_TEST_HEADER_H

#include <sstream>
#include <cstdlib>
#include "../../exception/plaja_exception.h"
#include "../../utils/utils.h"
#include "test_parser.h"
#include "test_utils.h"

namespace PLAJA_TEST {

    using TestResult = int;
    constexpr TestResult SUCCESS = 0;
    constexpr TestResult FAIL = 1;

    class TestSuite {
    private:
        TestResult rlt;

    public:

        TestSuite():
            rlt(SUCCESS) {};
        virtual ~TestSuite() = default;
        DELETE_CONSTRUCTOR(TestSuite)

        void mark_failed() { rlt = FAIL; }

        [[nodiscard]] bool is_failed() const { return rlt == FAIL; }

        [[nodiscard]] TestResult _rlt() const { return rlt; }

        virtual void set_up() {}

        virtual void run_tests() = 0;
    };

}

// MACROS

#define TS_ASSERT(CONDITION) \
    {\
    if (not (CONDITION)) {\
        std::stringstream str_tmp;\
        this->mark_failed();\
        str_tmp << PLAJA_UTILS::redColorStr << "TEST FAILED: " << static_cast<const char*>(__FUNCTION__) << " (line " << __LINE__ << ")." << PLAJA_UTILS::resetColorStr << PLAJA_UTILS::lineBreakString;\
        throw PlajaException(str_tmp.str());\
    }\
    }
// #define TS_ASSERT(CONDITION) assert(CONDITION); // debugging alternative

#define TS_ASSERT_EQUALS(VAL_1, VAL_2) TS_ASSERT((VAL_1) == (VAL_2))

#define RUN_TEST(CALL) set_up(); /*try {*/ (CALL); /*} catch (PlajaException& e) { std::cout << e.what() << PLAJA_UTILS::lineBreakString; };*/

#define TEST_MAIN(TEST_CLASS) int main(int /*argc*/, char** /*argv*/) { TEST_CLASS test; test.run_tests(); if (not test.is_failed()) { std::cout << std::endl << PLAJA_UTILS::greenColorStr << "SUCCESS" << PLAJA_UTILS::resetColorStr << PLAJA_UTILS::lineBreakString; } return test._rlt(); }

#endif //PLAJA_TEST_HEADER_H
