//
// This file is part of the PlaJA code base.
// Copyright (c) (2019 - 2020) Marcel Vinzent.
// See README.md in the top-level directory for licensing information.
//

#include "constructor_exception.h"
#include "exception_strings.h"

namespace PLAJA_EXCEPTION {
    const std::string nonLinear{"non-linear"}; // NOLINT(cert-err58-cpp)
    const std::string constructionErrorString("Construction error: "); // NOLINT(cert-err58-cpp)
}

ConstructorException::ConstructorException(const std::string& what_): PlajaException(PLAJA_EXCEPTION::constructionErrorString + what_) {}

ConstructorException::ConstructorException(const std::string& context, const std::string& remark): PlajaException(PLAJA_EXCEPTION::constructionErrorString + context + PLAJA_EXCEPTION::remarkString + remark) {}

ConstructorException::~ConstructorException() = default;