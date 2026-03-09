//
// This file is part of the PlaJA code base.
// Copyright (c) (2019 - 2023) Marcel Vinzent.
// See README.md in the top-level directory for licensing information.
//

#include "smt_solver.h"
#include "smt_constraint.h"

PLAJA::SmtSolver::SmtSolver(PLAJA::StatsBase* stats):
    unknownHandling(UnknownHandling::Error)
    , unknownRlt(false)
    , sharedStatistics(stats) {
}

PLAJA::SmtSolver::~SmtSolver() = default;

/* */

void PLAJA::SmtSolver::add_smt_constraint(const PLAJA::SmtConstraint& constraint) { constraint.add_to_solver(*this); }

void PLAJA::SmtSolver::pop_repeated(unsigned int n) {
    while (n > 0) {
        pop();
        --n;
    }
}

/* */

PLAJA::SmtSolver::SmtResult PLAJA::SmtSolver::pre_check() { return SmtResult::Unknown; }

PLAJA::SmtSolver::SmtResult PLAJA::SmtSolver::check_entailment(const PLAJA::SmtConstraint& constraint, bool do_pre_check) {

    /* Entail false?*/
    push();
    constraint.add_to_solver(*this);
    const SmtResult rlt = do_pre_check ? pre_and_check() : cast_to_rlt(check());
    pop();
    if (rlt == SmtResult::UnSat or rlt == SmtResult::Entailed) { return rlt; }

    /* Entail true? */
    push();
    constraint.add_negation_to_solver(*this);
    const SmtResult rlt_neg = do_pre_check ? pre_and_check() : cast_to_rlt(check());
    pop();
    switch (rlt_neg) {
        case Unknown:
        case Sat: { return rlt_neg; } // Sat has been found above.
        case UnSat: { return SmtResult::Entailed; }
        case Entailed: { return SmtResult::UnSat; }
    }

    PLAJA_ABORT
}
