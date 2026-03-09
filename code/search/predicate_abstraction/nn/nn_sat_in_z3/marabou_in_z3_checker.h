//
// This file is part of the PlaJA code base.
// Copyright (c) (2019 - 2021) Marcel Vinzent.
// See README.md in the top-level directory for licensing information.
//

#ifndef PLAJA_MARABOU_IN_Z3_CHECKER_H
#define PLAJA_MARABOU_IN_Z3_CHECKER_H

#include <memory>
#include <vector>
#include <unordered_map>
#include "../../../smt_nn/using_marabou.h"
#include "../../../smt/using_smt.h"
#include "nn_sat_in_z3_base.h"


// forward declaration:
class Equation;
class DisjunctionConstraint;
class ReluConstraint;
namespace MARABOU_IN_PLAJA { class MarabouQuery; }
namespace Z3_IN_PLAJA { class MarabouToZ3Vars; }


/**
 * Check an Marabou input query via Z3.
 */
class MarabouInZ3Checker final: public NNSatInZ3Base {

private:
    // generate variables:
    std::unique_ptr<Z3_IN_PLAJA::MarabouToZ3Vars> variables; // newly generated per query
    void generate_and_add_variables(const MARABOU_IN_PLAJA::MarabouQuery& query, Z3_IN_PLAJA::SMTSolver& solver);

    // generate structures:
    void add_relu(Z3_IN_PLAJA::SMTSolver& solver, const ReluConstraint& relu_constraint, const MARABOU_IN_PLAJA::MarabouQuery& query);

    bool check_marabou(const MARABOU_IN_PLAJA::MarabouQuery& query);
    bool check_marabou();
    bool check_() override;

public:
    explicit MarabouInZ3Checker(const Jani2NNet& jani_2_nnet, const PLAJA::Configuration& config);
    ~MarabouInZ3Checker() final;
    DELETE_CONSTRUCTOR(MarabouInZ3Checker)

};


#endif //PLAJA_MARABOU_IN_Z3_CHECKER_H
