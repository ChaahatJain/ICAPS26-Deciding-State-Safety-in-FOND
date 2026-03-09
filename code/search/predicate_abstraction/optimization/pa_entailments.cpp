//
// This file is part of PlaJA (2019 - 2024).
// Copyright (c) Marcel Vinzent (2024).
//

#include "pa_entailments.h"
#include "../../../include/compiler_config_const.h"
#include "../../factories/configuration.h"
#include "../../smt/solver/smt_solver_z3.h"
#include "../smt/model_z3_pa.h"

/* Hack to support "predicate_abstraction_test" legacy, see predicate_relations.cpp */
STMT_IF_DEBUG(extern bool allowTrivialPreds)

PaEntailments::PaEntailments(const PLAJA::Configuration& config, const ModelZ3PA& model_pa, const Expression* condition_ptr):
    condition(condition_ptr)
    , modelPa(&model_pa)
    , stats(config.get_sharable_ptr<PLAJA::StatsBase>(PLAJA::SharableKey::STATS))
    , maxPredIndex(-1) {
    increment();
}

PaEntailments::~PaEntailments() = default;

/* */

void PaEntailments::increment() {

    /* Nothing to be done without condition.
     * That is, we do not check whether predicates might be entailed by variable bounds.
     * In debug mode, we check that this is not the case.
     */
    if (not PLAJA_GLOBAL::debug and not condition) { return; }

    auto solver = Z3_IN_PLAJA::SMTSolver(modelPa->share_context(), stats);
    modelPa->add_src_bounds(solver, true);

    solver.push();
    if (not PLAJA_GLOBAL::debug or condition) { modelPa->add_expression(solver, *condition, 0); }

    auto num_preds = modelPa->get_number_predicates(); // Check incrementally ...

    for (PredicateIndex_type pred_index = maxPredIndex + 1; pred_index < num_preds; ++pred_index) {

        /* Check whether "reach entails !p". */
        solver.push();
        solver.add(modelPa->get_src_predicate(pred_index));
        if (not solver.check_pop()) {
            entailments.emplace_back(pred_index, false);
            continue;
        }

        /* Check whether "reach entails p_i". */
        solver.push();
        solver.add(!modelPa->get_src_predicate(pred_index));
        if (not solver.check_pop()) {
            entailments.emplace_back(pred_index, true);
            continue;
        }

    }

    solver.pop();

    PLAJA_ASSERT(allowTrivialPreds or condition or entailments.empty()) // No entailments without condition.

    maxPredIndex = num_preds - 1;
}