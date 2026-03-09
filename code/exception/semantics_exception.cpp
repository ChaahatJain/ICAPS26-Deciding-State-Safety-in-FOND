//
// This file is part of the PlaJA code base.
// Copyright (c) (2019 - 2019) Marcel Vinzent.
// See README.md in the top-level directory for licensing information.
//

#include "semantics_exception.h"

SemanticsException::SemanticsException(const std::string& what_): PlajaException("Semantic error: " + what_) {}

SemanticsException::~SemanticsException() = default;
