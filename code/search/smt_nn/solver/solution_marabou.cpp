//
// This file is part of the PlaJA code base.
// Copyright (c) (2019 - 2023) Marcel Vinzent.
// See README.md in the top-level directory for licensing information.
//

#include "solution_marabou.h"
#include "../../../utils/floating_utils.h"
#include "../../../utils/utils.h"
#include "../constraints_in_marabou.h"
#include "../marabou_query.h"

MARABOU_IN_PLAJA::Solution::Solution(const MARABOU_IN_PLAJA::MarabouQuery& query_):
    query(&query_)
    , assignment()
    , clippedAssignment() {
}

MARABOU_IN_PLAJA::Solution::~Solution() = default;

void MARABOU_IN_PLAJA::Solution::set_value(MarabouVarIndex_type var, MarabouFloating_type val) {
    PLAJA_ASSERT(not assignment.count(var))

    /*
     * OOB solutions are possible due to numeric issues.
     * We only clip if val violates bounds beyond tolerance however.
     */
    if constexpr (PLAJA_GLOBAL::marabouHandleNumericIssues) {

        const auto lb = query->get_lower_bound(var);
        const auto ub = query->get_upper_bound(var);

        if (FloatUtils::lt(val, lb)) {
            clippedAssignment.emplace(var, val);
            val = lb;
        } else if (FloatUtils::gt(val, ub)) {
            clippedAssignment.emplace(var, val);
            val = ub;
        }

    }

    assignment.emplace(var, val);

}

bool MARABOU_IN_PLAJA::Solution::is_integer() const {
    const auto& integer_vars = query->_integer_vars();
    return std::all_of(integer_vars.begin(), integer_vars.end(), [this](MarabouInteger_type var) { return PLAJA_FLOATS::is_integer(get_value(var), MARABOU_IN_PLAJA::integerPrecision); });
}

/**********************************************************************************************************************/

std::unique_ptr<::MarabouConstraint> MARABOU_IN_PLAJA::Solution::generate_solution_conjunction() const {
    std::unique_ptr<ConjunctionInMarabou> conjunction(new ConjunctionInMarabou(query->_context()));

    // AND_{v in V} v = sol(v)

    for (const auto& [var, val]: assignment) {
        PLAJA_ASSERT(query->is_marked_input(var))
        conjunction->add_equality_constraint(var, val);
    }

    return conjunction;
}

void MARABOU_IN_PLAJA::Solution::add_solution_conjunction(MARABOU_IN_PLAJA::QueryConstructable& query_) const {
    for (const auto& [var, val]: assignment) {
        PLAJA_ASSERT(query->is_marked_input(var))
        query_.tighten_lower_bound(var, val);
        query_.tighten_upper_bound(var, val);
    }
}

std::unique_ptr<MarabouConstraint> MARABOU_IN_PLAJA::Solution::generate_solution_exclusion() const {
    std::unique_ptr<DisjunctionInMarabou> exclusion(new DisjunctionInMarabou(query->_context()));
    exclusion->reserve_for_additional(2 * assignment.size());

    // OR_{v in V} v < sol(v) or v > sol(v)

    for (const auto& [var, val]: assignment) {
        PLAJA_ASSERT(query->is_marked_input(var))
        if (query->is_marked_integer(var)) {
            auto new_upper = static_cast<MarabouInteger_type>(std::floor(val));
            auto new_lower = static_cast<MarabouInteger_type>(std::ceil(val));
            // integer solution ?
            if (new_upper == new_lower) {
                new_upper = static_cast<PLAJA::integer>(std::round(val)) - 1; // var <= new_upper
                new_lower = static_cast<PLAJA::integer>(std::round(val)) + 1; // var >= new_lower
            }
            //
            if (query->get_lower_bound_int(var) <= new_upper) { BoundConstraint(query->_context(), var, Tightening::UB, new_upper).move_to_disjunction(*exclusion); }
            if (new_lower <= query->get_upper_bound_int(var)) { BoundConstraint(query->_context(), var, Tightening::LB, new_lower).move_to_disjunction(*exclusion); }
        } else {
            const auto new_upper = val - MARABOU_IN_PLAJA::strictnessPrecision;
            const auto new_lower = val + MARABOU_IN_PLAJA::strictnessPrecision;
            //
            if (query->get_lower_bound(var) <= new_upper) { BoundConstraint(query->_context(), var, Tightening::UB, new_upper).move_to_disjunction(*exclusion); }
            if (new_lower <= query->get_upper_bound(var)) { BoundConstraint(query->_context(), var, Tightening::LB, new_lower).move_to_disjunction(*exclusion); }
        }
    }

    exclusion->shrink_to_fit();
    return exclusion;
}

void MARABOU_IN_PLAJA::Solution::add_solution_exclusion(MARABOU_IN_PLAJA::QueryConstructable& query_) const { generate_solution_exclusion()->move_to_query(query_); }

/**********************************************************************************************************************/

FCT_IF_DEBUG(void MARABOU_IN_PLAJA::Solution::dump() const {

    std::stringstream ss;

    const auto end = assignment.cend();
    for (MarabouVarIndex_type var = 0; var < query->_query().getNumberOfVariables(); ++var) { // To preserve order.
        const auto it = assignment.find(var);
        if (it != end) { ss << "[" << var << ": " << it->second << "]" << PLAJA_UTILS::commaString << PLAJA_UTILS::spaceString; }
    }

    PLAJA_LOG_DEBUG(ss.str())

})
