//
// This file is part of the PlaJA code base.
// Copyright (c) (2019 - 2019) Marcel Vinzent.
// See README.md in the top-level directory for licensing information.
//

#include "option_parser_exception.h"
#include "exception_strings.h"

const std::string optionParserErrorString("Option error: "); // NOLINT(cert-err58-cpp)

OptionParserException::OptionParserException(const std::string& what_): PlajaException(optionParserErrorString + what_) {}

OptionParserException::OptionParserException(const std::string& argument, const std::string& remark): PlajaException(optionParserErrorString + argument + PLAJA_EXCEPTION::remarkString + remark) {}

OptionParserException::~OptionParserException() = default;
