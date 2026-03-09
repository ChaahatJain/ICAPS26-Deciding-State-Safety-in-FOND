//
// This file is part of the PlaJA code base.
// Copyright (c) (2019 - 2021) Marcel Vinzent.
// See README.md in the top-level directory for licensing information.
//

#ifndef PLAJA_NN_SAT_CHECKER_Z3_H
#define PLAJA_NN_SAT_CHECKER_Z3_H

#include <memory>
#include <vector>
#include "../../../information/jani2nnet/using_jani2nnet.h"
#include "../../../using_search.h"
#include "../../using_predicate_abstraction.h"
#include "nn_sat_in_z3_base.h"

// forward declaration:
struct NNVarsInZ3;
class ReluConstraint;
namespace Z3_IN_PLAJA { class SMTSolver; class MarabouToZ3Vars; }


class NNSatCheckerZ3 final: public NNSatInZ3Base {

private:
    std::vector<const ReluConstraint*> reluConstraints;
    std::unique_ptr<Z3_IN_PLAJA::MarabouToZ3Vars> variables;
    std::vector<z3::expr> outputInterfacePerIndex;
    std::unique_ptr<Z3_IN_PLAJA::SMTSolver> nnSatSolver;

    void add_relu_constraints_z3(const MARABOU_IN_PLAJA::PreprocessedBounds* preprocessed_bounds);

    void add_source_state_z3();
    void add_guard_z3();
    void add_guard_negation_z3();
    void add_update_z3();
    void add_target_predicate_state_z3();
    void add_output_interface_z3();


    bool check_z3(const MARABOU_IN_PLAJA::PreprocessedBounds* preprocessed_bounds);
    bool check_() override;

public:
    NNSatCheckerZ3(const Jani2NNet& jani_2_nnet, const PLAJA::Configuration& config);
    ~NNSatCheckerZ3() final;
    DELETE_CONSTRUCTOR(NNSatCheckerZ3)

};


#endif //PLAJA_NN_SAT_CHECKER_Z3_H
