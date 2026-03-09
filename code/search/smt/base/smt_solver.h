//
// This file is part of the PlaJA code base.
// Copyright (c) (2019 - 2023) Marcel Vinzent.
// See README.md in the top-level directory for licensing information.
//

#ifndef PLAJA_SMT_SOLVER_H
#define PLAJA_SMT_SOLVER_H

#include <memory>
#include "../../../utils/default_constructors.h"
#include "../../../stats/forward_stats.h"
#include "../../../assertions.h"
#include "forward_smt.h"

namespace PLAJA {

    /** Generic solver interface. */
    class SmtSolver {

    public:
        enum UnknownHandling { True, False, Error }; // NOLINT(*-enum-size)
        enum SmtResult { Unknown, UnSat, Sat, Entailed }; // NOLINT(*-enum-size)


    protected:
        UnknownHandling unknownHandling; // How to handle undecided queries.
        bool unknownRlt;

        PLAJA::StatsBase* sharedStatistics; // Stats.

    public:
        explicit SmtSolver(PLAJA::StatsBase* stats);
        virtual ~SmtSolver() = 0;
        DELETE_CONSTRUCTOR(SmtSolver)

        inline void set(UnknownHandling unknown_handling) { unknownHandling = unknown_handling; }

        [[nodiscard]] inline static bool is_unknown(SmtResult rlt) { return rlt == Unknown; }

        [[nodiscard]] inline static bool cast_to_bool(SmtResult rlt) {
            PLAJA_ASSERT(rlt != Unknown)
            switch (rlt) {
                case Unknown: { PLAJA_ABORT }
                case UnSat: { return false; }
                case Sat:
                case Entailed: { return true; }
            }
        }

        [[nodiscard]] inline static SmtResult cast_to_rlt(bool rlt) { return rlt ? SmtResult::Sat : SmtResult::UnSat; }

        void add_smt_constraint(const PLAJA::SmtConstraint& constraint);

        virtual void push() = 0;

        virtual void pop() = 0;

        virtual void pop_repeated(unsigned int n);

        virtual void clear() = 0;

        /**
         * Fast check that may or may not derived sat.
         * Quick fix for large initialization overhead in Marabou solver.
         */
        virtual SmtResult pre_check();

        virtual bool check() = 0;

        /* Extended interface. */

        inline SmtResult pre_and_check() {
            /* Pre-Check. */
            const auto rlt = pre_check();
            if (not is_unknown(rlt)) {
                PLAJA_ASSERT(rlt != SmtResult::Entailed)
                return rlt;
            }
            /* Check. */
            return cast_to_rlt(check());
        }

        [[nodiscard]] inline SmtResult check(const PLAJA::SmtConstraint& constraint, bool do_pre_check) {
            push();
            add_smt_constraint(constraint);
            const auto rlt = do_pre_check ? pre_and_check() : cast_to_rlt(check());
            pop();
            return rlt;
        }

        SmtResult check_entailment(const PLAJA::SmtConstraint& constraint, bool do_pre_check);

    };

}

#endif //PLAJA_SMT_SOLVER_H
