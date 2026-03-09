//
// This file is part of the PlaJA code base.
// Copyright (c) (2019 - 2019) Marcel Vinzent.
// See README.md in the top-level directory for licensing information.
//

#ifndef PLAJA_PLAJA_EXCEPTION_H
#define PLAJA_PLAJA_EXCEPTION_H

#include <exception>
#include <string>
#include "../utils/default_constructors.h"

namespace PLAJA_EXCEPTION { extern const std::string fileNotFoundString; }

class PlajaException: public std::exception {
private:
    std::string information;
public:
    explicit PlajaException(std::string information = "Planning in JANI exception.");
    ~PlajaException() noexcept override;
    DEFAULT_CONSTRUCTOR_ONLY(PlajaException)
    DELETE_ASSIGNMENT(PlajaException)

    [[nodiscard]] const char* what() const noexcept override;
};

#endif //PLAJA_PLAJA_EXCEPTION_H
