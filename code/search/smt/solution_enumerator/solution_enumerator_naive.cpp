//
// This file is part of the PlaJA code base.
// Copyright (c) (2019 - 2022) Marcel Vinzent.
// See README.md in the top-level directory for licensing information.
//

#include "solution_enumerator_naive.h"
#include "../../../exception/not_supported_exception.h"
#include "../../../parser/ast/expression/expression.h"
#include "../../../utils/utils.h"
#include "../../information/model_information.h"
#include "../../states/state_values.h"

SolutionEnumeratorNaive::SolutionEnumeratorNaive(const ModelInformation& model_info, const Expression& condition_ref):
    SolutionEnumeratorBase(model_info)
    , condition(&condition_ref) {

    static bool warning_printed(false);
    if (not warning_printed) {
        PLAJA_LOG(PLAJA_UTILS::to_red_string("Warning: Naive solution enumeration."))
        PLAJA_LOG(PLAJA_UTILS::to_red_string("Consider building SMT-Module (dependency: Z3) to enable scalable enumeration methods."))
        warning_printed = true;
    }

    if (model_info.get_floating_state_size() > 0) { throw NotSupportedException("Naive solution enumeration cannot handle continuous state spaces."); }

}

SolutionEnumeratorNaive::~SolutionEnumeratorNaive() = default;

/* */

void SolutionEnumeratorNaive::compute_solutions_rec(VariableIndex_type state_index, StateValues& state_values) { // NOLINT(misc-no-recursion)

    if (state_index == get_model_info().get_int_state_size()) {

        /* Sanity: floating not supported. */
        PLAJA_ASSERT(get_model_info().get_int_state_size() == get_model_info().get_state_size())
        PLAJA_ASSERT(get_model_info().get_floating_state_size() == 0)

        if (condition->evaluate_integer(state_values)) { solutions.push_back(state_values); }

    } else {

        const auto lower_bound = get_model_info().get_lower_bound_int(state_index);
        const auto upper_bound = get_model_info().get_upper_bound_int(state_index);
        auto next_state_index = state_index + 1;

        for (auto value = lower_bound; value <= upper_bound; ++value) {
            state_values.assign_int<false>(state_index, value);
            compute_solutions_rec(next_state_index, state_values);
        }

    }
}

/* */

void SolutionEnumeratorNaive::initialize() {
    solutions.clear();
    StateValues state_values(get_model_info().get_int_state_size(), 0);
    compute_solutions_rec(0, state_values);
    set_initialized();
}

/* */

std::list<StateValues> SolutionEnumeratorNaive::compute_solutions(const Expression& condition, const ModelInformation& model_info) {
    SolutionEnumeratorNaive enumerator(model_info, condition);
    return enumerator.enumerate_solutions();
}