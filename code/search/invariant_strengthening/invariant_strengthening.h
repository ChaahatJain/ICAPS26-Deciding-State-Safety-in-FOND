//
// This file is part of the PlaJA code base.
// Copyright (c) (2019 - 2021) Marcel Vinzent.
// See README.md in the top-level directory for licensing information.
//

#ifndef PLAJA_INVARIANT_STRENGTHENING_H
#define PLAJA_INVARIANT_STRENGTHENING_H

#include <memory>
#include <vector>
#include <stack>
#include "veritas_box_helpers.h"
#include "../../parser/ast/expression/forward_expression.h"
#include "../fd_adaptions/search_engine.h"
#include "../smt/using_smt.h"
#include "../smt/forward_smt_z3.h"
#include "../smt_nn/forward_smt_nn.h"
#include "../states/forward_states.h"
#include "interval.hpp"
class SuccessorGeneratorAbstract;

class InvariantStrengthening final: public SearchEngine {

private:
    std::unique_ptr<Expression> invariant; // i.e. of the form "!unsafety constraint AND  ..."
    std::unique_ptr<Expression> nonInvariant; // i.e. of the form "unsafety constraint OR ..."
    std::vector<std::vector<veritas::Interval>> debug_solutions;
    std::vector<std::vector<veritas::Interval>> step_solutions;
    //
    std::unique_ptr<ModelZ3> modelZ3;
    std::unique_ptr<Z3_IN_PLAJA::SMTSolver> solverZ3;
    // NN
    std::unique_ptr<ModelMarabou> modelMarabou;
    std::unique_ptr<MARABOU_IN_PLAJA::SMTSolver> solverMarabou;
    // ENSEMBLE
    std::unique_ptr<ModelVeritas> modelVeritas;
    std::unique_ptr<VERITAS_IN_PLAJA::Solver> solver;

    long int count = 0;

    SearchStatus initialize() override;
    SearchStatus finalize() override;
    SearchStatus step() override;
    SearchStatus step_nn();
    SearchStatus step_ensemble();

    void restrict(const StateValues& state);
    void restrict(const std::vector<veritas::Interval>& box);
    void restrict(const std::vector<std::vector<veritas::Interval>>& boxes);


    /* Override stats. */
    void print_statistics() const override;
    void stats_to_csv(std::ofstream& file) const override;
    void stat_names_to_csv(std::ofstream& file) const override;

public:
    explicit InvariantStrengthening(const PLAJA::Configuration& config);
    ~InvariantStrengthening() override;
    DELETE_CONSTRUCTOR(InvariantStrengthening)

};

#endif //PLAJA_INVARIANT_STRENGTHENING_H
