//
// This file is part of the PlaJA code base.
// Copyright (c) (2023 - 2024) Chaahat Jain.
//

#include "solution_check_wrapper_veritas.h"
#include "../../states/state_values.h"
#include "../model/model_veritas.h"
#include "solution_veritas.h"

VERITAS_IN_PLAJA::SolutionCheckWrapper::SolutionCheckWrapper(std::unique_ptr<::SolutionCheckerInstance>&& solution_checker_instance, const ModelVeritas& model):
    ::SolutionCheckWrapper(std::move(solution_checker_instance))
    , model(&model)
    , solutionPtr(nullptr) {
}

VERITAS_IN_PLAJA::SolutionCheckWrapper::~SolutionCheckWrapper() = default;

std::unique_ptr<StateValues> VERITAS_IN_PLAJA::SolutionCheckWrapper::to_state(StepIndex_type step, bool include_locs) const {
    auto solution_jani = std::make_unique<StateValues>(get_instance().get_solution_constructor());
    const auto& veritas_interface = model->get_state_indexes(step);
    PLAJA_ASSERT(solutionPtr)
    veritas_interface.extract_solution(*solutionPtr, *solution_jani, include_locs);
    return solution_jani;
}

void VERITAS_IN_PLAJA::SolutionCheckWrapper::to_value(StepIndex_type step, StateValues& target, VariableIndex_type state_index) const {
    const auto& veritas_interface = model->get_state_indexes(step);
    PLAJA_ASSERT(solutionPtr)
    veritas_interface.extract_solution(*solutionPtr, target, state_index);
}

bool VERITAS_IN_PLAJA::SolutionCheckWrapper::check(const VERITAS_IN_PLAJA::Solution& solution) {
    set_solution_ptr(solution);
    return get_instance().check(*this);
}

void VERITAS_IN_PLAJA::SolutionCheckWrapper::set(const VERITAS_IN_PLAJA::Solution& solution) {
    set_solution_ptr(solution);
    get_instance().set(*this);
}