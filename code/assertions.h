//
// This file is part of the PlaJA code base.
// Copyright (c) (2019 - 2019) Marcel Vinzent.
// See README.md in the top-level directory for licensing information.
//

#ifndef PLAJA_ASSERTIONS_H
#define PLAJA_ASSERTIONS_H

#include <cassert>
#include <cstdlib>
#include <limits>
#include "utils/default_constructors.h"

//#define NDEBUG // no debugging assertions, set via cmake for release mode

// General assertion, e.g., for invariants, preconditions ...
#define MAYBE_UNUSED(var) static_cast<void>(var);
#define PLAJA_ASSERT(expr) assert(expr);
#define PLAJA_ASSERT_EXPECT(expr) assert(expr); /** Intended for assertions set due to intended behavior (e.g., implicit invariance) not due to hard assumptions of the asserted code fragment. */
#define PLAJA_STATIC_ASSERT(expr) static_assert(expr);
#define PLAJA_ABORT_IF(condition) if (condition) { abort(); }
#define PLAJA_ABORT_IF_CONST(condition) if constexpr(condition) { abort(); }

// Abort, e.g., to prevent non-void function compilation Werror in release mode.
#ifdef NDEBUG
// Some syntactic sugar for debugging dependent behavior:
#define FIELD_IF_DEBUG(FIELD)
#define FCT_IF_DEBUG(FCT)
#define STMT_IF_DEBUG(STMT)
#define CONSTRUCT_IF_DEBUG(CONSTRUCT)
#else
// Some syntactic sugar for debugging dependent behavior:
#define FIELD_IF_DEBUG(FIELD) FIELD
#define FCT_IF_DEBUG(FCT) FCT
#define STMT_IF_DEBUG(STMT) STMT
#define CONSTRUCT_IF_DEBUG(CONSTRUCT) ,CONSTRUCT
#endif
//
#define PLAJA_ABORT PLAJA_ABORT_IF_CONST(true)

// Legacy general assertion, now assertion to emphasise assumption about the JANI input.
#define JANI_ASSERT(expr) assert(expr);

//#define RUNTIME_CHECKS // runtime assertions, set via cmake for debug mode

// Runtime assertion to catch modelling error, that cannot be detected at parsing/checking etc.
// Disable assertion if the models are "known" to be correct, as such checks may be costly, but are not needed to compute the target information.
#ifdef RUNTIME_CHECKS
#define RUNTIME_ASSERT(expr, info) if(!(expr)) { throw RuntimeException(info); }
// Some syntactic sugar for runtime dependent behavior:
#define FIELD_IF_RUNTIME_CHECKS(FIELD) FIELD
#define FCT_IF_RUNTIME_CHECKS(FCT) FCT
#define PARAM_IF_RUNTIME_CHECKS(PARAM) PARAM
#define STMT_IF_RUNTIME_CHECKS(STMT) STMT;
#define CONSTRUCT_IF_RUNTIME_CHECKS(CONSTRUCT) ,CONSTRUCT
#else
#define RUNTIME_ASSERT(expr, info);
#define FIELD_IF_RUNTIME_CHECKS(FIELD)
#define FCT_IF_RUNTIME_CHECKS(FCT)
#define PARAM_IF_RUNTIME_CHECKS(PARAM)
#define STMT_IF_RUNTIME_CHECKS(STMT)
#define CONSTRUCT_IF_RUNTIME_CHECKS(CONSTRUCT)
#endif

#endif //PLAJA_ASSERTIONS_H
