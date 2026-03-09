//
// This file is part of the PlaJA code base.
// Copyright (c) (2019 - 2023) Marcel Vinzent.
// See README.md in the top-level directory for licensing information.
//

#include "context_z3.h"
#include "constraint_z3.h"

namespace Z3_IN_PLAJA {

    const std::string intVarPrefix { "z3_int_" }; // NOLINT(cert-err58-cpp)
    const std::string boolVarPrefix { "z3_bool_" }; // NOLINT(cert-err58-cpp)
    const std::string realVarPrefix { "z3_real_" }; // NOLINT(cert-err58-cpp)
    const std::string intArrayVarPrefix { "z3_int_array_" }; // NOLINT(cert-err58-cpp)
    const std::string boolArrayVarPrefix { "z3_bool_array_" }; // NOLINT(cert-err58-cpp)
    const std::string realArrayVarPrefix { "z3_real_array_" }; // NOLINT(cert-err58-cpp)
    const std::string auxiliaryVarPrefix { "z3_aux_" }; // NOLINT(cert-err58-cpp)
    const std::string freeVarPrefix { "z3_free_" }; // NOLINT(cert-err58-cpp)

}

Z3_IN_PLAJA::Context::Context():
    PLAJA::SmtContext()
    , context()
    , IntSort(context.int_sort())
    , BoolSort(context.bool_sort())
    , RealSort(context.real_sort())
    , IntArraySort(context.array_sort(IntSort, IntSort))
    , BoolArraySort(context.array_sort(IntSort, context.bool_sort()))
    , RealArraySort(context.array_sort(RealSort, RealSort))
    , nextVar(0) {}

Z3_IN_PLAJA::Context::~Context() = default;

/* */

std::unique_ptr<Z3_IN_PLAJA::Constraint> Z3_IN_PLAJA::Context::trivially_true() { return std::make_unique<Z3_IN_PLAJA::ConstraintTrivial>(operator()(), true); }

std::unique_ptr<Z3_IN_PLAJA::Constraint> Z3_IN_PLAJA::Context::trivially_false() { return std::make_unique<Z3_IN_PLAJA::ConstraintTrivial>(operator()(), false); }