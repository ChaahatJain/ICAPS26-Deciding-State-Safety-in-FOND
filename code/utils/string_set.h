//
// This file is part of the PlaJA code base.
// Copyright (c) (2019 - 2023) Marcel Vinzent.
// See README.md in the top-level directory for licensing information.
//

#ifndef PLAJA_STRING_SET_H
#define PLAJA_STRING_SET_H

#include <unordered_map>
#include <unordered_set>
#include "utils.h"

namespace PLAJA {

    struct StringHash {
        std::size_t operator()(const std::string* str) const {
            PLAJA_ASSERT(str)
            return PLAJA_UTILS::hashString(*str);
        }
    };

    struct StringEqual {
        bool operator()(const std::string* str1, const std::string* str2) const {
            PLAJA_ASSERT(str1)
            PLAJA_ASSERT(str2)
            return *str1 == *str2;
        }
    };

    using StringSet = std::unordered_set<const std::string*, StringHash, StringEqual>;

    template<typename t> using StringMap = std::unordered_map<const std::string*, t, StringHash, StringEqual>;

}

#endif //PLAJA_STRING_SET_H
