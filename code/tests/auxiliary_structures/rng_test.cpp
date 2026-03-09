//
// This file is part of the PlaJA code base.
// Copyright (c) (2019 - 2022) Marcel Vinzent.
// See README.md in the top-level directory for licensing information.
//

#ifndef PLAJA_RNG_TEST_CPP
#define PLAJA_RNG_TEST_CPP

#include "../utils/test_header.h"
#include <unordered_map>
#include "../../utils/floating_utils.h"
#include "../../utils/rng.h"

class RngTest: public PLAJA_TEST::TestSuite {

private:
    RandomNumberGenerator rng;

    // aux:

    std::size_t iterations = 0;
    double countTolerance = 0;
    double meanTolerance = 0;

    template<typename Result_t>
    void check_result(const std::unordered_map<Result_t, double>& counter, double sum, double lower, double upper) { // NOLINT(*-easily-swappable-parameters)
        const double expected_count = PLAJA_UTILS::cast_numeric<double>(iterations) / (upper - lower + 1);
        const double expected_mean = lower + (upper - lower) / 2.0;

        double max_count_diff = 0;
        for (const auto& [number, count]: counter) {
            max_count_diff = std::max(max_count_diff, std::abs(count - expected_count));
            TS_ASSERT(PLAJA_FLOATS::equal(count, expected_count, countTolerance))
            if (not PLAJA_FLOATS::equal(count, expected_count, countTolerance)) {
                std::cout << "Number: " << number << ", count: " << count << ", expected: " << expected_count << std::endl;
            }
        }
        std::cout << "Max count difference: " << max_count_diff << std::endl;

        auto mean = sum / PLAJA_UTILS::cast_numeric<double>(iterations);
        TS_ASSERT(PLAJA_FLOATS::equal(mean, expected_mean, meanTolerance))

        std::cout << "Mean: " << mean << ", expected mean: " << expected_mean << std::endl;
    }

public:

    RngTest():
        rng(42) {}

    ~RngTest() override = default;

    DELETE_CONSTRUCTOR(RngTest)

/**********************************************************************************************************************/

    void testDefault() {
        std::cout << std::endl << "Test default \"next\":" << std::endl;

        iterations = 1E6;

        auto found_neg = false;
        auto found_pos = false;
        std::unordered_map<RandomNumberGenerator::ResultType, unsigned> counter;

        for (std::size_t i = 0; i < iterations; ++i) {
            auto rlt = rng.sample();
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wpedantic"
#pragma GCC diagnostic ignored "-Wtype-limits"
#pragma ide diagnostic ignored "ConstantConditionsOC"
            if (rlt < 0) { found_neg = true; }
            else { found_pos = true; }
#pragma GCC diagnostic pop
            TS_ASSERT(++counter[rlt] <= 3) // would not expect the same element more than three times
        }

        TS_ASSERT(not found_neg)
        TS_ASSERT(found_pos)
    }

    void testBounded() {
        std::cout << std::endl << "Test bounded sampling:" << std::endl;

        iterations = 1E7;
        countTolerance = 2500;
        meanTolerance = 0.1;

        const RandomNumberGenerator::ResultType l = 11;
        const RandomNumberGenerator::ResultType u = 20;

        std::unordered_map<RandomNumberGenerator::ResultType, double> counter;
        double sum = 0;

        for (std::size_t i = 0; i < iterations; ++i) {
            auto rlt = rng.sample_unsigned(l, u);
            TS_ASSERT(l <= rlt and rlt <= u)
            ++counter[rlt];
            sum += rlt;
        }

        check_result(counter, sum, l, u);
    }

    void testIndex() {
        std::cout << std::endl << "Test index sampling:" << std::endl;

        iterations = 1E7;
        countTolerance = 1000;
        meanTolerance = 0.1;

        const std::size_t size = 100;

        std::unordered_map<RandomNumberGenerator::ResultType, double> counter;
        double sum = 0;

        for (std::size_t i = 0; i < iterations; ++i) {
            auto rlt = rng.index(size);
            TS_ASSERT(rlt < size)
            ++counter[rlt];
            sum += PLAJA_UTILS::cast_numeric<double>(rlt);
        }

        check_result(counter, sum, 0, size - 1);
    }

    void testSanity() {
        std::cout << std::endl << "Test bounded sampling sanity:" << std::endl;
        for (std::size_t i = 0; i < 100; ++i) { TS_ASSERT_EQUALS(rng.sample_unsigned(0, 0), 0) }
        for (std::size_t i = 0; i < 100; ++i) { TS_ASSERT_EQUALS(rng.index(1), 0) }
    }

