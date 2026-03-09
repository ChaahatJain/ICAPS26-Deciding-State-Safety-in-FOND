//
// This file is part of the PlaJA code base.
// Copyright (c) (2019 - 2019) Marcel Vinzent.
// See README.md in the top-level directory for licensing information.
//

#include "not_implemented_exception.h"

NotImplementedException::NotImplementedException(const std::string& function): PlajaException("Not implemented: " + function) {}
