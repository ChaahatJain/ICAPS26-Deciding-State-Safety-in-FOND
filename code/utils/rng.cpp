//
// This file is part of the PlaJA code base.
// Copyright (c) (2019 - 2022) Marcel Vinzent.
// See README.md in the top-level directory for licensing information.
//

#include "rng.h"
#include <algorithm>
#include <random>
#include "../include/compiler_config_const.h"
#include "../search/factories/configuration.h"
#include "../utils/floating_utils.h"

struct RNGInternals {
    friend RandomNumberGenerator;

private:
    std::random_device dev;
    std::mt19937 mtRNG;
    std::uniform_int_distribution<RandomNumberGenerator::ResultType> distro;
    // std::uniform_int_distribution<RandomNumberGenerator::SignedType> signedDistro;
    std::uniform_real_distribution<RandomNumberGenerator::FloatType> probDistro;

    RandomNumberGenerator::ResultType sample(RandomNumberGenerator::ResultType l, RandomNumberGenerator::ResultType u) { return distro(mtRNG, std::uniform_int_distribution<RandomNumberGenerator::ResultType>::param_type(l, u)); }

    // RandomNumberGenerator::SignedType sample_signed(RandomNumberGenerator::SignedType l, RandomNumberGenerator::SignedType u) { return signedDistro(mtRNG, std::uniform_int_distribution<RandomNumberGenerator::SignedType>::param_type(l, u)); }

    RandomNumberGenerator::FloatType sample_prob() { return probDistro(mtRNG); }

    template<typename T> void shuffle(std::vector<T>& vec) { std::shuffle(vec.begin(), vec.end(), mtRNG); }

    RNGInternals():
        mtRNG(dev())
        , probDistro(0.0, 1.0) {} // mtRNG(std::chrono::system_clock::now().time_since_epoch().count())
public:
    ~RNGInternals() = default;
    DELETE_CONSTRUCTOR(RNGInternals)

};

RandomNumberGenerator::RandomNumberGenerator():
    rngInternals(new RNGInternals())
    CONSTRUCT_IF_DEBUG(seed(static_cast<ResultType>(-1))) {
    static_assert(PLAJA_UTILS::is_static_type<std::mt19937::result_type, ResultType>());
    static_assert(PLAJA_GLOBAL::isGCC or sizeof(ResultType) == 4); // on GCC apparently 8 bit
}

RandomNumberGenerator::RandomNumberGenerator(RandomNumberGenerator::ResultType seed):
    rngInternals(new RNGInternals())
    CONSTRUCT_IF_DEBUG(seed(seed)) {
    rngInternals->mtRNG.seed(seed);
    rngInternals->probDistro.reset();
}

RandomNumberGenerator::~RandomNumberGenerator() = default;

void RandomNumberGenerator::set_seed(RandomNumberGenerator::ResultType seed_v) {
    STMT_IF_DEBUG(seed = seed_v)
    rngInternals->mtRNG.seed(seed_v);
}

RandomNumberGenerator::ResultType RandomNumberGenerator::sample() { return rngInternals->mtRNG(); }

RandomNumberGenerator::ResultType RandomNumberGenerator::sample_unsigned(RandomNumberGenerator::ResultType lower, RandomNumberGenerator::ResultType upper) { return rngInternals->sample(lower, upper); }

// RandomNumberGenerator::SignedType RandomNumberGenerator::next_signed() { return rngInternals->sample_signed(std::numeric_limits<SignedType>::min(), std::numeric_limits<SignedType>::max()); }

// RandomNumberGenerator::SignedType RandomNumberGenerator::next_signed(RandomNumberGenerator::SignedType lower, RandomNumberGenerator::SignedType upper) { return rngInternals->sample_signed(lower, upper); }

RandomNumberGenerator::FloatType RandomNumberGenerator::prob() { return rngInternals->sample_prob(); }

RandomNumberGenerator::FloatType RandomNumberGenerator::sample_float(RandomNumberGenerator::FloatType lower_in, RandomNumberGenerator::FloatType upper_ex) {
    PLAJA_ASSERT(lower_in <= upper_ex)
    return lower_in + (upper_ex - lower_in) * prob();
}

RandomNumberGenerator::FloatType RandomNumberGenerator::sample_float_inverse(RandomNumberGenerator::FloatType lower_ex, RandomNumberGenerator::FloatType upper_in) {
    PLAJA_ASSERT(lower_ex <= upper_in)
    return upper_in - (upper_in + lower_ex) * prob();
}

void RandomNumberGenerator::shuffle(std::vector<int>& vec) { rngInternals->shuffle(vec); }

void RandomNumberGenerator::shuffle(std::vector<unsigned int>& vec) { rngInternals->shuffle(vec); }
