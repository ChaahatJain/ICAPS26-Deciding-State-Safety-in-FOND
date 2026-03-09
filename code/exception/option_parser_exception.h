//
// This file is part of the PlaJA code base.
// Copyright (c) (2019 - 2019) Marcel Vinzent.
// See README.md in the top-level directory for licensing information.
//

#ifndef PLAJA_OPTION_PARSER_EXCEPTION_H
#define PLAJA_OPTION_PARSER_EXCEPTION_H

#include "plaja_exception.h"

class OptionParserException: public PlajaException {
public:
    explicit OptionParserException(const std::string& what_);
    explicit OptionParserException(const std::string& argument, const std::string& remark);
    ~OptionParserException() override;
};

#endif //PLAJA_OPTION_PARSER_EXCEPTION_H