    void testBoundedIntermingled() {
        std::cout << std::endl << "Test bounded sampling intermingled:" << std::endl;

        iterations = 1E7;
        countTolerance = 1000;
        meanTolerance = 0.1;

        const RandomNumberGenerator::ResultType l = 1000;
        const RandomNumberGenerator::ResultType u = 2000;
        const std::size_t size = 500;

        std::unordered_map<RandomNumberGenerator::ResultType, double> counter1;
        double sum1 = 0;
        std::unordered_map<RandomNumberGenerator::ResultType, double> counter2;
        double sum2 = 0;

        for (std::size_t i = 0; i < iterations; ++i) {
            auto rlt = rng.sample_unsigned(l, u);
            TS_ASSERT(l <= rlt and rlt <= u)
            ++counter1[rlt];
            sum1 += rlt;
            //
            rlt = rng.index(size);
            TS_ASSERT(rlt < size)
            ++counter2[rlt];
            sum2 += rlt;
        }

        check_result(counter1, sum1, l, u);
        check_result(counter2, sum2, 0, size - 1);
    }

    /******************************************************************************************************************/

    void testSigned() {
        std::cout << std::endl << "Test signed \"next\":" << std::endl;

        iterations = 1E6;

        std::size_t count_neg = 0;
        std::size_t count_pos = 0;
        std::unordered_map<RandomNumberGenerator::SignedType, unsigned> counter;
        // double sum = 0;

        for (std::size_t i = 0; i < iterations; ++i) {
            auto rlt = rng.sample_signed();
            if (rlt < 0) { ++count_neg; }
            else { ++count_pos; }
            TS_ASSERT(++counter[rlt] <= 3) // would not expect the same element more than three times
            // sum += rlt;
        }

        TS_ASSERT(count_pos)
        TS_ASSERT(count_neg)
        auto expected_pos_neg = PLAJA_UTILS::cast_numeric<double>(iterations) * 0.5;
        TS_ASSERT(PLAJA_FLOATS::equal<double>(count_pos, expected_pos_neg, 1000))
        TS_ASSERT(PLAJA_FLOATS::equal<double>(count_neg, expected_pos_neg, 1000))
        std::cout << "Counted positive: " << count_pos << ", counted negative: " << count_neg << std::endl;

        // auto mean = sum / iterations;
        // TS_ASSERT(PLAJA_UTILS::equal<double>(mean, -1, 0.1))
        // std::cout << "Mean: " << mean << " expected mean: " << -1 << std::endl;
    }

    void testBoundedSigned() {
        std::cout << std::endl << "Test bounded signed sampling:" << std::endl;

        iterations = 1E7;
        countTolerance = 2500;
        meanTolerance = 0.1;

        const RandomNumberGenerator::SignedType l = -10;
        const RandomNumberGenerator::SignedType u = 10;

        std::unordered_map<RandomNumberGenerator::SignedType, double> counter;
        double sum = 0;

        for (std::size_t i = 0; i < iterations; ++i) {
            auto rlt = rng.sample_signed(l, u);
            TS_ASSERT(l <= rlt and rlt <= u)
            ++counter[rlt];
            sum += rlt;
        }

        check_result(counter, sum, l, u);
    }

    /******************************************************************************************************************/

    void testProb() {
        std::cout << std::endl << "Test probability sampling:" << std::endl;

        iterations = 1000000;
        const double conv_tolerance = 0.1;

        double sum = 0;
        for (std::size_t i = 0; i < iterations; ++i) { sum += rng.prob(); }

        auto mean = sum / PLAJA_UTILS::cast_numeric<double>(iterations);
        TS_ASSERT(PLAJA_FLOATS::equal(mean, 0.5, conv_tolerance))

        std::cout << "Mean: " << mean << std::endl;
    }

/**********************************************************************************************************************/

    void run_tests() override {
        RUN_TEST(testDefault())
        RUN_TEST(testBounded())
        RUN_TEST(testIndex())
        RUN_TEST(testSanity())
        RUN_TEST(testBoundedIntermingled())
        RUN_TEST(testSigned())
        RUN_TEST(testBoundedSigned())
        RUN_TEST(testProb())
    }

};

TEST_MAIN(RngTest)

#endif //PLAJA_RNG_TEST_CPP
