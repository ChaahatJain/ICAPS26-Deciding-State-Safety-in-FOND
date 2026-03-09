//
// This file is part of the PlaJA code base.
// Copyright (c) (2019 - 2023) Marcel Vinzent.
// See README.md in the top-level directory for licensing information.
//

#ifndef PLAJA_SMT_SOLVER_MARABOU_BB_H
#define PLAJA_SMT_SOLVER_MARABOU_BB_H

#include "smt_solver_marabou.h"

namespace MARABOU_IN_PLAJA {

    struct BranchingVariable;

    class SmtSolverBB final: public SMTSolver {

        friend BranchingVariable;

    private:
        /* Config. */
        unsigned int maxBranchingDepthNonReal;
        bool onlyAcceptExactSolution;
        /**/
        unsigned int branchingDepthNonReal;
        unsigned int branchingDepth; // For stats.
        bool hasContinuousVars;

        // static MarabouVarIndex_type select_branching_var(const std::list<std::pair<MarabouVarIndex_type, MarabouInteger_type>>& candidates);
        void select_branching_var(BranchingVariable& branching_variable, const MARABOU_IN_PLAJA::Solution& solution) const;

        void compute_branches(BranchingVariable& branching_var);

        void push_branch(BranchingVariable& branching_var, unsigned int branch_index);
        void pop_branch();

        bool check_recursively();
        bool check_init();

    public:
        explicit SmtSolverBB(const PLAJA::Configuration& config, std::shared_ptr<MARABOU_IN_PLAJA::Context>&& c);
        explicit SmtSolverBB(const PLAJA::Configuration& config, std::unique_ptr<MARABOU_IN_PLAJA::MarabouQuery>&& query);
        ~SmtSolverBB() override;
        DELETE_CONSTRUCTOR(SmtSolverBB)

        [[nodiscard]] bool is_exact() const override;

        bool check() override;
    };

}

#endif //PLAJA_SMT_SOLVER_MARABOU_BB_H
