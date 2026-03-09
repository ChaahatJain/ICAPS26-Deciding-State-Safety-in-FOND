//
// This file is part of the PlaJA code base.
// Copyright (c) (2019 - 2024) Marcel Vinzent.
// See README.md in the top-level directory for licensing information.
//

#ifndef PLAJA_SMT_CONTEXT_H
#define PLAJA_SMT_CONTEXT_H

#include "../../../utils/default_constructors.h"
#include "forward_smt.h"

namespace PLAJA {

    class SmtContext {
    protected:
        SmtContext();
    public:
        virtual ~SmtContext() = 0;
        DELETE_CONSTRUCTOR(SmtContext)
    };

}
#endif //PLAJA_SMT_CONTEXT_H
