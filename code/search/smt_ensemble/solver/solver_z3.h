//
// This file is part of the PlaJA code base.
// Copyright (c) (2023 - 2024) Chaahat Jain.
//

#ifndef PLAJA_ENSEMBLE_SOLVER_Z3
#define PLAJA_ENSEMBLE_SOLVER_Z3

#include "../../states/state_values.h"
#include <memory>
#include "../../predicate_abstraction/solution_checker_pa.h"
#include "solution_check_wrapper_veritas.h"
#include <stack>
#include "../../smt/base/smt_solver.h"
#include "../veritas_query.h"
#include "solver.h"
#include "z3.h"
#include "../to_z3/veritas_to_z3.h"
#include "../../predicate_abstraction/smt/forward_smt_pa.h"
#include "../../smt/solver/smt_solver_z3.h"

namespace VERITAS_IN_PLAJA {

class SolverZ3 : public VERITAS_IN_PLAJA::Solver {

private:
    bool check_internal();
    const ModelZ3PA& modelZ3;
    std::unique_ptr<Z3_IN_PLAJA::SMTSolver> ensembleSatSolver;
    std::unique_ptr<Z3_IN_PLAJA::VeritasToZ3Vars> variables;
    std::vector<z3::expr> outputInterfacePerIndex;

    bool load_policy(ActionLabel_type action_label);

public:
    explicit SolverZ3(const PLAJA::Configuration& config, std::shared_ptr<VERITAS_IN_PLAJA::Context>&& c);
    explicit SolverZ3(const PLAJA::Configuration& config, const VERITAS_IN_PLAJA::VeritasQuery& query);
    ~SolverZ3() override;
    bool check(int action_label) override;

    void reset() override { ensembleSatSolver->reset(); }

    std::unique_ptr<SolutionCheckerInstance> extract_solution_via_checker() override;
    std::vector<veritas::Interval> extract_box() override { return {}; }
    std::vector<std::vector<veritas::Interval>> extract_boxes() override { return {}; }

};

}

#endif //PLAJA_ENSEMBLE_SOLVER_Z3