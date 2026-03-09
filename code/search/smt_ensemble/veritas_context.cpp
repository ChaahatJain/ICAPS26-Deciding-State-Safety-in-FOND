//
// This file is part of the PlaJA code base.
// Copyright (c) (2023 - 2024) Chaahat Jain.
//

#include "veritas_context.h"
#include "constraints_in_veritas.h"
#include "predicates/Tightening.h"

unsigned int VERITAS_IN_PLAJA::Context::contextCounter = 0;

VERITAS_IN_PLAJA::Context::Context():
    PLAJA::SmtContext()
    , contextId(contextCounter++)
    , nextVar(0) {
}

VERITAS_IN_PLAJA::Context::~Context() = default;

//

bool VERITAS_IN_PLAJA::Context::tighten_bounds(VeritasVarIndex_type var, VeritasFloating_type lb, VeritasFloating_type ub) {
    PLAJA_ASSERT(exists(var))
    auto& var_info = variableInformation[var];
    const bool differ = var_info.lb != lb or ub != var_info.ub;
    // tightest possible domain
    if (var_info.lb < lb) { var_info.lb = lb; }
    if (ub < var_info.ub) { var_info.ub = ub; }
    CHECK_VERITAS_INF_BOUNDS(var_info.lb, var_info.ub)
    return differ;
}



std::unique_ptr<VeritasConstraint> VERITAS_IN_PLAJA::Context::trivially_true() { return std::make_unique<ConjunctionInVeritas>(*this); }

std::unique_ptr<VeritasConstraint> VERITAS_IN_PLAJA::Context::trivially_false() {
    PLAJA_ASSERT(get_number_of_variables() > 0)
    std::unique_ptr<ConjunctionInVeritas> conjunction_in_veritas(new ConjunctionInVeritas(*this));
    conjunction_in_veritas->add_bound(0, Tightening::LB, 1);
    conjunction_in_veritas->add_bound(0, Tightening::UB, -1);
    return conjunction_in_veritas;
}

//

#ifndef NDEBUG

#include "../../utils/utils.h"

void VERITAS_IN_PLAJA::Context::dump() const {
    for (auto var : variableInformation) {
        std::cout << "Variable: " << var.first << " lower bound: " << var.second.lb << " upper bound: " << var.second.ub << std::endl; 
    }
    for (auto var : EqAuxVars) {
        std::cout << "Equation Indicator Variable: " << var << " lower bound: " << 0 << " upper bound " << 1 << std::endl;
    }
    for (auto var : operatorAuxVars) {
        std::cout << "Indicator Variable for operator: " << var.first << " is Variable: " << var.second << std::endl;
    }
}

#endif
