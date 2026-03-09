//
// This file is part of the PlaJA code base.
// Copyright (c) (2019 - 2019) Marcel Vinzent.
// See README.md in the top-level directory for licensing information.
//

#ifndef PLAJA_RUNTIME_EXCEPTION_H
#define PLAJA_RUNTIME_EXCEPTION_H

#include "plaja_exception.h"

namespace PLAJA_EXCEPTION {
    extern const std::string probabilityOutOfBounds;
    extern const std::string probabilitySumIs;
}

class RuntimeException: public PlajaException {
public:
    explicit RuntimeException(const std::string& what_);
    ~RuntimeException() override;
};

#endif //PLAJA_RUNTIME_EXCEPTION_H
