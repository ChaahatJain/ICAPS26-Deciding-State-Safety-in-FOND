//
// This file is part of the PlaJA code base.
// Copyright (c) (2019 - 2019) Marcel Vinzent.
// See README.md in the top-level directory for licensing information.
//

#include "plaja_exception.h"

// strings
namespace PLAJA_EXCEPTION {
    const std::string fileNotFoundString("File not found: "); // NOLINT(cert-err58-cpp)
}

PlajaException::PlajaException(std::string information): information(std::move(information)) {}

PlajaException::~PlajaException() noexcept = default;

// deleted to make sure exception are caught by reference
// PlajaException::PlajaException(const PlajaException& plaja_exception) noexcept {
//     information = plaja_exception.information;
// }

const char* PlajaException::what() const noexcept { return information.c_str(); }
