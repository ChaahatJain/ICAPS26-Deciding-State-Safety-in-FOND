//
// This file is part of the PlaJA code base.
// Copyright (c) (2023 - 2024) Chaahat Jain.
//

#include "solution_veritas.h"
#include "../../../utils/floating_utils.h"
#include "../../../utils/utils.h"
#include "../constraints_in_veritas.h"
#include "../veritas_query.h"

VERITAS_IN_PLAJA::Solution::Solution(const VERITAS_IN_PLAJA::VeritasQuery& query_):
    query(&query_)
    , assignment()
    , clippedAssignment() {
}

VERITAS_IN_PLAJA::Solution::~Solution() = default;

void VERITAS_IN_PLAJA::Solution::set_value(VeritasVarIndex_type var, VeritasFloating_type val) {
    PLAJA_ASSERT(not assignment.count(var))
    assignment.emplace(var, val);
}

bool VERITAS_IN_PLAJA::Solution::is_integer() const {
    const auto& integer_vars = query->_integer_vars();
    return std::all_of(integer_vars.begin(), integer_vars.end(), [this](VeritasInteger_type var) { return PLAJA_FLOATS::is_integer(get_value(var), VERITAS_IN_PLAJA::integerPrecision); });
}

/**********************************************************************************************************************/

std::unique_ptr<::VeritasConstraint> VERITAS_IN_PLAJA::Solution::generate_solution_conjunction() const {
    std::unique_ptr<ConjunctionInVeritas> conjunction(new ConjunctionInVeritas(query->_context()));

    // AND_{v in V} v = sol(v)

    for (const auto& [var, val]: assignment) {
        // PLAJA_ASSERT(query->is_marked_input(var))
        conjunction->add_equality_constraint(var, val);
    }

    return conjunction;
}

void VERITAS_IN_PLAJA::Solution::add_solution_conjunction(VERITAS_IN_PLAJA::QueryConstructable& query_) const {
    for (const auto& [var, val]: assignment) {
        // PLAJA_ASSERT(query->is_marked_input(var))
        query_.tighten_lower_bound(var, val);
        query_.tighten_upper_bound(var, val);
    }
}

/**********************************************************************************************************************/

FCT_IF_DEBUG(void VERITAS_IN_PLAJA::Solution::dump() const {

    std::stringstream ss;

    const auto end = assignment.cend();
    for (VeritasVarIndex_type var = 0; var < query->number_of_inputs(); ++var) { // To preserve order.
        const auto it = assignment.find(var);
        if (it != end) { ss << "[" << var << ": " << it->second << "]" << PLAJA_UTILS::commaString << PLAJA_UTILS::spaceString; }
    }

    PLAJA_LOG_DEBUG(ss.str())

})
