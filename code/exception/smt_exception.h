//
// This file is part of the PlaJA code base.
// Copyright (c) (2019 - 2023) Marcel Vinzent.
// See README.md in the top-level directory for licensing information.
//

#ifndef PLAJA_SMT_EXCEPTION_H
#define PLAJA_SMT_EXCEPTION_H

#include "runtime_exception.h"

class SMTException: public RuntimeException {

public:
    enum Solver { Z3 , MARABOU };
    enum Reason { Unknown };

    SMTException(Solver solver, Reason reason, const std::string& query_file);
    ~SMTException() override;

};

#endif //PLAJA_SMT_EXCEPTION_H
