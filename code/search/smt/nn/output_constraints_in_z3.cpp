//
// This file is part of the PlaJA code base.
// Copyright (c) (2019 - 2023) Marcel Vinzent.
// See README.md in the top-level directory for licensing information.
//

#include "output_constraints_in_z3.h"
#include "../../information/jani2nnet/jani_2_nnet.h"
#include "../model/model_z3.h"
#include "../solver/smt_solver_z3.h"
#include "../visitor/extern/to_z3_visitor.h"
#include "../constraint_z3.h"
#include "nn_in_z3.h"

#ifdef ENABLE_APPLICABILITY_FILTERING

#include "../successor_generation/op_applicability.h"

#endif

Z3_IN_PLAJA::OutputConstraints::OutputConstraints(const ModelZ3& model):
    model(&model) {
}

Z3_IN_PLAJA::OutputConstraints::~OutputConstraints() = default;

/* */

inline const Jani2NNet& Z3_IN_PLAJA::OutputConstraints::get_interface() const {
    PLAJA_ASSERT(get_model().get_interface())
    return *dynamic_cast<const Jani2NNet*>(get_model().get_interface());
}

inline Z3_IN_PLAJA::Context& Z3_IN_PLAJA::OutputConstraints::get_context() const { return get_model().get_context(); }

const NNInZ3& Z3_IN_PLAJA::OutputConstraints::get_nn_in_smt(StepIndex_type step) const {
    PLAJA_ASSERT(get_context() == model->get_nn_in_smt(step)._context())
    return model->get_nn_in_smt(step);
}

z3::expr Z3_IN_PLAJA::OutputConstraints::compute_eq(OutputIndex_type output_index, const z3::expr& output_var, OutputIndex_type other_output_index, const z3::expr& other_output_var) {
    PLAJA_ASSERT(output_index != other_output_index)
    return other_output_index < output_index ? output_var > other_output_var : output_var >= other_output_var;
}

/* */

#ifdef ENABLE_APPLICABILITY_FILTERING

std::unique_ptr<Z3_IN_PLAJA::OutputConstraints> Z3_IN_PLAJA::OutputConstraints::construct(const ModelZ3& model) {
    if (model.get_interface()->_do_applicability_filtering()) { return std::make_unique<Z3_IN_PLAJA::OutputConstraintsAppFilter>(model); }
    else { return std::make_unique<Z3_IN_PLAJA::OutputConstraintsNoFilter>(model); }
    PLAJA_ABORT;
}

#else

std::unique_ptr<Z3_IN_PLAJA::OutputConstraintsNoFilter> Z3_IN_PLAJA::OutputConstraints::construct(const ModelZ3& model) {
    return std::make_unique<Z3_IN_PLAJA::OutputConstraintsNoFilter>(model);
}

#endif

/**********************************************************************************************************************/


Z3_IN_PLAJA::OutputConstraintsNoFilter::OutputConstraintsNoFilter(const ModelZ3& model):
    OutputConstraints(model) {

    // Cache constraints for step=0.

    auto output_layer_size = get_nn_in_smt(0).get_output_layer_size();
    constraintsPerLabel.reserve(output_layer_size);

    for (OutputIndex_type output_index = 0; output_index < output_layer_size; ++output_index) {
        constraintsPerLabel.push_back(std::make_unique<z3::expr>(compute_internal(output_index, 0)));
    }

}

Z3_IN_PLAJA::OutputConstraintsNoFilter::~OutputConstraintsNoFilter() = default;

z3::expr Z3_IN_PLAJA::OutputConstraintsNoFilter::compute_internal(OutputIndex_type output_index, StepIndex_type step) const { // NOLINT(*-easily-swappable-parameters)
    const auto& nn_in_smt = get_nn_in_smt(step);
    const auto& output_var = nn_in_smt.get_output_var_z3(output_index);

    std::vector<z3::expr> selection_constraint;

    const auto num_outputs = nn_in_smt.get_output_layer_size();
    PLAJA_ASSERT(get_interface().get_num_output_features() == num_outputs)
    selection_constraint.reserve(num_outputs - 1);

    OutputIndex_type other_output = 0;
    for (; other_output < output_index; ++other_output) { selection_constraint.push_back(output_var > nn_in_smt.get_output_var_z3(other_output)); }
    for (++other_output; other_output < num_outputs; ++other_output) { selection_constraint.push_back(output_var >= nn_in_smt.get_output_var_z3(other_output)); }

    return Z3_IN_PLAJA::to_conjunction(selection_constraint);
}

