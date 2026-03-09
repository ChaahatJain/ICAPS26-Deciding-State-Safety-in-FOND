//
// This file is part of the PlaJA code base.
// Copyright (c) (2019 - 2020) Marcel Vinzent.
// See README.md in the top-level directory for licensing information.
//

#ifndef PLAJA_CONSTRUCTOR_EXCEPTION_H
#define PLAJA_CONSTRUCTOR_EXCEPTION_H

#include "plaja_exception.h"

namespace PLAJA_EXCEPTION { extern const std::string nonLinear; }

class ConstructorException: public PlajaException {
public:
    explicit ConstructorException(const std::string& what_);
    ConstructorException(const std::string& context, const std::string& remark);
    ~ConstructorException() override;
};

#endif //PLAJA_CONSTRUCTOR_EXCEPTION_H
