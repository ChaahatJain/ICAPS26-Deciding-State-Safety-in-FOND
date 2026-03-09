//
// This file is part of the PlaJA code base.
// Copyright (c) (2019 - 2023) Marcel Vinzent.
// See README.md in the top-level directory for licensing information.
//

#include "solution_check_wrapper_marabou.h"
#include "../../states/state_values.h"
#include "../model/model_marabou.h"
#include "../vars_in_marabou.h"
#include "solution_marabou.h"

MARABOU_IN_PLAJA::SolutionCheckWrapper::SolutionCheckWrapper(std::unique_ptr<::SolutionCheckerInstance>&& solution_checker_instance, const ModelMarabou& model):
    ::SolutionCheckWrapper(std::move(solution_checker_instance))
    , model(&model)
    , solutionPtr(nullptr) {
}

MARABOU_IN_PLAJA::SolutionCheckWrapper::~SolutionCheckWrapper() = default;

std::unique_ptr<StateValues> MARABOU_IN_PLAJA::SolutionCheckWrapper::to_state(StepIndex_type step, bool include_locs) const {
    auto solution_jani = std::make_unique<StateValues>(get_instance().get_solution_constructor());
    const auto& marabou_interface = model->get_state_indexes(step);
    PLAJA_ASSERT(solutionPtr)
    marabou_interface.extract_solution(*solutionPtr, *solution_jani, include_locs);
    return solution_jani;
}

void MARABOU_IN_PLAJA::SolutionCheckWrapper::to_value(StepIndex_type step, StateValues& target, VariableIndex_type state_index) const {
    const auto& marabou_interface = model->get_state_indexes(step);
    PLAJA_ASSERT(solutionPtr)
    marabou_interface.extract_solution(*solutionPtr, target, state_index);
}

bool MARABOU_IN_PLAJA::SolutionCheckWrapper::check(const MARABOU_IN_PLAJA::Solution& solution) {
    set_solution_ptr(solution);
    return get_instance().check(*this);
}

void MARABOU_IN_PLAJA::SolutionCheckWrapper::set(const MARABOU_IN_PLAJA::Solution& solution) {
    set_solution_ptr(solution);
    get_instance().set(*this);
}