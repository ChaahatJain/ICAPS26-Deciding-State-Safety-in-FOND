//
// This file is part of the PlaJA code base.
// Copyright (c) (2019 - 2023) Marcel Vinzent.
// See README.md in the top-level directory for licensing information.
//

#ifndef PLAJA_BMC_Z3_H
#define PLAJA_BMC_Z3_H

#include "bmc.h"

class ModelZ3;
namespace Z3_IN_PLAJA { class SMTSolver; }

class BMC_Z3 final: public BoundedMC {

private:
    std::unique_ptr<ModelZ3> modelZ3;

    [[nodiscard]] bool check_start_smt() const override;
    void prepare_solver(Z3_IN_PLAJA::SMTSolver& solver, unsigned int steps) const;
    [[nodiscard]] bool check_loop_free_smt(unsigned int steps) const override;
    [[nodiscard]] bool check_unsafe_smt(unsigned int steps) const override;

public:
    explicit BMC_Z3(const PLAJA::Configuration& config);
    ~BMC_Z3() override;
    DELETE_CONSTRUCTOR(BMC_Z3)

};


#endif //PLAJA_BMC_Z3_H
