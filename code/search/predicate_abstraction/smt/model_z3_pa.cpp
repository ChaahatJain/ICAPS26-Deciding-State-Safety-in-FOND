//
// This file is part of the PlaJA code base.
// Copyright (c) (2019 - 2020) Marcel Vinzent.
// See README.md in the top-level directory for licensing information.
//

#include "model_z3_pa.h"
#include "../../../parser/ast/expression/non_standard/predicates_expression.h"
#include "../../factories/configuration.h"
#include "../../information/model_information.h"
#include "../../smt/solver/smt_solver_z3.h"
#include "../../smt/visitor/extern/to_z3_visitor.h"
#include "../../smt/context_z3.h"
#include "../../smt/vars_in_z3.h"
#include "../pa_states/abstract_state.h"
#include "../pa_states/pa_state_values.h"
#include "../successor_generation/action_op/action_op_pa.h"

ModelZ3PA::ModelZ3PA(const PLAJA::Configuration& config, const PredicatesExpression* predicates_):
    ModelZ3(config)
    , paInitialState(nullptr) {

    // PLAJA_ASSERT(not do_locs())
    // PLAJA_ASSERT(not use_copy_assignments())

    if (predicates_) {
        PLAJA_ASSERT(not _predicates())
        predicates = predicates_;
    }

    PLAJA_ASSERT(_predicates())

    // predicate dependent
    step_predicates(true);
    compute_abstract_model_info(true);

}

ModelZ3PA::~ModelZ3PA() = default;

// Predicate-dependent routines:

void ModelZ3PA::step_predicates(bool reset) {

    const auto num_preds = predicates->get_number_predicates();
    predicatesInZ3PerStep.reserve(get_max_num_step());

    for (auto it = iterate_steps(0); !it.end(); ++it) {
        const auto step = it.step();

        if (step >= predicatesInZ3PerStep.size()) { predicatesInZ3PerStep.emplace_back(); }
        auto& predicates_in_z3 = predicatesInZ3PerStep[step];

        if (reset) { predicates_in_z3.clear(); }

        PLAJA_ASSERT(predicates_in_z3.size() <= num_preds) // Incremental usage: number of predicates should only increase  (except for empty initialization or when already incremented).

        predicates_in_z3.reserve(num_preds);

        const auto& state_vars = get_state_vars(step);
        for (PredicateIndex_type index = predicates_in_z3.size(); index < num_preds; ++index) { // incremental usage: continue after existing bounds
            const auto* predicate = predicates->get_predicate(index);
            predicates_in_z3.emplace_back(std::make_unique<z3::expr>(Z3_IN_PLAJA::to_z3_condition(*predicate, state_vars)));
        }

    }

}

void ModelZ3PA::compute_abstract_model_info(bool reset) {
    if (reset) { // in case of reuse:
        lowerBounds.clear();
        upperBounds.clear();
        ranges.clear();
    }

    const auto& model_initial_values = get_model_info().get_initial_values();

    const std::size_t automata_offset = get_model_info().get_automata_offset();
    const std::size_t l = automata_offset + predicates->get_number_predicates();
    PLAJA_ASSERT(lowerBounds.size() <= l) // incremental usage: number of predicates should only increase (or be equal when already incremented)

    paInitialState = compute_abstract_state_values(model_initial_values); // (first) model initial state
    lowerBounds.reserve(l);
    upperBounds.reserve(l);
    ranges.reserve(l);

    // automata locations ...
    const auto& model_info = get_model_info();
    std::size_t i = lowerBounds.size(); // incremental usage: continue after existing bounds
    PLAJA_ASSERT(i == 0 or i >= automata_offset) // i.e., all location bounds should have been set
    for (; i < automata_offset; ++i) {
        lowerBounds.push_back(model_info.get_lower_bound_int(i));
        upperBounds.push_back(model_info.get_upper_bound_int(i));
        ranges.push_back(model_info.get_range(i));
    }
    // bounds and ranges ...
    for (; i < l; ++i) {
        lowerBounds.push_back(0);
        upperBounds.push_back(1);
        ranges.push_back(2);
    }

}

void ModelZ3PA::generate_steps(StepIndex_type max_step) {
    ModelZ3::generate_steps(max_step);
    step_predicates(false);
}

void ModelZ3PA::increment() {
    PLAJA_ASSERT(_src_predicates().size() <= predicates->get_number_predicates()) // number of predicates should have increased (or already being incremented to current count)

    step_predicates(false);
    compute_abstract_model_info(false);
}

//

std::unique_ptr<PaStateValues> ModelZ3PA::compute_abstract_state_values(const StateValues& concrete_state) const {
    auto pa_state_values = std::make_unique<PaStateValues>(get_number_predicates(), get_number_automata_instances(), predicates);

    /* Locations. */
    for (auto it_loc = get_model_info().locationIndexIterator(); !it_loc.end(); ++it_loc) { pa_state_values->set_location(it_loc.state_index(), concrete_state.get_int(it_loc.state_index())); }

    /* Predicates. */
    PredicateIndex_type pred_index(0);
    for (auto it = predicates->predicatesIterator(); !it.end(); ++it) { pa_state_values->set_predicate_value(pred_index++, it->evaluateInteger(&concrete_state)); }

    return pa_state_values;
}

//

const ActionOpPA& ModelZ3PA::get_action_op_pa(ActionOpID_type op_id) const { return PLAJA_UTILS::cast_ref<ActionOpPA>(get_action_op(op_id)); }

//

void ModelZ3PA::add_to(const AbstractState& abstract_state, Z3_IN_PLAJA::SMTSolver& solver, StepIndex_type step) const {

    // locations:
    const auto& vars_z3 = get_state_vars(step);
    for (const VariableIndex_type state_index: get_input_locations()) {
        solver.add(vars_z3.loc_to_z3_expr(state_index) == context->int_val(abstract_state.location_value(state_index)));
    }

    // predicates:
    const auto& predicates_in_z3 = get_predicates_z3(step);
    for (auto pred_it = abstract_state.init_pred_state_iterator(); !pred_it.end(); ++pred_it) {
        PLAJA_ASSERT(PLAJA_GLOBAL::lazyPA or pred_it.is_set())
        if constexpr (PLAJA_GLOBAL::lazyPA) { if (not pred_it.is_set()) { continue; } }
        if (pred_it.predicate_value()) { solver.add(*predicates_in_z3[pred_it.predicate_index()]); }
        else { solver.add(!*predicates_in_z3[pred_it.predicate_index()]); }
    }

}

//

void MODEL_Z3_PA::make_sharable(const PLAJA::Configuration& config) {
    PLAJA_ASSERT(not config.has_sharable(PLAJA::SharableKey::MODEL_Z3))
    config.set_sharable(PLAJA::SharableKey::MODEL_Z3, std::shared_ptr<ModelZ3>(new ModelZ3PA(config)));
}
