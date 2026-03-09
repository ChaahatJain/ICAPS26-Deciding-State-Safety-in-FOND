//
// This file is part of the PlaJA code base.
// Copyright (c) (2019 - 2021) Marcel Vinzent.
// See README.md in the top-level directory for licensing information.
//

#ifndef PLAJA_NN_SAT_IN_Z3_BASE_H
#define PLAJA_NN_SAT_IN_Z3_BASE_H

#include <memory>
#include "../../../smt/utils/forward_z3.h"
#include "../../smt/forward_smt_pa.h"
#include "../nn_sat_checker/nn_sat_checker.h"

namespace MARABOU_TO_Z3 { struct EncodingInformation; }

namespace MARABOU_IN_PLAJA { struct PreprocessedBounds; }

class NNSatInZ3Base: public NNSatChecker {

protected:
    const ModelZ3PA& modelZ3;
    std::unique_ptr<MARABOU_TO_Z3::EncodingInformation> encodingInformation;
    bool relaxedForExact;
    bool preprocessQuery;

    int check_relaxed_for_exact(std::unique_ptr<MARABOU_IN_PLAJA::PreprocessedBounds>* preprocessed_bounds);

public:
    NNSatInZ3Base(const Jani2NNet& jani_2_nnet, const PLAJA::Configuration& config);
    ~NNSatInZ3Base() override = 0;
    DELETE_CONSTRUCTOR(NNSatInZ3Base)
};

#endif //PLAJA_NN_SAT_IN_Z3_BASE_H
