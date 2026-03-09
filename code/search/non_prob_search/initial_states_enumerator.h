//
// This file is part of the PlaJA code base.
// Copyright (c) (2019 - 2021) Marcel Vinzent.
// See README.md in the top-level directory for licensing information.
//

#ifndef PLAJA_INITIAL_STATES_ENUMERATOR_H
#define PLAJA_INITIAL_STATES_ENUMERATOR_H

#include "../../assertions.h"
#include "../../parser/ast/expression/forward_expression.h"
#include "../../parser/ast/forward_ast.h"
#include "../../utils/default_constructors.h"
#include "../factories/forward_factories.h"
#include "../smt/forward_smt_z3.h"
#include "../states/forward_states.h"
#include "forward_non_prob_search.h"
#include <list>
#include <memory>
#include <z3++.h>

#include <vector>

class InitialStatesEnumerator final {

private:
    std::shared_ptr<const ModelZ3> modelSmt;
    std::unique_ptr<Z3_IN_PLAJA::SMTSolver> solver;
    std::unique_ptr<SolutionEnumeratorBase> solutionEnumerator;
    std::unique_ptr<std::vector<std::pair<int, int>>> intervals;

    FIELD_IF_DEBUG(const Expression* startCondition;)

    void prepare_smt_based_enumeration(const PLAJA::Configuration& config, const StateConditionExpression& initial_state_condition);

public:
    InitialStatesEnumerator(const PLAJA::Configuration& config, const Expression& condition);
    ~InitialStatesEnumerator();
    DELETE_CONSTRUCTOR(InitialStatesEnumerator)

    void initialize();

    std::unique_ptr<StateValues> retrieve_state();

    std::list<StateValues> enumerate_states();

    [[nodiscard]] static std::list<StateValues> enumerate_states(const PLAJA::Configuration& config, const Expression& condition);

    /* */

    [[nodiscard]] bool samples() const;

    [[nodiscard]] std::unique_ptr<StateValues> sample_state();

    void update_start_condition(const Expression& new_start);
};

#endif //PLAJA_INITIAL_STATES_ENUMERATOR_H
