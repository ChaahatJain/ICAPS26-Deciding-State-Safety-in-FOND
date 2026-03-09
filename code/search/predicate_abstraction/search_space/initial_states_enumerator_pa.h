//
// This file is part of the PlaJA code base.
// Copyright (c) (2019 - 2021) Marcel Vinzent.
// See README.md in the top-level directory for licensing information.
//

#ifndef PLAJA_INITIAL_STATES_ENUMERATOR_PA_H
#define PLAJA_INITIAL_STATES_ENUMERATOR_PA_H

#include <list>
#include <memory>
#include "../../../include/ct_config_const.h"
#include "../../../parser/ast/expression/forward_expression.h"
#include "../../../parser/using_parser.h"
#include "../../../utils/default_constructors.h"
#include "../../factories/forward_factories.h"
#include "../match_tree/forward_match_tree_pa.h"
#include "../optimization/forward_optimization_pa.h"
#include "../pa_states/forward_pa_states.h"
#include "../smt/forward_smt_pa.h"
#include "../using_predicate_abstraction.h"
#include "forward_search_space.h"

class InitialStatesEnumerator;

class InitialStatesEnumeratorPa {

private:
    const SearchSpacePABase* parent;
    const ModelZ3PA* modelZ3;
    const Expression* conditionPtr;
    std::unique_ptr<InitialStatesEnumerator> explicitEnumerator;
    // std::unique_ptr<StateConditionExpression> stateCondition; // Only required at construction time for now.
    std::unique_ptr<Z3_IN_PLAJA::SMTSolver> solver;
    std::unique_ptr<PredicateRelations> predicateRelations; // Own instance to include start condition as ground truth.

    /* Cache. */
    const std::size_t numLocations;
    std::size_t numPredicates;

    std::unique_ptr<StateValues> locationState;
    std::list<std::unique_ptr<PaStateValues>> paStates;

    void enumerate(PredicateState& pred_state, PredicateTraverser& predicate_traverser);

public:
    explicit InitialStatesEnumeratorPa(const PLAJA::Configuration& config, const SearchSpacePABase& parent, const Expression* condition = nullptr);
    ~InitialStatesEnumeratorPa();
    DELETE_CONSTRUCTOR(InitialStatesEnumeratorPa)

    void increment(); // Incremental usage.
    void reset_smt();

    std::list<std::unique_ptr<PaStateValues>> enumerate();
    std::list<std::unique_ptr<PaStateValues>> enumerate(const AbstractState& base); // For incremental usage.

    [[nodiscard]] bool do_enumerate_explicitly() const { return not solver; }

    std::list<std::unique_ptr<PaStateValues>> enumerate_explicitly();

};

#endif //PLAJA_INITIAL_STATES_ENUMERATOR_PA_H
