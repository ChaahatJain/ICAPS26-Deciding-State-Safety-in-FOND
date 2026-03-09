//
// This file is part of the PlaJA code base.
// Copyright (c) (2023 - 2024) Chaahat Jain.
//

#ifndef PLAJA_ENSEMBLE_SAT_IN_Z3_BASE_H
#define PLAJA_ENSEMBLE_SAT_IN_Z3_BASE_H

#include <memory>
#include "../../../smt/utils/forward_z3.h"
#include "../../smt/forward_smt_pa.h"
#include "../ensemble_sat_checker/ensemble_sat_checker.h"

namespace VERITAS_TO_Z3 { struct EncodingInformation; }

class EnsembleSatInZ3Base: public EnsembleSatChecker {

protected:
    const ModelZ3PA& modelZ3;
    std::unique_ptr<VERITAS_TO_Z3::EncodingInformation> encodingInformation;
    int check_relaxed_for_exact();

public:
    EnsembleSatInZ3Base(const Jani2Ensemble& jani_2_ensemble, const PLAJA::Configuration& config);
    ~EnsembleSatInZ3Base() override = 0;
    DELETE_CONSTRUCTOR(EnsembleSatInZ3Base)
};

#endif //PLAJA_ENSEMBLE_SAT_IN_Z3_BASE_H
