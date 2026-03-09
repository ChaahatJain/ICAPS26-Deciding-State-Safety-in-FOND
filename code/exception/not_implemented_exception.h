//
// This file is part of the PlaJA code base.
// Copyright (c) (2019 - 2019) Marcel Vinzent.
// See README.md in the top-level directory for licensing information.
//

#ifndef PLAJA_NOT_IMPLEMENTED_EXCEPTION_H
#define PLAJA_NOT_IMPLEMENTED_EXCEPTION_H

#include "plaja_exception.h"

/**
 * Development exception:
 * thrown if function is not implemented for, in particular for virtual functions.
 */
class NotImplementedException: public PlajaException {
public:
    explicit NotImplementedException(const std::string& function);
};

#endif //PLAJA_NOT_IMPLEMENTED_EXCEPTION_H
