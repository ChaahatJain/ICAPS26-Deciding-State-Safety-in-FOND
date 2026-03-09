//
// This file is part of the PlaJA code base.
// Copyright (c) (2023 - 2024) Chaahat Jain.
//

#ifndef PLAJA_SOLUTION_CHECK_WRAPPER_VERITAS_H
#define PLAJA_SOLUTION_CHECK_WRAPPER_VERITAS_H

#include <memory>
#include "../../../utils/default_constructors.h"
#include "../../../assertions.h"
#include "../../smt/base/solution_check_wrapper.h"
#include "../../states/forward_states.h"
#include "../forward_smt_veritas.h"

namespace VERITAS_IN_PLAJA {

    class SolutionCheckWrapper: public ::SolutionCheckWrapper {

    private:
        const ModelVeritas* model;
        const VERITAS_IN_PLAJA::Solution* solutionPtr; // resposibility of callee to keep object in memory

    protected:

        inline void set_solution_ptr(const VERITAS_IN_PLAJA::Solution& solution) { solutionPtr = &solution; }

    public:
        SolutionCheckWrapper(std::unique_ptr<::SolutionCheckerInstance>&& solution_checker_instance, const ModelVeritas& model);
        ~SolutionCheckWrapper() override;
        DELETE_CONSTRUCTOR(SolutionCheckWrapper)

        [[nodiscard]] std::unique_ptr<StateValues> to_state(StepIndex_type step, bool include_locs) const override;

        void to_value(StepIndex_type step, StateValues& target, VariableIndex_type state_index) const override;

        /* veritas interface */

        [[nodiscard]] bool check(const VERITAS_IN_PLAJA::Solution& solution);

        void set(const VERITAS_IN_PLAJA::Solution& solution);

    };

}

#endif //PLAJA_SOLUTION_CHECK_WRAPPER_VERITAS_H
