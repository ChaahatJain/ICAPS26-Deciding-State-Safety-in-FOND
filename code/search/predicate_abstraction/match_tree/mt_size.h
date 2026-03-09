//
// This file is part of the PlaJA code base.
// Copyright (c) (2019 - 2023) Marcel Vinzent.
// See README.md in the top-level directory for licensing information.
//

#ifndef PLAJA_MT_SIZE_H
#define PLAJA_MT_SIZE_H

#include <algorithm>
#include "../../../utils/default_constructors.h"

namespace PA_MATCH_TREE {

    /** Some stats. */
    struct MtSize final {

        unsigned size;
        unsigned maxDepth;
        unsigned leafDepthSum;
        unsigned leafCount;

        inline MtSize():
            size(0), maxDepth(0), leafDepthSum(0), leafCount(0) {}
        inline ~MtSize() = default;
        DEFAULT_CONSTRUCTOR_ONLY(MtSize)
        DELETE_ASSIGNMENT(MtSize)

        inline void inc_size() { ++size; }

        inline void update_depth(unsigned depth) {
            maxDepth = std::max(maxDepth, depth);
            leafDepthSum += depth;
            ++leafCount;
        }

        [[nodiscard]] inline double get_av_depth() const { return static_cast<double>(leafDepthSum) / leafCount; }

    };

}

#endif //PLAJA_MT_SIZE_H
