//
// This file is part of the PlaJA code base.
// Copyright (c) (2019 - 2019) Marcel Vinzent.
// See README.md in the top-level directory for licensing information.
//

#include "runtime_exception.h"

namespace PLAJA_EXCEPTION {
    const std::string probabilityOutOfBounds(" ,probability out of bounds"); // NOLINT(cert-err58-cpp)
    const std::string probabilitySumIs("probability sum is "); // NOLINT(cert-err58-cpp)
}

RuntimeException::RuntimeException(const std::string& what_): PlajaException("Runtime error: " + what_) {}

RuntimeException::~RuntimeException() = default;
