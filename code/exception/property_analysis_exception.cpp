//
// This file is part of the PlaJA code base.
// Copyright (c) (2019 - 2020) Marcel Vinzent.
// See README.md in the top-level directory for licensing information.
//

#include "property_analysis_exception.h"
#include "exception_strings.h"

PropertyAnalysisException::PropertyAnalysisException(const std::string& property, const std::string& remark): PlajaException("Property: " + property + " is not supported" + PLAJA_EXCEPTION::remarkString + remark) {}

PropertyAnalysisException::PropertyAnalysisException(const std::string& property, const std::string& engine, const std::string& remark): PlajaException("Property: " + property + " is not supported by engine: " + engine + PLAJA_EXCEPTION::remarkString + remark) {}

PropertyAnalysisException::~PropertyAnalysisException() = default;
