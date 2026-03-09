//
// This file is part of the PlaJA code base.
// Copyright (c) (2019 - 2023) Marcel Vinzent.
// See README.md in the top-level directory for licensing information.
//

#include "solution_z3.h"
#include "../../../parser/ast/expression/expression.h"
#include "../../../parser/ast/expression/expression_utils.h"
#include "../context_z3.h"
#include "smt_solver_z3.h"

Z3_IN_PLAJA::Solution::Solution(const Z3_IN_PLAJA::SMTSolver& solver):
    solver(&solver)
    , modelZ3(solver.get_model()) {}

Z3_IN_PLAJA::Solution::~Solution() = default;

/* */

std::unique_ptr<::Expression> Z3_IN_PLAJA::Solution::to_value(const z3::expr& e) {
    if (e.is_real()) { return PLAJA_EXPRESSION::make_real(e.as_double()); }
    else if (e.is_int()) { return PLAJA_EXPRESSION::make_int(e.get_numeral_int()); }
    else {
        PLAJA_ASSERT(e.is_bool())
        return PLAJA_EXPRESSION::make_bool(e.is_true());
    }
}

z3::expr Z3_IN_PLAJA::Solution::eval(Z3_IN_PLAJA::VarId_type var, bool model_completion) const { return eval(solver->_context().get_var(var), model_completion); }

std::unique_ptr<::Expression> Z3_IN_PLAJA::Solution::eval_to_value(const z3::expr& e, bool model_completion) const { return to_value(eval(e, model_completion)); }

std::unique_ptr<::Expression> Z3_IN_PLAJA::Solution::eval_to_value(Z3_IN_PLAJA::VarId_type var, bool model_completion) const { return to_value(eval(var, model_completion)); }