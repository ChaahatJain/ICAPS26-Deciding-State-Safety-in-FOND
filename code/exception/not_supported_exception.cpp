//
// This file is part of the PlaJA code base.
// Copyright (c) (2019 - 2019) Marcel Vinzent.
// See README.md in the top-level directory for licensing information.
//

#include "not_supported_exception.h"
#include "../include/compiler_config_const.h"
#include "../utils/utils.h"
#include "exception_strings.h"

const std::string notSupportedString("Not supported: "); // NOLINT(cert-err58-cpp)

NotSupportedException::NotSupportedException(const std::string& what_):
    PlajaException(notSupportedString + what_) {}

NotSupportedException::NotSupportedException(const std::string& input, const std::string& remark):
    PlajaException(notSupportedString + input + PLAJA_EXCEPTION::remarkString + remark) {}

NotSupportedException::NotSupportedException(const std::string& input, const std::string& context, const std::string& remark):
    PlajaException(notSupportedString + input + PLAJA_EXCEPTION::contextString + context + PLAJA_EXCEPTION::remarkString + remark) {
}

NotSupportedException::NotSupportedException(const std::string& function, const std::string& input, const std::string& context, const std::string& remark):
    PlajaException(notSupportedString + input + PLAJA_EXCEPTION::contextString + context + PLAJA_EXCEPTION::remarkString + remark + (PLAJA_GLOBAL::debug ? PLAJA_EXCEPTION::functionString + function : PLAJA_UTILS::emptyString)) {
}

NotSupportedException::~NotSupportedException() = default;
