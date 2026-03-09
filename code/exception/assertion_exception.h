//
// This file is part of the PlaJA code base.
// Copyright (c) (2019 - 2023) Marcel Vinzent.
// See README.md in the top-level directory for licensing information.
//

#ifndef PLAJA_ASSERTION_EXCEPTION_H
#define PLAJA_ASSERTION_EXCEPTION_H

#ifndef NDEBUG

#include "plaja_exception.h"

/**
 * Debugging exception in case we want to "catch" the assert higher up in the call stack,
 * e.g., to degenerate additional output information.
 */
class AssertionException final: public PlajaException {

public:
    explicit AssertionException(const std::string& function, const std::string& file, const std::string& line);
    ~AssertionException() noexcept override;
    DEFAULT_CONSTRUCTOR_ONLY(AssertionException)
    DELETE_ASSIGNMENT(AssertionException)

};

#define PLAJA_ASSERT_EXCEPTION(expr) { if (not (expr)) { throw AssertionException(static_cast<const char*>(__FUNCTION__), static_cast<const char*>(__FILE__), std::to_string(__LINE__)); } }

#endif

#endif //PLAJA_ASSERTION_EXCEPTION_H
