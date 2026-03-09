//
// This file is part of the PlaJA code base.
// Copyright (c) (2019 - 2019) Marcel Vinzent.
// See README.md in the top-level directory for licensing information.
//

#ifndef PLAJA_PARSER_EXCEPTION_H
#define PLAJA_PARSER_EXCEPTION_H

#include "plaja_exception.h"

class ParserException: public PlajaException {
public:
    explicit ParserException(const std::string& what_);
    ParserException(const std::string& input, const std::string& context, const std::string& remark);
    ParserException(const std::string& function, const std::string& input, const std::string& context, const std::string& remark);
    ~ParserException() override;
    DELETE_CONSTRUCTOR(ParserException)
};

#endif //PLAJA_PARSER_EXCEPTION_H
