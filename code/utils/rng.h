//
// This file is part of the PlaJA code base.
// Copyright (c) (2019 - 2022) Marcel Vinzent.
// See README.md in the top-level directory for licensing information.
//

#ifndef PLAJA_RNG_H
#define PLAJA_RNG_H

#include "../utils/utils.h"

struct RNGInternals;

class RandomNumberGenerator {
public:
    using ResultType = uint_fast32_t;
    using SignedType = int;
    using FloatType = double;

private:
    std::unique_ptr<RNGInternals> rngInternals;
    FIELD_IF_DEBUG(ResultType seed;)

public:
    RandomNumberGenerator();
    explicit RandomNumberGenerator(ResultType seed);
    ~RandomNumberGenerator();
    DELETE_CONSTRUCTOR(RandomNumberGenerator)

    inline static void set_global_seed(unsigned int seed) { srand(seed); }

    [[nodiscard]] inline static int get_global_rand() { return rand(); }

    void set_seed(ResultType seed);
    FCT_IF_DEBUG([[nodiscard]] ResultType get_seed() const { return seed; })

    /* Unsigned. */

    ResultType sample();

    ResultType sample_unsigned(ResultType lower, ResultType upper);

    template<typename Result_t = std::size_t>
    inline Result_t index(std::size_t size) {
        PLAJA_ASSERT(PLAJA_UTILS::is_non_negative(size))
        PLAJA_ASSERT(PLAJA_UTILS::is_non_narrowing_conversion<Result_t>(size))
        return sample_unsigned(0, size - 1);
    }

    /* Signed. */

    inline SignedType sample_signed() { return static_cast<SignedType>(sample()); }

    inline SignedType sample_signed(SignedType lower, SignedType upper) { return static_cast<SignedType>(sample_unsigned(0, upper - lower)) + lower; }

    /* Continuous. */
    FloatType prob(); // random real in [0..1)

    FloatType sample_float(FloatType lower_in, FloatType upper_ex);

    FloatType sample_float_inverse(FloatType lower_ex, FloatType upper_in); // MV: for sake of ...

    /* Bool */
    inline bool flag() { return sample_unsigned(0, 1) == 0; }

    /* Shuffle. */

    void shuffle(std::vector<int>& vec);

    void shuffle(std::vector<unsigned int>& vec);

};

namespace PLAJA_GLOBAL { extern std::unique_ptr<RandomNumberGenerator> rng; }

#endif //PLAJA_RNG_H
