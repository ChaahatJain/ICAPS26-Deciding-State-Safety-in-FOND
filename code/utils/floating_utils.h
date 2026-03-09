//
// This file is part of the PlaJA code base.
// Copyright (c) (2019 - 2021) Marcel Vinzent.
// See README.md in the top-level directory for licensing information.
//

#ifndef PLAJA_FLOATING_UTILS_H
#define PLAJA_FLOATING_UTILS_H

/**
 * The routines in this file adapt from similar routines in Marabou (https://github.com/NeuralNetworkVerification/Marabou/),
 * specifically https://github.com/NeuralNetworkVerification/Marabou/blob/2a09b23ab508f273b108dc9e8705332843c38611/src/common/FloatUtils.h
 * and https://github.com/NeuralNetworkVerification/Marabou/blob/2a09b23ab508f273b108dc9e8705332843c38611/src/common/FloatUtils.cpp
 * (May 2021).
 */

#include <cfloat>
#include <cmath>
#include "../assertions.h"

namespace PLAJA_FLOATS {

    template<typename floating=double>
    inline double abs(floating x) { return std::fabs(x); }

    template<typename floating=double>
    inline bool is_finite(floating x) {
        PLAJA_ASSERT(not std::isfinite(std::numeric_limits<floating>::infinity()));
        assert(not std::isfinite(-std::numeric_limits<floating>::infinity()));
        return std::isfinite(x); // also NaN
    }

    template<typename floating=double>
    inline bool is_infinite(floating x) {
        PLAJA_ASSERT(std::isinf(std::numeric_limits<floating>::infinity()));
        PLAJA_ASSERT(std::isinf(-std::numeric_limits<floating>::infinity()));
        return std::isinf(x);
    }

    template<typename floating=double>
    inline bool is_nan(floating x) { return std::isnan(x); }

    template<typename floating=double>
    inline floating infinity() {
        PLAJA_ASSERT(std::numeric_limits<floating>::is_iec559);
        return std::numeric_limits<floating>::infinity();
    }

    template<typename floating=double>
    inline floating negative_infinity() {
        PLAJA_ASSERT(std::numeric_limits<floating>::is_iec559);
        return -std::numeric_limits<floating>::infinity();
    }

    template<typename floating=double>
    inline static floating max_value() { return std::numeric_limits<floating>::max(); }

    template<typename floating=double>
    inline floating min_positive_value() {
        static_assert(std::numeric_limits<floating>::min() > 0);
        return std::numeric_limits<floating>::min();
    }

    template<typename floating=double>
    inline floating min_absolute_value() {
        static_assert(std::numeric_limits<floating>::lowest() < 0);
        return std::numeric_limits<floating>::lowest();
    }

    // NOTE: *tolerance* is here the tolerance towards zero respectively towards equality.

    template<typename floating=double>
    inline bool is_zero(floating x, floating tolerance = std::numeric_limits<floating>::epsilon()) {
        PLAJA_ASSERT(tolerance > 0)
        floating lower = -tolerance;
        floating upper = tolerance;
        return (x - upper) * (x - lower) <= 0;
    }

    template<typename floating=double>
    inline bool is_positive(floating x, floating tolerance = std::numeric_limits<floating>::epsilon()) {
        PLAJA_ASSERT(tolerance > 0)
        return x > tolerance;
    }

    template<typename floating=double>
    inline bool is_negative(floating x, floating tolerance = std::numeric_limits<floating>::epsilon()) {
        PLAJA_ASSERT(tolerance > 0)
        return x < -tolerance;
    }

    // Marabou here references:
    // https://randomascii.wordpress.com/2012/02/25/comparing-floating-point-numbers-2012-edition/
    // http://docs.oracle.com/cd/E19957-01/806-3568/ncg_goldberg.html
    template<typename floating=double>
    inline bool equal(floating x, floating y, floating tolerance = std::numeric_limits<floating>::epsilon()) {
        PLAJA_ASSERT(tolerance > 0)
        floating diff = abs(x - y);
        if (diff <= tolerance) { return true; }

        x = abs(x);
        y = abs(y);
        floating largest = (x > y) ? x : y;

        if (diff <= largest * std::numeric_limits<floating>::epsilon()) { return true; }

        return false;
    }

    template<typename floating=double>
    inline bool non_equal(floating x, floating y, floating tolerance = std::numeric_limits<floating>::epsilon()) { return not equal(x, y, tolerance); }

    template<typename floating=double>
    inline bool gt(floating x, floating y, floating tolerance = std::numeric_limits<floating>::epsilon()) { return is_positive(x - y, tolerance); }

    template<typename floating=double>
    inline bool gte(floating x, floating y, floating tolerance = std::numeric_limits<floating>::epsilon()) { return not is_negative(x - y, tolerance); }

    template<typename floating=double>
    inline bool lt(floating x, floating y, floating tolerance = std::numeric_limits<floating>::epsilon()) { return gt(y, x, tolerance); }

    template<typename floating=double>
    inline bool lte(floating x, floating y, floating tolerance = std::numeric_limits<floating>::epsilon()) { return gte(y, x, tolerance); }

    template<typename floating=double>
    inline double min(floating x, floating y, floating tolerance = std::numeric_limits<floating>::epsilon()) { return lt(x, y, tolerance) ? x : y; }

    template<typename floating=double>
    inline double max(floating x, floating y, floating tolerance = std::numeric_limits<floating>::epsilon()) { return gt(x, y, tolerance) ? x : y; }

    template<typename floating=double, typename integer=int>
    inline bool equals_integer(floating x, integer y, floating tolerance = std::numeric_limits<floating>::epsilon()) { return PLAJA_FLOATS::equal(x, static_cast<floating>(y), tolerance); }

    template<typename floating=double, typename integer=int>
    inline bool is_integer(floating x, floating tolerance = std::numeric_limits<floating>::epsilon()) { return PLAJA_FLOATS::equal(x, std::round(x), tolerance); }

}

#endif //PLAJA_FLOATING_UTILS_H
