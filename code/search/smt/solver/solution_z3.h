//
// This file is part of the PlaJA code base.
// Copyright (c) (2019 - 2023) Marcel Vinzent.
// See README.md in the top-level directory for licensing information.
//

#ifndef PLAJA_SOLUTION_Z3_H
#define PLAJA_SOLUTION_Z3_H

#include <z3++.h>
#include "../../../parser/ast/expression/forward_expression.h"
#include "../../../assertions.h"
#include "../forward_smt_z3.h"
#include "../using_smt.h"

namespace Z3_IN_PLAJA {

    class Solution final {

    private:
        const Z3_IN_PLAJA::SMTSolver* solver;
        z3::model modelZ3;

    public:
        explicit Solution(const Z3_IN_PLAJA::SMTSolver& solver);
        ~Solution();
        DELETE_CONSTRUCTOR(Solution)

        [[nodiscard]] static std::unique_ptr<::Expression> to_value(const z3::expr& e);

        inline static z3::expr eval(const z3::expr& e, const z3::model& m, bool model_completion = true) { return m.eval(e, model_completion); }

        [[nodiscard]] inline z3::expr eval(const z3::expr& e, bool model_completion = true) const { return eval(e, modelZ3, model_completion); }

        [[nodiscard]] z3::expr eval(VarId_type var, bool model_completion = true) const;

        [[nodiscard]] std::unique_ptr<::Expression> eval_to_value(const z3::expr& e, bool model_completion = true) const;
        [[nodiscard]] std::unique_ptr<::Expression> eval_to_value(VarId_type var, bool model_completion = true) const;

    };

}

#endif //PLAJA_SOLUTION_Z_3_H
