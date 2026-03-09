//
// This file is part of the PlaJA code base.
// Copyright (c) (2019 - 2019) Marcel Vinzent.
// See README.md in the top-level directory for licensing information.
//

#ifndef PLAJA_SEMANTICS_EXCEPTION_H
#define PLAJA_SEMANTICS_EXCEPTION_H

#include "plaja_exception.h"

class SemanticsException: public PlajaException {
public:
    explicit SemanticsException(const std::string& what_);
    ~SemanticsException() override;
};

#endif //PLAJA_SEMANTICS_EXCEPTION_H
