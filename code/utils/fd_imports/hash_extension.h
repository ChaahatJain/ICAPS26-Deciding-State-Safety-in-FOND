//
// This file is part of the PlaJA code base.
// Copyright (c) (2019 - 2022) Marcel Vinzent.
// See README.md in the top-level directory for licensing information.
//

#ifndef PLAJA_HASH_EXTENSION_H
#define PLAJA_HASH_EXTENSION_H

#include "../../include/compiler_config_const.h"
#include "hash.h"

/**
 * This file extents the template specialization for the "feed" function in Fast Downward file hash.h.
 */

namespace utils {

#ifdef IS_OSX
    inline void feed(HashState& hash_state, long unsigned int value) {
        static_assert(sizeof(std::size_t) == sizeof(std::uint64_t));
        hash_state.feed(static_cast<std::uint64_t>(value));
    }
#endif

    inline void feed(HashState& hash_state, bool value) { hash_state.feed(value); }

    inline void feed(HashState& hash_state, float value) {
        static_assert(sizeof(float) == sizeof(std::uint32_t));
        static std::hash<float> std_hash;
        hash_state.feed(std_hash.operator()(value));
    }

    inline void feed(HashState& hash_state, double value) {
        static_assert(sizeof(double) == sizeof(std::uint64_t));
        static std::hash<double> std_hash;
        hash_state.feed(std_hash.operator()(value));
    }

}

#endif //PLAJA_HASH_EXTENSION_H
