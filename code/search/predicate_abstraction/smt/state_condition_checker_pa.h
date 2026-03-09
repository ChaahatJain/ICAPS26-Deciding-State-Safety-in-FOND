//
// This file is part of the PlaJA code base.
// Copyright (c) (2019 - 2022) Marcel Vinzent.
// See README.md in the top-level directory for licensing information.
//

#ifndef PLAJA_STATE_CONDITION_CHECKER_PA_H
#define PLAJA_STATE_CONDITION_CHECKER_PA_H

#include <memory>
#include "../../../parser/ast/expression/forward_expression.h"
#include "../../../stats/forward_stats.h"
#include "../../../utils/default_constructors.h"
#include "../../factories/forward_factories.h"
#include "../../smt/forward_smt_z3.h"
#include "../optimization/forward_optimization_pa.h"
#include "../pa_states/forward_pa_states.h"
#include "forward_smt_pa.h"

class StateConditionCheckerPa {

    std::unique_ptr<StateConditionExpression> stateCondition;
    std::unique_ptr<PaEntailments> entailments;
    std::unique_ptr<Z3_IN_PLAJA::SMTSolver> solver;

    bool entailmentsSufficient; // Are entailments sufficient to fulfill condition?
    mutable bool lastCheckExact;

#ifndef NDEBUG
    std::unique_ptr<Z3_IN_PLAJA::SMTSolver> solverDebug;
#endif

public:
    explicit StateConditionCheckerPa(const PLAJA::Configuration& config, const ModelZ3PA& model_pa, const Expression* condition = nullptr);
    ~StateConditionCheckerPa();
    DELETE_CONSTRUCTOR(StateConditionCheckerPa)

    [[nodiscard]] bool check(const AbstractState& abstract_state) const;

    /* Truth value holds for all concrete states in abstract state? */
    [[nodiscard]] inline bool is_exact() const { return lastCheckExact; }

    void increment();

};

#endif //PLAJA_STATE_CONDITION_CHECKER_PA_H
