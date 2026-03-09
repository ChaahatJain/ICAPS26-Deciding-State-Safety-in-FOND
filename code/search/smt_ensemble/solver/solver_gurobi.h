//
// This file is part of the PlaJA code base.
// Copyright (c) (2023 - 2024) Chaahat Jain.
//

#ifndef PLAJA_ENSEMBLE_SOLVER_GUROBI
#define PLAJA_ENSEMBLE_SOLVER_GUROBI

#include "../../states/state_values.h"
#include <memory>
#include "../../predicate_abstraction/solution_checker_pa.h"
#include "solution_check_wrapper_veritas.h"
#include <stack>
#include "../../smt/base/smt_solver.h"
#include "../veritas_query.h"
#include "solver.h"
#include "gurobi_c++.h"
#include "../to_gurobi/tree_vars_in_gurobi.h"
#include "../to_gurobi/forest_vars_in_gurobi.h"
namespace VERITAS_IN_PLAJA {

class SolverGurobi : public VERITAS_IN_PLAJA::Solver {

private:
    bool check_internal();
    std::shared_ptr<GRBModel> gurobi_model = nullptr;
    std::shared_ptr<GUROBI_IN_PLAJA::VeritasToGurobiVars> variables = nullptr;
    std::shared_ptr<GUROBI_IN_PLAJA::RFVeritasToGurobiVars> rf_variables = nullptr;



public:
    GRBEnv env;
    explicit SolverGurobi(const PLAJA::Configuration& config, std::shared_ptr<VERITAS_IN_PLAJA::Context>&& c);
    explicit SolverGurobi(const PLAJA::Configuration& config, const VERITAS_IN_PLAJA::VeritasQuery& query);
    ~SolverGurobi() override;
    bool check(int action_label) override;

    void reset() override { gurobi_model = nullptr; variables = nullptr; }

    VERITAS_IN_PLAJA::Solution extract_solution();
    std::unique_ptr<SolutionCheckerInstance> extract_solution_via_checker() override;
    std::vector<veritas::Interval> extract_box() override { return {}; }
    std::vector<std::vector<veritas::Interval>> extract_boxes() override { return {}; }

};

}

#endif //PLAJA_ENSEMBLE_SOLVER_GUROBI