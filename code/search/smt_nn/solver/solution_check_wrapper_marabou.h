//
// This file is part of the PlaJA code base.
// Copyright (c) (2019 - 2023) Marcel Vinzent.
// See README.md in the top-level directory for licensing information.
//

#ifndef PLAJA_SOLUTION_CHECK_WRAPPER_MARABOU_H
#define PLAJA_SOLUTION_CHECK_WRAPPER_MARABOU_H

#include <memory>
#include "../../../utils/default_constructors.h"
#include "../../../assertions.h"
#include "../../smt/base/solution_check_wrapper.h"
#include "../../states/forward_states.h"
#include "../forward_smt_nn.h"

namespace MARABOU_IN_PLAJA {

    class SolutionCheckWrapper: public ::SolutionCheckWrapper {

    private:
        const ModelMarabou* model;
        const MARABOU_IN_PLAJA::Solution* solutionPtr; // resposibility of callee to keep object in memory

    protected:

        inline void set_solution_ptr(const MARABOU_IN_PLAJA::Solution& solution) { solutionPtr = &solution; }

    public:
        SolutionCheckWrapper(std::unique_ptr<::SolutionCheckerInstance>&& solution_checker_instance, const ModelMarabou& model);
        ~SolutionCheckWrapper() override;
        DELETE_CONSTRUCTOR(SolutionCheckWrapper)

        [[nodiscard]] std::unique_ptr<StateValues> to_state(StepIndex_type step, bool include_locs) const override;

        void to_value(StepIndex_type step, StateValues& target, VariableIndex_type state_index) const override;

        /* marabou interface */

        [[nodiscard]] bool check(const MARABOU_IN_PLAJA::Solution& solution);

        void set(const MARABOU_IN_PLAJA::Solution& solution);

    };

}

#endif //PLAJA_SOLUTION_CHECK_WRAPPER_MARABOU_H
