//
// This file is part of the PlaJA code base.
// Copyright (c) (2019 - 2023) Marcel Vinzent.
// See README.md in the top-level directory for licensing information.
//

#ifndef PLAJA_SMT_BASED_RESTRICTIONS_CHECK_H
#define PLAJA_SMT_BASED_RESTRICTIONS_CHECK_H

#include "../../../factories/forward_factories.h"

class AstElement;

namespace SMT_BASED_RESTRICTIONS_CHECKER {

    extern void check(const AstElement& ast_element, const PLAJA::Configuration& config);

}

#endif //PLAJA_SMT_BASED_RESTRICTIONS_CHECK_H
