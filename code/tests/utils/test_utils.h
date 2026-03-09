//
// This file is part of the PlaJA code base.
// Copyright (c) (2019 - 2020) Marcel Vinzent.
// See README.md in the top-level directory for licensing information.
//

#ifndef PLAJA_TEST_UTILS_H
#define PLAJA_TEST_UTILS_H

#include <memory>

namespace PLAJA_TEST_UTILS {

    template<typename Type_t>
    std::shared_ptr<Type_t> share(std::shared_ptr<Type_t> ptr_) { return ptr_; }

}

#endif //PLAJA_TEST_UTILS_H
