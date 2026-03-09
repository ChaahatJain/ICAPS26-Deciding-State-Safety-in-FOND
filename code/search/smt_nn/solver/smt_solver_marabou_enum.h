//
// This file is part of the PlaJA code base.
// Copyright (c) (2019 - 2023) Marcel Vinzent.
// See README.md in the top-level directory for licensing information.
//

#ifndef PLAJA_SMT_SOLVER_MARABOU_ENUM_H
#define PLAJA_SMT_SOLVER_MARABOU_ENUM_H

#include "../../states/forward_states.h"
#include "../forward_smt_nn.h"
#include "smt_solver_marabou.h"

namespace MARABOU_IN_PLAJA {

    class SmtSolverEnum final: public SMTSolver {

    private:
        bool check_recursively(std::list<MarabouVarIndex_type>& input_list, MARABOU_IN_PLAJA::Solution& solution);

    public:
        explicit SmtSolverEnum(const PLAJA::Configuration& config, std::shared_ptr<MARABOU_IN_PLAJA::Context>&& c);
        explicit SmtSolverEnum(const PLAJA::Configuration& config, std::unique_ptr<MARABOU_IN_PLAJA::MarabouQuery>&& query);
        ~SmtSolverEnum() override;
        DELETE_CONSTRUCTOR(SmtSolverEnum)

        [[nodiscard]] bool is_exact() const override;

        bool check() override;

    };

}

#endif //PLAJA_SMT_SOLVER_MARABOU_ENUM_H
