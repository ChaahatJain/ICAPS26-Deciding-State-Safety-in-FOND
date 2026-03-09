//
// This file is part of the PlaJA code base.
// Copyright (c) (2019 - 2022) Marcel Vinzent.
// See README.md in the top-level directory for licensing information.
//

#ifndef PLAJA_SPURIOUSNESS_CHECKER_H
#define PLAJA_SPURIOUSNESS_CHECKER_H

#include <memory>
#include "../../../parser/ast/expression/forward_expression.h"
#include "../../../parser/ast/forward_ast.h"
#include "../../../assertions.h"
#include "../../factories/forward_factories.h"
#include "../../smt/base/forward_solution_check.h"
#include "../../states/forward_states.h"
#include "../../successor_generation/forward_successor_generation.h"
#include "../pa_states/forward_pa_states.h"
#include "../forward_pa.h"
#include "forward_cegar.h"
#include "selection_refinement.h"


class SpuriousnessChecker {

private:
    const PredicatesStructure* predicates;
    std::unique_ptr<PathExistenceChecker> pathExistenceChecker;
    std::unique_ptr<SelectionRefinement> selectionRefinement;
    bool paStateAware;
    bool checkPolicySpuriousness;
    mutable bool encounteredNonDetUpdate; // Under non-det. updates, we always need SMT to guarantee completeness.

    static void add_relevant_pred_split(SpuriousnessResult& spuriousness_result, const Expression& predicate, const StateBase& state);
    static void add_violated_predicates(SpuriousnessResult& spuriousness_result, const AbstractState& abstract_state, const StateBase& state);

    [[nodiscard]] std::unique_ptr<SpuriousnessResult> check_path(std::unique_ptr<SolutionCheckerInstancePaPath>&& path_instance) const;
    [[nodiscard]] std::unique_ptr<SpuriousnessResult> check_path_nn(std::unique_ptr<SolutionCheckerInstancePaPath>&& path_instance) const;

public:
    explicit SpuriousnessChecker(const PLAJA::Configuration& configuration, const PredicatesStructure& predicates_);
    ~SpuriousnessChecker();
    DELETE_CONSTRUCTOR(SpuriousnessChecker)

    /** Check an abstract path for spuriousness. */
    [[nodiscard]] std::unique_ptr<SpuriousnessResult> check_path(const AbstractPath& path) const;
    #ifdef USE_VERITAS
    [[nodiscard]] bool needs_interval() const { return selectionRefinement->needs_interval_witness(); }
    #endif

};

#endif //PLAJA_SPURIOUSNESS_CHECKER_H
