//
// This file is part of the PlaJA code base.
// Copyright (c) (2019 - 2023) Marcel Vinzent.
// See README.md in the top-level directory for licensing information.
//

#include "../utils/utils.h"
#include "smt_exception.h"


namespace SMT_EXCEPTION {

    std::string solver_to_str(SMTException::Solver solver) {
        switch (solver) {
            case SMTException::Z3: { return "z3"; }
            case SMTException::MARABOU: { return "Marabou"; }
            default: PLAJA_ABORT
        }
        PLAJA_ABORT
    }

    std::string reason_to_str(SMTException::Reason reason) {
        switch (reason) {
            case SMTException::Unknown: { return "undecided query"; }
            default: PLAJA_ABORT
        }
        PLAJA_ABORT
    }

}


SMTException::SMTException(Solver solver, Reason reason, const std::string& query_file): RuntimeException(PLAJA_UTILS::string_f("SMT exception due to %s using %s. Query saved as %s", SMT_EXCEPTION::reason_to_str(reason).c_str(), SMT_EXCEPTION::solver_to_str(solver).c_str(), query_file.c_str())) {}

SMTException::~SMTException() = default;
