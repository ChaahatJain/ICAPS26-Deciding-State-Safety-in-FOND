//
// This file is part of the PlaJA code base.
// Copyright (c) (2019 - 2024) Marcel Vinzent.
// See README.md in the top-level directory for licensing information.
//

#ifndef PLAJA_SMT_CONSTRAINT_H
#define PLAJA_SMT_CONSTRAINT_H

#include "../../../utils/default_constructors.h"
#include "forward_smt.h"

namespace PLAJA {

    class SmtConstraint {
    protected:
        SmtConstraint();
        DEFAULT_CONSTRUCTOR_ONLY(SmtConstraint)
    public:
        virtual ~SmtConstraint() = 0;
        DELETE_ASSIGNMENT(SmtConstraint)

        [[nodiscard]] virtual bool is_trivially_true() const;
        [[nodiscard]] virtual bool is_trivially_false() const;

        virtual void add_to_solver(PLAJA::SmtSolver& solver) const = 0;
        virtual void add_negation_to_solver(PLAJA::SmtSolver& solver) const = 0;

    };

}

#endif //PLAJA_SMT_CONSTRAINT_H
