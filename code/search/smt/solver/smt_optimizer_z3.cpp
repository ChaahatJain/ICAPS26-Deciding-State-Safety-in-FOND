//
// Created by Daniel Sherbakov in 2024.
//

#include "smt_optimizer_z3.h"

#include "../../../exception/smt_exception.h"
#include "../../../parser/ast/expression/expression.h"
#include "../bias_functions/bias_to_z3.h"
#include "solution_z3.h"


#include <utility>

#ifndef NDEBUG

const z3::expr* Z3Assertions::retrieve_bound(Z3_IN_PLAJA::VarId_type var) const {
    const auto it = registeredBounds.find(var);
    return it == registeredBounds.cend() ? nullptr : &it->second;
}

#endif

SMTOptimizer::SMTOptimizer(std::shared_ptr<Z3_IN_PLAJA::Context>&& c, z3::expr objective):
    Z3_IN_PLAJA::SMTSolver(std::move(c))
    , optimizer(_context()())
    , objective(std::move(objective))
    , assertions(_context()().bool_val(true))
{
    // z3::set_param("unsat_core", true);
    pushedAssertions.emplace_back(_context()());
    // optimizer.push();
}


bool SMTOptimizer::check() {
    switch (optimizer.check()) {
        case z3::sat: {
            print_bias_value();
            return true;
        }
        case z3::unsat: {
            // z3::expr_vector constraints(_context()());
            // constraints.push_back(assertions);
            auto core = optimizer.unsat_core();
            std::cout << "UNSAT " << core.size() << std::endl;
            for (const auto& expr : core) {
                std::cerr << expr.to_string() << std::endl;
            }
            return false;
        }
        case z3::unknown: { throw SMTException(SMTException::Z3, SMTException::Unknown, "z3_undecided_z3_query.z3"); }
        default: { PLAJA_ABORT }
    }
}

std::unique_ptr<Z3_IN_PLAJA::Solution> SMTOptimizer::get_solution() const { return std::make_unique<Z3_IN_PLAJA::Solution>(*this); }
