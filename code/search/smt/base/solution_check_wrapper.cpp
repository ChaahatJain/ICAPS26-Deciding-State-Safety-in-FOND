//
// This file is part of the PlaJA code base.
// Copyright (c) (2019 - 2023) Marcel Vinzent.
// See README.md in the top-level directory for licensing information.
//

#include "solution_check_wrapper.h"

SolutionCheckWrapper::SolutionCheckWrapper(std::unique_ptr<::SolutionCheckerInstance>&& solution_checker_instance):
    solutionCheckerInstance(std::move(solution_checker_instance)) {
}

SolutionCheckWrapper::~SolutionCheckWrapper() = default;

std::unique_ptr<::SolutionCheckerInstance> SolutionCheckWrapper::release_instance() { return std::move(solutionCheckerInstance); }
