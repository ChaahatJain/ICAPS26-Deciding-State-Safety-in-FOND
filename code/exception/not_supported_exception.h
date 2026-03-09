//
// This file is part of the PlaJA code base.
// Copyright (c) (2019 - 2019) Marcel Vinzent.
// See README.md in the top-level directory for licensing information.
//

#ifndef PLAJA_NOT_SUPPORTED_EXCEPTION_H
#define PLAJA_NOT_SUPPORTED_EXCEPTION_H

#include "plaja_exception.h"

/**
 * Exception intended to be thrown during checks if model contains not (completely) supported structures of the standard fragment or (partly) supported extensions.
 */
class NotSupportedException: public PlajaException {
public:
    explicit NotSupportedException(const std::string& what_);
    NotSupportedException(const std::string& input, const std::string& remark);
    NotSupportedException(const std::string& input, const std::string& context, const std::string& remark);
    NotSupportedException(const std::string& function, const std::string& input, const std::string& context, const std::string& remark);
    ~NotSupportedException() override;
    DELETE_CONSTRUCTOR(NotSupportedException)
};

#endif //PLAJA_NOT_SUPPORTED_EXCEPTION_H
