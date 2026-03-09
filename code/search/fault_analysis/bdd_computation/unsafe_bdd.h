//
// This file is part of the PlaJA code base.
// Copyright (c) (2023 - 2024) Chaahat Jain.
//

#ifndef PLAJA_UNSAFE_BDD_H
#define PLAJA_UNSAFE_BDD_H

#include <memory>
#include <vector>
#include <stack>
#include "../../../parser/ast/expression/forward_expression.h"
#include "../../fd_adaptions/search_engine.h"
#include "bdd_query.h"
#include "../../smt/using_smt.h"
#include "../../smt/forward_smt_z3.h"
#include "../../states/forward_states.h"
#include "../../successor_generation/update.h"
#include "../../successor_generation/action_op.h"
#include "bdd_query.h"
class SuccessorGeneratorAbstract;

class UnsafeBDD final: public SearchEngine {

private:
//
    BDD_IN_PLAJA::Query query;
    std::unique_ptr<ModelZ3> modelZ3;
    std::unique_ptr<Z3_IN_PLAJA::SMTSolver> solverZ3;
    
    std::string unsafePath;
    BDD rho;
    BDD tau;
    BDD reachable;
    

    SearchStatus initialize() override;
    SearchStatus finalize() override;
    SearchStatus step() override;

    /* Override stats. */
    void print_statistics() const override;
    void stats_to_csv(std::ofstream& file) const override;
    void stat_names_to_csv(std::ofstream& file) const override;

public:
    explicit UnsafeBDD(const PLAJA::Configuration& config);
    ~UnsafeBDD() override;
    DELETE_CONSTRUCTOR(UnsafeBDD)

};

#endif //PLAJA_UNSAFE_BDD_H
