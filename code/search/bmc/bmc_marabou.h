//
// This file is part of the PlaJA code base.
// Copyright (c) (2019 - 2023) Marcel Vinzent.
// See README.md in the top-level directory for licensing information.
//

#ifndef PLAJA_BMC_MARABOU_H
#define PLAJA_BMC_MARABOU_H

#include "bmc.h"

class ModelMarabou;
namespace MARABOU_IN_PLAJA { class SMTSolver; }

class BMCMarabou final: public BoundedMC {

private:
    std::unique_ptr<ModelMarabou> modelMarabou;
    std::unique_ptr<MARABOU_IN_PLAJA::SMTSolver> solverMarabou;

    [[nodiscard]] bool check_start_smt() const override;
    void prepare_solver(MARABOU_IN_PLAJA::SMTSolver& solver, unsigned int steps_) const;
    [[nodiscard]] bool check_loop_free_smt(unsigned int steps_) const override;
    [[nodiscard]] bool check_unsafe_smt(unsigned int steps_) const override;


public:
    explicit BMCMarabou(const PLAJA::Configuration& config);
    ~BMCMarabou() override;
    DELETE_CONSTRUCTOR(BMCMarabou)

};


#endif //PLAJA_BMC_MARABOU_H
