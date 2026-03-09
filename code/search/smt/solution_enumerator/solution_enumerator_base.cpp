//
// This file is part of the PlaJA code base.
// Copyright (c) (2019 - 2024) Marcel Vinzent.
// See README.md in the top-level directory for licensing information.
//

#include "solution_enumerator_base.h"
#include "../../../option_parser/enum_option_values_set.h"
#include "../../../utils/rng.h"
#include "../../information/model_information.h"
#include "../../states/state_values.h"

namespace PLAJA_OPTION {

    namespace SOLUTION_ENUMERATION {

        extern std::unique_ptr<PLAJA_OPTION::EnumOptionValuesSet> construct_default_configs_enum();

        std::unique_ptr<PLAJA_OPTION::EnumOptionValuesSet> construct_default_configs_enum() {
            return std::make_unique<PLAJA_OPTION::EnumOptionValuesSet>(
                std::list<PLAJA_OPTION::EnumOptionValue> {
                    PLAJA_OPTION::EnumOptionValue::Naive,
#ifdef USE_Z3
                    PLAJA_OPTION::EnumOptionValue::Inc,
                    PLAJA_OPTION::EnumOptionValue::Bs,
                    PLAJA_OPTION::EnumOptionValue::Td,
                    PLAJA_OPTION::EnumOptionValue::Sample,
                    PLAJA_OPTION::EnumOptionValue::Biased,
                    PLAJA_OPTION::EnumOptionValue::WP,
                    PLAJA_OPTION::EnumOptionValue::AvoidCount,
#endif
                },
                PLAJA_GLOBAL::useZ3 ? PLAJA_OPTION::EnumOptionValue::Inc : PLAJA_OPTION::EnumOptionValue::Naive
            );
        }

    }

}

SolutionEnumeratorBase::SolutionEnumeratorBase(const ModelInformation& model_info):
    rng(PLAJA_GLOBAL::rng.get())
    , modelInfo(&model_info)
    , defaultValues(new StateValues(model_info.get_initial_values()))
    , initialized(false) {
}

SolutionEnumeratorBase::~SolutionEnumeratorBase() = default;

/* */

void SolutionEnumeratorBase::set_constructor(std::unique_ptr<StateValues>&& default_values) { defaultValues = std::move(default_values); }

/* */

void SolutionEnumeratorBase::initialize() {
    PLAJA_ASSERT(not is_initialized())
    set_initialized();
}

std::unique_ptr<StateValues> SolutionEnumeratorBase::retrieve_solution() {
    PLAJA_ASSERT(is_initialized())
    if (solutions.empty()) { return nullptr; }
    auto solution = std::make_unique<StateValues>(std::move(solutions.front()));
    solutions.pop_front();
    return solution;
}

std::list<StateValues> SolutionEnumeratorBase::enumerate_solutions() {
    if (not is_initialized()) { initialize(); }
    return std::move(solutions);
}

std::unique_ptr<StateValues> SolutionEnumeratorBase::sample() {
    PLAJA_ASSERT(is_initialized())
    if (solutions.empty()) { return nullptr; }

    auto index = rng->index(solutions.size());

    for (const auto& solution: solutions) {
        if (index == 0) { return solution.to_ptr(); }
        --index;
    }

    PLAJA_ABORT
}