std::unique_ptr<Z3_IN_PLAJA::ConstraintExpr> Z3_IN_PLAJA::OutputConstraintsNoFilter::compute(ActionLabel_type action_label, StepIndex_type step) const { // NOLINT(*-easily-swappable-parameters)
    PLAJA_ASSERT(get_interface().is_learned(action_label))
    const auto output_index = get_interface().get_output_index(action_label);
    PLAJA_ASSERT(output_index < constraintsPerLabel.size())

    return std::make_unique<Z3_IN_PLAJA::ConstraintExpr>(step > 0 ? compute_internal(output_index, step) : *constraintsPerLabel[output_index]);
}

void Z3_IN_PLAJA::OutputConstraintsNoFilter::add(Z3_IN_PLAJA::SMTSolver& solver, ActionLabel_type action_label, StepIndex_type step) const { // NOLINT(*-easily-swappable-parameters)
    PLAJA_ASSERT(get_interface().is_learned(action_label))
    const auto output_index = get_interface().get_output_index(action_label);
    PLAJA_ASSERT(output_index < constraintsPerLabel.size())

    if (step > 0) { solver.add(compute_internal(output_index, step)); }
    else { solver.add(*constraintsPerLabel[output_index]); }
}


/**********************************************************************************************************************/


#ifdef ENABLE_APPLICABILITY_FILTERING

// Applicability filtering: output_var >= output_var' or ( not g_1(var') and ... not g_n(var') )
// where g_i(var') is the guard of a respectively labeled operator.

Z3_IN_PLAJA::OutputConstraintsAppFilter::OutputConstraintsAppFilter(const ModelZ3& model_marabou):
    OutputConstraints(model_marabou)
    , opApp(nullptr) {
    PLAJA_ASSERT(&model_marabou.get_successor_generator())
}

Z3_IN_PLAJA::OutputConstraintsAppFilter::~OutputConstraintsAppFilter() = default;

std::unique_ptr<Z3_IN_PLAJA::ConstraintExpr> Z3_IN_PLAJA::OutputConstraintsAppFilter::compute(ActionLabel_type action_label, StepIndex_type step) const { // NOLINT(*-easily-swappable-parameters)
    if constexpr (PLAJA_GLOBAL::enableApplicabilityCache) { opApp = get_model().get_op_applicability(); }

    const auto& nn_interface = get_interface();
    // const auto do_locs = not get_model().ignore_locs();

    const auto output_index = nn_interface.get_output_index(action_label);
    PLAJA_ASSERT(output_index < get_nn_in_smt(step).get_output_layer_size())

    const auto& nn_in_smt = get_nn_in_smt(step);
    const auto& output_var = nn_in_smt.get_output_var_z3(output_index);
    const auto output_layer_size = nn_in_smt.get_output_layer_size();

    auto& context = get_context();

    z3::expr selection_constraint(context());

    // Add guard disjunction:

    if (not(opApp and opApp->known_self_applicability())) { selection_constraint = get_model().guards_to_smt(action_label, step)->move_to_expr(); }
    else { PLAJA_ASSERT(opApp->get_self_applicability()) }

    // Add output constraint:

    for (OutputIndex_type neuron_index = 0; neuron_index < output_layer_size; ++neuron_index) {
        if (output_index == neuron_index) { continue; }

        auto other_label_guards = get_model().guards_to_smt(nn_interface.get_output_label(neuron_index), step);

        if (other_label_guards->is_trivially_false()) { continue; }

        if (other_label_guards->is_trivially_true()) {
            Z3_EXPR_ADD(selection_constraint, compute_eq(output_index, output_var, neuron_index, nn_in_smt.get_output_var_z3(neuron_index)), &&)
        } else {
            Z3_EXPR_ADD(selection_constraint, (compute_eq(output_index, output_var, neuron_index, nn_in_smt.get_output_var_z3(neuron_index)) || !get_model().guards_to_smt(nn_interface.get_output_label(neuron_index), step)->move_to_expr()), &&);
        }

    }

    if constexpr (PLAJA_GLOBAL::enableApplicabilityCache) { opApp = nullptr; }
    return std::make_unique<Z3_IN_PLAJA::ConstraintExpr>(selection_constraint);

}

void Z3_IN_PLAJA::OutputConstraintsAppFilter::add(Z3_IN_PLAJA::SMTSolver& solver, ActionLabel_type action_label, StepIndex_type step) const { // NOLINT(*-easily-swappable-parameters)
    compute(action_label, step)->add_to_solver(solver);
}

#endif
