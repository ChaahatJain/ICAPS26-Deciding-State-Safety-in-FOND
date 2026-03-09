//
// This file is part of the PlaJA code base.
// Copyright (c) (2019 - 2023) Marcel Vinzent.
// See README.md in the top-level directory for licensing information.
//

#ifndef NDEBUG

#include "assertion_exception.h"
#include "../utils/utils.h"

AssertionException::AssertionException(const std::string& function, const std::string& file, const std::string& line):
    PlajaException(PLAJA_UTILS::string_f("Assertion error: function %s in %s, line %s.", function.c_str(), file.c_str(), line.c_str())) {
}

AssertionException::~AssertionException() noexcept = default;

#endif