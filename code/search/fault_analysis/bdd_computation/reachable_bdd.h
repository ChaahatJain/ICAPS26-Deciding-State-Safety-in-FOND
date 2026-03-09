//
// This file is part of the PlaJA code base.
// Copyright (c) (2023 - 2024) Chaahat Jain.
//

#ifndef PLAJA_REACHABLE_BDD_H
#define PLAJA_REACHABLE_BDD_H

#include <memory>
#include <vector>
#include <stack>
#include "../../../parser/ast/expression/forward_expression.h"
#include "../../fd_adaptions/search_engine.h"
#include "../../smt/using_smt.h"
#include "../../smt/forward_smt_z3.h"
#include "../../states/forward_states.h"
#include "../../successor_generation/update.h"
#include "../../successor_generation/action_op.h"
#include "bdd_query.h"
class SuccessorGeneratorAbstract;

class ReachableBDD final: public SearchEngine {

private:
//
    BDD_IN_PLAJA::Query query;
    std::unique_ptr<ModelZ3> modelZ3;
    std::unique_ptr<Z3_IN_PLAJA::SMTSolver> solverZ3;
    
    std::string reachablePath;
    BDD reachable;
    BDD unsafe; 
    BDD rho;   

    SearchStatus initialize() override;
    SearchStatus finalize() override;
    SearchStatus step() override;

    /* Override stats. */
    void print_statistics() const override;
    void stats_to_csv(std::ofstream& file) const override;
    void stat_names_to_csv(std::ofstream& file) const override;

public:
    explicit ReachableBDD(const PLAJA::Configuration& config);
    ~ReachableBDD() override;
    DELETE_CONSTRUCTOR(ReachableBDD)

};

#endif //PLAJA_REACHABLE_BDD_H
