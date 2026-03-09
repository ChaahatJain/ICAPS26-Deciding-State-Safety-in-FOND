//
// This file is part of the PlaJA code base.
// Copyright (c) (2019 - 2022) Marcel Vinzent.
// See README.md in the top-level directory for licensing information.
//

#include "state_condition_checker_pa.h"
#include "../../../parser/ast/expression/non_standard/state_condition_expression.h"
#include "../../factories/configuration.h"
#include "../../smt/solver/smt_solver_z3.h"
#include "../../smt/vars_in_z3.h"
#include "../optimization/pa_entailments.h"
#include "../pa_states/abstract_state.h"
#include "model_z3_pa.h"

StateConditionCheckerPa::StateConditionCheckerPa(const PLAJA::Configuration& config, const ModelZ3PA& model_pa, const Expression* condition):
    stateCondition(nullptr)
    , entailments(nullptr)
    , solver(nullptr)
    , entailmentsSufficient(false)
    , lastCheckExact(true) {

#ifndef NDEBUG
    solverDebug = std::make_unique<Z3_IN_PLAJA::SMTSolver>(model_pa.share_context(), nullptr);
    solverDebug->push(); // To suppress scoped_timer thread in z3, which is started when using tactic2solver.
    model_pa.add_src_bounds(*solverDebug, true);
#endif

    if (not condition) { return; } // No condition is interpreted as "false" condition.

    stateCondition = StateConditionExpression::transform(condition->deepCopy_Exp());

    if (not stateCondition->get_constraint()) { return; }

    /* Prepare solver. */
    solver = std::make_unique<Z3_IN_PLAJA::SMTSolver>(model_pa.share_context(), config.get_sharable_ptr<PLAJA::StatsBase>(PLAJA::SharableKey::STATS));
    solver->push(); // To suppress scoped_timer thread in z3, which is started when using tactic2solver.
    model_pa.add_src_bounds(*solver, true);
    const auto constraint_z3 = model_pa.to_z3_condition(*stateCondition->get_constraint(), 0);
    solver->add(constraint_z3);

    if (not solver->check()) { // Trivially false.
        stateCondition = nullptr;
        solver = nullptr;
    } else {
        solver->reset();
    }

    /* Check entailments. */

    entailments = std::make_unique<PaEntailments>(config, model_pa, stateCondition->get_constraint());

    /*
     * We compute the predicate-truth-value pairs entailed by the state condition c.
     *
     * For any abstract state S such that S(p) != entailment(p), it follows S does not fulfill c -> we can skip the call to z3.
     *
     * [Sufficient entailments]
     * If for any s such that p(s) == entailment(p) for each entailed p, it holds c(s),
     * then for any S such that S(p) = entailment(p) for any entailed p, it follows S fulfills c -> we can skip the call to z3.
     *
     * What about the border case where c is unsat?
     * Handled above.
     *
     * What about the border case where S is empty?
     * We assume that this never happens, since we only consider reachable abstract states (see debug-mode check below).
     *
     * What about S(p) is undef.
     * If S(p) = entailment(p) for a subset of entailed(p) (while undef for the remaining),
     * we cannot conclude that S fulfills c.
     * Counterexample: goal predicate x-y is undef, but x == 0 and y >= 1 are set.
     * Analogously: If, for non-sufficient entailments, no entailment is violated we cannot conclude that S fulfills c.
     *
     */

    /* Check whether entailments are sufficient, i.e, there does not exist state such that condition is false but entailments hold. */
    Z3_IN_PLAJA::SMTSolver solver_opt(model_pa.share_context(), config.get_sharable_ptr<PLAJA::StatsBase>(PLAJA::SharableKey::STATS));
    solver_opt.push();
    model_pa.add_src_bounds(solver_opt, true);
    solver_opt.add(!constraint_z3);
    for (const auto& [pred, value]: entailments->get_entailments()) { solver_opt.add(value ? model_pa.get_predicate_z3(pred, 0) : !model_pa.get_predicate_z3(pred, 0)); }
    entailmentsSufficient = not solver_opt.check_pop();
    // entailmentsSufficient = false;
    if (not PLAJA_GLOBAL::lazyPA and entailmentsSufficient) { solver = nullptr; }
}

StateConditionCheckerPa::~StateConditionCheckerPa() = default;

bool StateConditionCheckerPa::check(const AbstractState& abstract_state) const {
    // stateCondition->dump(true);
    // abstract_state.dump();
    lastCheckExact = true; // Check is exact as long as we do not call SMT.

    if (not stateCondition) { return false; }

    /* Explicit check. */
    if (not stateCondition->check_location_constraint(&abstract_state)) { return false; }

    if (not stateCondition->get_constraint()) { return true; }

#ifndef NDEBUG
    /* Expect non-empty abstract state. */
    solverDebug->push();
    entailments->get_model_pa().add_to(abstract_state, *solverDebug);
    PLAJA_ASSERT(solverDebug->check_pop());
#endif


    /* Abstract check. */

    bool match_entailments(true);

    for (const auto& [pred, value]: entailments->get_entailments()) {

        if constexpr (PLAJA_GLOBAL::lazyPA) { if (not abstract_state.is_set(pred)) { match_entailments = false; } }

        PLAJA_ASSERT(abstract_state.is_set(pred))

        if (abstract_state.predicate_value(pred) != value) { return false; }

    }

    PLAJA_ASSERT(PLAJA_GLOBAL::lazyPA or match_entailments)

    if (match_entailments and entailmentsSufficient) { return true; }

    lastCheckExact = false;

    PLAJA_ASSERT(solver)

    solver->push();
    // solver->dump_solver_to_file("before add.z3");
    entailments->get_model_pa().add_to(abstract_state, *solver);
    return solver->check_pop();
}

void StateConditionCheckerPa::increment() {
    if (not stateCondition or not stateCondition->get_constraint()) { return; }

    if (PLAJA_GLOBAL::lazyPA or not entailmentsSufficient) { // No need to compute additional entailments if known entailments are sufficient already.
        entailments->increment();
        solver->reset(); // If entailments suffice, solver is unused ...
    }

}