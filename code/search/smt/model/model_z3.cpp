//
// This file is part of the PlaJA code base.
// Copyright (c) (2019 - 2021) Marcel Vinzent.
// See README.md in the top-level directory for licensing information.
//

#include "model_z3.h"
#include "../../../exception/not_implemented_exception.h"
#include "../../../parser/ast/assignment.h"
#include "../../../parser/ast/automaton.h"
#include "../../../parser/ast/destination.h"
#include "../../../parser/ast/edge.h"
#include "../../../parser/ast/expression/lvalue_expression.h"
#include "../../../parser/ast/expression/non_standard/states_values_expression.h"
#include "../../../parser/ast/model.h"
#include "../../../parser/visitor/extern/to_normalform.h"
#include "../../../parser/visitor/extern/variables_of.h"
#include "../../factories/configuration.h"
#include "../../factories/smt_base/smt_options.h"
#include "../../information/jani2nnet/jani_2_nnet.h"
#include "../../information/model_information.h"
#include "../../predicate_abstraction/successor_generation/action_op/action_op_abstract.h"
#include "../../smt/successor_generation/successor_generator_to_z3.h"
#include "../../states/state_values.h"
#include "../constraint_z3.h"
#include "../context_z3.h"
#include "../nn/nn_in_z3.h"
#include "../nn/output_constraints_in_z3.h"
#include "../solver/smt_solver_z3.h"
#include "../utils/to_z3_expr.h"
#include "../vars_in_z3.h"
#include "../visitor/extern/to_z3_visitor.h"
#include "../visitor/smt_vars_of.h"
#include "model_z3_structures.h"
#include <z3++.h>

#if 0
namespace Z3_IN_PLAJA {

    const std::string z3LocPrefix{ "z3_loc_" }; // NOLINT(cert-err58-cpp)

    const std::string z3VarPostfix{ "_z3" }; // NOLINT(cert-err58-cpp)

    unsigned int z3AuxVarCounter = 0;

    const std::string z3SrcPostfix{ "_src_z3" }; // NOLINT(cert-err58-cpp)
    const std::string z3TargetPostfix{ "_target_z3" }; // NOLINT(cert-err58-cpp)

    const std::string z3NNVarPrefix{ "z3_nn_v_" }; // NOLINT(cert-err58-cpp)
    const std::string z3NNAuxVarPrefix{ "z3_nn_aux_v_" }; // NOLINT(cert-err58-cpp)

}
#endif

// external:
namespace SUCCESSOR_GENERATOR_ABSTRACT { [[nodiscard]] extern std::list<const ActionOpAbstract*> extract_ops_per_label(const SuccessorGeneratorAbstract& successor_generator, ActionLabel_type action_label); }

/**********************************************************************************************************************/


ModelZ3::ModelZ3(const PLAJA::Configuration& config):
    ModelSmt(std::shared_ptr<PLAJA::SmtContext>(new Z3_IN_PLAJA::Context()), config)
    , context(PLAJA_UTILS::cast_ptr<Z3_IN_PLAJA::Context>(get_context_ptr().get()))
    , nnSupport(has_nn() and (config.is_flag_set(PLAJA_OPTION::nn_support_z3) or config.is_flag_set(PLAJA_OPTION::nn_multi_step_support_z3)))
    , nnMultiStepSupport(nnSupport and config.is_flag_set(PLAJA_OPTION::nn_multi_step_support_z3))
    , successorGenerationToZ3(new SuccessorGeneratorToZ3(*this)) {

    set_start(std::move(start));
    set_reach(std::move(reach));

    /* Array constants? */
    PLAJA_ASSERT(context->get_number_of_variables() == 0)
    for (const ConstantIdType id: modelInfo->get_constants()) {
        const auto z3_var = Z3_IN_PLAJA::StateVarsInZ3::add(*model->get_constant(id), get_context());
        constants.emplace(id, z3_var);
    };

    generate_steps(maxStep);

    if (nnSupport) { outputConstraints = Z3_IN_PLAJA::OutputConstraints::construct(*this); }

}

ModelZ3::~ModelZ3() = default;

/**********************************************************************************************************************/

std::shared_ptr<Z3_IN_PLAJA::Context> ModelZ3::share_context() const { return PLAJA_UTILS::cast_shared<Z3_IN_PLAJA::Context>(get_context_ptr()); }

z3::context& ModelZ3::get_context_z3() const { return get_context()(); }

/**********************************************************************************************************************/

void ModelZ3::set_start(std::unique_ptr<Expression>&& start_ptr) {
    ModelSmt::set_start(std::move(start_ptr));
    startZ3 = nullptr;
    startZ3WithInit = nullptr;
}

void ModelZ3::set_reach(std::unique_ptr<Expression>&& reach_ptr) {
    ModelSmt::set_reach(std::move(reach_ptr));
    reachZ3.clear();
}

bool ModelZ3::initial_state_is_subsumed_by_start() const { return not startZ3WithInit; }

/**********************************************************************************************************************/

[[nodiscard]] std::unique_ptr<Z3_IN_PLAJA::StateVarsInZ3> ModelZ3::generate_state_variables(StepIndex_type step_index) const {

    std::unique_ptr<Z3_IN_PLAJA::StateVarsInZ3> state_vars(new Z3_IN_PLAJA::StateVarsInZ3(*this));

    const std::string postfix = PLAJA_UTILS::underscoreString + std::to_string(step_index);

    if (not ignore_locs()) {

        for (auto it_loc = modelInfo->locationIndexIterator(); !it_loc.end(); ++it_loc) { state_vars->add_loc(it_loc.state_index(), postfix); }

    } else if (nnSupport) {

        for (auto it_loc = modelInfo->locationIndexIterator(); !it_loc.end(); ++it_loc) {
            if (std::any_of(inputLocations.cbegin(), inputLocations.cend(), [&it_loc](VariableIndex_type loc_input) { return it_loc.state_index() == loc_input; })) {
                state_vars->add_loc(it_loc.state_index(), postfix);
            }
        }

    }

    for (auto it_var = model->variableIterator(); !it_var.end(); ++it_var) {
        state_vars->add(it_var.variable_id(), postfix);
        /*
         * Optimization: e.g. on racetrack the map information is encoded via a constant array variable,
         * by fixing the initial values as bounds, we omit the enumeration factor compared to encoding it via predicate.
         * Deprecated in favor of const-vars-to-consts on AST level.
         */
        // if (fixConstantsBounds and is_constant.is_constant(it_var.variable_id())) { state_vars->fix(it_var.variable_id()); }
    }

    return state_vars;
}

std::vector<Z3_IN_PLAJA::VarId_type> ModelZ3::extract_nn_inputs(const Z3_IN_PLAJA::StateVarsInZ3& state_vars) const {
    PLAJA_ASSERT(interface)
    auto nnInterface = dynamic_cast<const Jani2NNet*>(interface);
    PLAJA_ASSERT(nnInterface)
    PLAJA_ASSERT(nnSupport)

    std::vector<Z3_IN_PLAJA::VarId_type> nn_inputs;
    nn_inputs.reserve(nnInterface->get_num_input_features());

    const auto& model_info = *modelInfo;

    for (auto it = nnInterface->init_input_feature_iterator(); !it.end(); ++it) {
        const auto state_index = it.get_input_feature_index();

        if (model_info.is_location(state_index)) {

            PLAJA_ASSERT(state_vars.exists_loc(state_index))

            nn_inputs.push_back(state_vars.loc_to_z3(state_index));

        } else {

            const auto var_id = model_info.get_id(state_index);

            if (state_vars.exists(var_id)) {

                if (model_info.belongs_to_array(state_index)) {

                    const auto var_z3 = context->add_var(Z3_IN_PLAJA::bool_to_int_if(state_vars.var_to_z3_expr_index(state_index)));
                    nn_inputs.push_back(var_z3);

                } else {

                    const auto var_z3 = state_vars.to_z3(model_info.get_id(state_index));
                    const auto& var_expr = context->get_var(var_z3);

                    if (var_expr.is_bool()) {
                        const auto var_z3_fresh = context->add_var(Z3_IN_PLAJA::bool_to_int(var_expr));
                        nn_inputs.push_back(var_z3_fresh);
                    } else { nn_inputs.push_back(state_vars.to_z3(var_id)); }

                }

            } else { PLAJA_ABORT }
        }

    }
    return nn_inputs;
}

/**********************************************************************************************************************/

z3::expr ModelZ3::compute_assignment(const Assignment& assignment, const Z3_IN_PLAJA::StateVarsInZ3& src_vars, const Z3_IN_PLAJA::StateVarsInZ3& target_vars, std::unordered_map<VariableID_type, std::vector<z3::expr>>* updated_array_indexes, DestinationZ3* parent) const {
    PLAJA_ASSERT(assignment.get_index() == 0) // we do not support sequences

    // Cache assigned variable value.
    const VariableID_type var_id = assignment.get_updated_var_id();
    const auto* index = assignment.get_array_index();

    // Append parent.
    if (parent) { parent->assigned_ids.push_back(var_id); }

    ToZ3Expr var_ref(get_context()); // variable reference expression

    // check for array
    if (index) { // array assignment
        auto index_z3 = Z3_IN_PLAJA::to_z3(*index, src_vars);

        // Cache array index.
        if (updated_array_indexes) {
            auto it = updated_array_indexes->find(var_id);
            if (it == updated_array_indexes->end()) { updated_array_indexes->emplace(var_id, std::vector<z3::expr> { index_z3.primary }); }
            else { (*updated_array_indexes)[var_id].push_back(index_z3.primary); }
        }

        // Append parent.
        if (parent) {
            parent->array_indexes.push_back(std::make_unique<z3::expr>(var_ref.primary));
        }

        // bounds
        if (index->is_constant()) { // constant indexes are statistically checked to be within-bounds
            PLAJA_ASSERT(0 <= index->evaluate_integer_const() && index->evaluate_integer_const() <= get_array_size(var_id))
        } else { // add bounds on index without removing "inner bounds"
            index_z3.conjunct_auxiliary(0 <= index_z3.primary && index_z3.primary < context->int_val(PLAJA_UTILS::cast_numeric<PLAJA::integer>(get_array_size(var_id)))); // add bounds on index
        }

        // finally set var_ref.first to actual access expression:
        var_ref.set(z3::select(target_vars.to_z3_expr(var_id), index_z3.primary));
        if (index_z3.has_auxiliary()) { var_ref.conjunct_auxiliary(index_z3.release_auxiliary()); }

    } else { // non-array assignment

        var_ref.set(target_vars.to_z3_expr(var_id));

        // Append parent.
        if (parent) { parent->array_indexes.push_back(nullptr); }

    }

    // assigned value expression
    if (assignment.is_non_deterministic()) {

        PLAJA_ASSERT(assignment.get_lower_bound())
        PLAJA_ASSERT(assignment.get_upper_bound())

        auto var = var_ref.primary;

        ToZ3Expr lb_z3 = Z3_IN_PLAJA::to_z3(*assignment.get_lower_bound(), src_vars);
        var_ref.set(var >= lb_z3.release());
        if (lb_z3.has_auxiliary()) { var_ref.conjunct_auxiliary(lb_z3.release_auxiliary()); }

        ToZ3Expr ub_z3 = Z3_IN_PLAJA::to_z3(*assignment.get_upper_bound(), src_vars);
        var_ref.set(var >= ub_z3.release());
        if (ub_z3.has_auxiliary()) { var_ref.conjunct_auxiliary(ub_z3.release_auxiliary()); }

    } else {

        PLAJA_ASSERT(assignment.get_value())
        ToZ3Expr val_z3 = Z3_IN_PLAJA::to_z3(*assignment.get_value(), src_vars);

        var_ref.set(var_ref.primary == val_z3.primary);
        if (val_z3.has_auxiliary()) { var_ref.conjunct_auxiliary(val_z3.release_auxiliary()); }

    }

    return var_ref.to_conjunction();
}

std::unique_ptr<DestinationZ3> ModelZ3::compute_destination(const Destination& dest, const Z3_IN_PLAJA::StateVarsInZ3& src_vars, const Z3_IN_PLAJA::StateVarsInZ3& target_vars) const {
    PLAJA_ASSERT(dest.get_number_sequences() <= 1)
    std::unique_ptr<DestinationZ3> destination_z3(new DestinationZ3(dest));

    if (dest.get_number_sequences() == 0) { return destination_z3; } // 0 sequences, possible since destinations may only update location
    // else:
    auto num_assignments = dest.get_number_assignments_per_seq(0);

    destination_z3->assignments.reserve(num_assignments);
    destination_z3->assigned_ids.reserve(num_assignments);
    destination_z3->array_indexes.reserve(num_assignments);

    for (const auto* assignment: dest.get_assignments_per_seq(0)) {
        destination_z3->assignments.push_back(compute_assignment(*assignment, src_vars, target_vars, nullptr, destination_z3.get()));
    }

    return destination_z3;
}

std::unique_ptr<EdgeZ3> ModelZ3::compute_edge(const Edge& edge, const Z3_IN_PLAJA::StateVarsInZ3& src_vars, const Z3_IN_PLAJA::StateVarsInZ3& target_vars) const {
    std::unique_ptr<EdgeZ3> edge_z3(new EdgeZ3(edge, get_context()(), edge.get_guardExpression() ?
                                                                      std::make_unique<z3::expr>(Z3_IN_PLAJA::to_z3_condition(*edge.get_guardExpression(), src_vars)) :
                                                                      nullptr));
    // destinations to z3
    const std::size_t num_destinations = edge.get_number_destinations();
    edge_z3->destinations.reserve(num_destinations);
    for (std::size_t i = 0; i < num_destinations; ++i) {
        edge_z3->destinations.push_back(compute_destination(*edge.get_destination(i), src_vars, target_vars));
    }
    return edge_z3;
}

std::unique_ptr<AutomatonZ3> ModelZ3::compute_automaton(const Automaton& automaton, const Z3_IN_PLAJA::StateVarsInZ3& src_vars, const Z3_IN_PLAJA::StateVarsInZ3& target_vars) const {
    std::unique_ptr<AutomatonZ3> automaton_z3(new AutomatonZ3());
    const std::size_t num_edges = automaton.get_number_edges();
    automaton_z3->edges.reserve(num_edges);
    for (std::size_t i = 0; i < num_edges; ++i) {
        automaton_z3->edges.push_back(compute_edge(*automaton.get_edge(i), src_vars, target_vars));
    }
    return automaton_z3;
}

std::vector<std::unique_ptr<AutomatonZ3>> ModelZ3::compute_model(const Z3_IN_PLAJA::StateVarsInZ3& src_vars, const Z3_IN_PLAJA::StateVarsInZ3& target_vars) const {
    const auto& model = get_model();
    std::vector<std::unique_ptr<AutomatonZ3>> automata_z3;
    const std::size_t num_automata_instances = model.get_number_automataInstances();
    automata_z3.reserve(num_automata_instances);
    for (AutomatonIndex_type automaton_index = 0; automaton_index < num_automata_instances; ++automaton_index) {
        automata_z3.push_back(compute_automaton(*model.get_automatonInstance(automaton_index), src_vars, target_vars));
    }
    return automata_z3;
}

ToZ3Expr ModelZ3::to_z3(const Expression& expr, StepIndex_type step_index) const { return Z3_IN_PLAJA::to_z3(expr, get_state_vars(step_index)); }

z3::expr ModelZ3::to_z3_condition(const Expression& expr, StepIndex_type step_index) const { return Z3_IN_PLAJA::to_z3_condition(expr, get_state_vars(step_index)); }

void ModelZ3::add_expression(Z3_IN_PLAJA::SMTSolver& solver, const Expression& condition, StepIndex_type step_index) const { solver.add(to_z3_condition(condition, step_index)); }

/**********************************************************************************************************************/

// TODO might utilize z3::expr::substitute instead of recomputing structure for each step.
//  However, at least in terms of time consumption no issue, maybe beneficial to z3 heuristics or memory?
void ModelZ3::generate_steps(StepIndex_type max_step) {
    PLAJA_ASSERT(variablesPerStep.empty() or variablesPerStep.size() == get_max_num_step())
    PLAJA_ASSERT(nnInZ3PerStep.empty() or nnInZ3PerStep.size() == get_max_path_len())

    maxStep = std::max(max_step, maxStep);

    /* Vars (one additional "layer" to account for "target"). */

    variablesPerStep.reserve(get_max_num_step());

    for (auto it = iterate_steps(variablesPerStep.size()); !it.end(); ++it) {
        PLAJA_ASSERT(it.step() == variablesPerStep.size())
        variablesPerStep.push_back(generate_state_variables(it.step()));
    }

    PLAJA_ASSERT(variablesPerStep.size() == get_max_num_step())

    // nn:
    if (nnSupport) {
        PLAJA_ASSERT(interface)
        auto nnInterface = dynamic_cast<const Jani2NNet*>(interface);
        PLAJA_ASSERT(nnInterface)
        nnInZ3PerStep.reserve(nnMultiStepSupport ? get_max_path_len() : 1);

        for (auto it = iterate_path_from_to(nnInZ3PerStep.size(), nnMultiStepSupport ? get_max_path_len() : 1); !it.end(); ++it) {
            const std::string postfix = std::to_string(it.step());
            nnInZ3PerStep.push_back(std::make_unique<NNInZ3>(nnInterface->load_network(), *context, extract_nn_inputs(get_state_vars(it.step())), postfix));
        }

        PLAJA_ASSERT(nnInZ3PerStep.size() == get_max_path_len())

    }

    /* READ(!) : THE FOLLOWING CODE WAS COMMENTED OUT AS A WORKAROUND FOR CERTAIN BENCHMARKS. IF YOU NEED IT, PLEASE UNCOMMENT IT. */
    // start and reach:
    if (dynamic_cast<StatesValuesExpression*>(start.get()) == nullptr) {
        if (not startZ3) {
            if (start) {
                PLAJA_ASSERT(not startZ3WithInit)
                startZ3 = std::make_unique<z3::expr>(to_z3_condition(*start, 0)); // This line breaks if you are using a fixed start condition like in random_starts.jani files.
                if (not ModelSmt::initial_state_is_subsumed_by_start()) { // Only set if model's initial state is not subsumed.
                    startZ3WithInit = std::make_unique<z3::expr>(get_state_vars(0).encode_state(modelInfo->get_initial_values(), not ignore_locs()) || *startZ3);
                }
            } else if (not startZ3WithInit) {
                startZ3WithInit = std::make_unique<z3::expr>(get_state_vars(0).encode_state(modelInfo->get_initial_values(), not ignore_locs()));
            }
        }
    }

    /* END OF COMMENTED OUT CODE. */

    if (reach) {
        reachZ3.reserve(get_max_num_step());
        for (auto it = iterate_steps(reachZ3.size()); !it.end(); ++it) { reachZ3.push_back(std::make_unique<z3::expr>(to_z3_condition(*reach, it.step()))); }
    }
    PLAJA_ASSERT((not reach and reachZ3.empty()) or reachZ3.size() == get_max_num_step())

#ifdef ENABLE_TERMINAL_STATE_SUPPORT
    const auto* non_terminal = get_non_terminal();
    if (non_terminal) {
        nonTerminalSmt.reserve(get_max_num_step());
        for (auto it = iterate_steps(nonTerminalSmt.size()); !it.end(); ++it) { nonTerminalSmt.push_back(std::make_unique<Z3_IN_PLAJA::ConstraintExpr>(to_z3_condition(*non_terminal, it.step()))); }
    }
    PLAJA_ASSERT((not non_terminal and nonTerminalSmt.empty()) or nonTerminalSmt.size() == get_max_num_step())
#endif

    // generate steps for action ops if already present -> *not* during construction
    // if (not actionOps.empty()) { for (auto& id_op: actionOps) { id_op.second->generate_steps(); } }

    if (outputConstraints) { outputConstraints->generate_steps(); }

}

const z3::expr& ModelZ3::get_var(VariableID_type var, StepIndex_type step) const { return get_state_vars(step).to_z3_expr(var); }

void ModelZ3::register_constant(Z3_IN_PLAJA::SMTSolver& solver, ConstantIdType constant) const {
    const auto constant_z3 = get_constant_var(constant);
    solver.register_bound_if(constant_z3, context->get_bound(constant_z3));
}

void ModelZ3::register_bound(Z3_IN_PLAJA::SMTSolver& solver, VariableID_type var, StepIndex_type step) const { // NOLINT(bugprone-easily-swappable-parameters)
    const auto& state_vars = get_state_vars(step);
    const auto var_z3 = state_vars.to_z3(var);
    const auto* bound = context->get_bound_if(var_z3);
    if (not bound) { return; }
    solver.register_bound_if(var_z3, *bound);
}

void ModelZ3::register_bounds(Z3_IN_PLAJA::SMTSolver& solver, StepIndex_type step, bool ignore_locs) const {

    /* Always register constants */
    for (const auto constant: constants) {
        const auto* bound = context->get_bound_if(constant.second);
        if (bound) { solver.register_bound_if(constant.second, *bound); }
    }

    const auto state_vars = get_state_vars(step);

    if (not ignore_locs and not ignoreLocations) {
        for (auto it_loc = state_vars.locIterator(); !it_loc.end(); ++it_loc) {
            solver.register_bound_if(it_loc.loc_z3(), context->get_bound(it_loc.loc_z3()));
        }
    }

    for (auto it_var = state_vars.varIterator(); !it_var.end(); ++it_var) {
        const auto var_z3 = it_var.var_z3();
        const auto* bound = context->get_bound_if(var_z3);
        if (bound) { solver.register_bound_if(var_z3, *bound); }
    }

}

void ModelZ3::register_bounds(Z3_IN_PLAJA::SMTSolver& solver, const Expression& expr, StepIndex_type step) const {
    for (const auto var: Z3_IN_PLAJA::collect_smt_vars(expr, get_state_vars(step))) {
        const auto* bound = context->get_bound_if(var);
        if (bound) { solver.register_bound_if(var, *bound); }
    }
}

void ModelZ3::add_start(Z3_IN_PLAJA::SMTSolver& solver, bool include_init) const {
    PLAJA_ASSERT(startZ3WithInit or ModelSmt::initial_state_is_subsumed_by_start())
    if (include_init) { solver.add(initial_state_is_subsumed_by_start() ? *startZ3 : *startZ3WithInit); }
    else if (start) {
        PLAJA_ASSERT(startZ3)
        solver.add(*startZ3);
    }
}

void ModelZ3::add_init(Z3_IN_PLAJA::SMTSolver& solver, StepIndex_type step) const { solver.add(get_state_vars(step).encode_state(modelInfo->get_initial_values(), not ignore_locs())); }

void ModelZ3::exclude_start(Z3_IN_PLAJA::SMTSolver& solver, bool include_init, StepIndex_type step) const {

    if (step == 0) {
        if (include_init) { solver.add(!(initial_state_is_subsumed_by_start() ? *startZ3 : *startZ3WithInit)); }
        else {
            if (start) { solver.add(!*startZ3); }
            else { solver.add(context->bool_val(false)); }
        }
    }

    if (include_init) {
        solver.add(!get_state_vars(0).encode_state(modelInfo->get_initial_values(), not ignore_locs()));
    }

    if (start) { solver.add(!to_z3_condition(*start, step)); }
    else if (not include_init) { solver.add(context->bool_val(false)); }

}

void ModelZ3::add_reach(Z3_IN_PLAJA::SMTSolver& solver, StepIndex_type step) const {
    if (not reach) { solver.add(context->bool_val(false)); }
    PLAJA_ASSERT(step < reachZ3.size())
    solver.add(*reachZ3[step]);
}

void ModelZ3::exclude_reach(Z3_IN_PLAJA::SMTSolver& solver, StepIndex_type step) const {
    if (reach) {
        PLAJA_ASSERT(step < reachZ3.size())
        solver.add(!*reachZ3[step]);
    }
}

void ModelZ3::add_nn(Z3_IN_PLAJA::SMTSolver& solver, StepIndex_type step) const {
    PLAJA_ASSERT(step < nnInZ3PerStep.size())
    nnInZ3PerStep[step]->add(solver);
}

void ModelZ3::add_output_interface(Z3_IN_PLAJA::SMTSolver& solver, ActionLabel_type action_label, StepIndex_type step) const { // NOLINT(bugprone-easily-swappable-parameters)
    PLAJA_ASSERT(interface)
    outputConstraints->add(solver, action_label, step);
}

void ModelZ3::add_output_interface_for_op(Z3_IN_PLAJA::SMTSolver& solver, ActionOpID_type action_op, StepIndex_type step) const {
    if constexpr (PLAJA_GLOBAL::enableApplicabilityCache) { set_self_applicability(true); }
    add_output_interface(solver, successorGenerationToZ3->get_action_op_structure(action_op)._action_label(), step);
    if constexpr (PLAJA_GLOBAL::enableApplicabilityCache) { unset_self_applicability(); }
}

#ifdef BUILD_PA

void ModelZ3::share_action_ops(const SuccessorGeneratorAbstract& successor_generator) {
    if (not interface) { return; } // so far only needed in NN-Sat context

    // TODO cache action ops in z3 (preliminary quick fix)
    for (auto output_it = interface->init_output_feature_iterator(); !output_it.end(); ++output_it) {

        auto& action_ops_for_label = actionOpsPerLabel.emplace(output_it.get_output_label(), SUCCESSOR_GENERATOR_ABSTRACT::extract_ops_per_label(successor_generator, output_it.get_output_label())).first->second;
        for (const auto* action_op_z3: action_ops_for_label) {
            actionOps.emplace(action_op_z3->_op_id(), action_op_z3);
        }

    }
}

#endif

/* */

const ActionOpToZ3& ModelZ3::get_action_op_structure(ActionOpID_type op) const { return successorGenerationToZ3->get_action_op_structure(op); }

void ModelZ3::add_guard(Z3_IN_PLAJA::SMTSolver& solver, ActionOpID_type op, bool do_locs, StepIndex_type step) const {
    const auto& action_op = successorGenerationToZ3->get_action_op_structure(op);
    action_op.guard_to_z3(get_state_vars(step), do_locs)->add_to_solver(solver);
}

void ModelZ3::add_update(Z3_IN_PLAJA::SMTSolver& solver, ActionOpID_type op, UpdateIndex_type update_index, bool do_locs, bool do_copy, StepIndex_type step) const {
    const auto& action_op = successorGenerationToZ3->get_action_op_structure(op);
    action_op.update_to_z3(update_index, get_state_vars(step), get_state_vars(step + 1), do_locs, do_copy)->add_to_solver(solver);
}

void ModelZ3::add_action_op(Z3_IN_PLAJA::SMTSolver& solver, ActionOpID_type op, UpdateIndex_type update_index, bool do_locs, bool do_copy, StepIndex_type step) const {
    const auto& action_op = successorGenerationToZ3->get_action_op_structure(op);
    action_op.to_z3(update_index, get_state_vars(step), get_state_vars(step + 1), do_locs, do_copy)->add_to_solver(solver);
}

std::unique_ptr<Z3_IN_PLAJA::Constraint> ModelZ3::guards_to_smt(ActionLabel_type action_label, StepIndex_type step) const { return successorGenerationToZ3->generate_guards(action_label, step); }

void ModelZ3::add_guard_disjunction(Z3_IN_PLAJA::SMTSolver& solver, ActionLabel_type action_label, StepIndex_type step) const { guards_to_smt(action_label, step)->add_to_solver(solver); }

void ModelZ3::add_choice_point(Z3_IN_PLAJA::SMTSolver& solver, StepIndex_type step, bool nn_aware) const {
    if (nn_aware) {

        if constexpr (PLAJA_GLOBAL::enableApplicabilityCache) { add_self_applicability(true); }

        const auto& src_vars = get_state_vars(step);
        const auto& target_vars = get_state_vars(step + 1);

        const auto* nn_interface = dynamic_cast<const Jani2NNet*>(interface);
        PLAJA_ASSERT(nn_interface)
        auto it = nn_interface->init_output_feature_iterator();
        z3::expr choice_point = outputConstraints->compute(it.get_output_label(), step)->move_to_expr() && successorGenerationToZ3->generate_action(it.get_output_label(), src_vars, target_vars)->move_to_expr();
        for (++it; !it.end(); ++it) {
            choice_point = z3::ite(outputConstraints->compute(it.get_output_label(), step)->move_to_expr(),
                                   successorGenerationToZ3->generate_action(it.get_output_label(), src_vars, target_vars)->move_to_expr(),
                                   choice_point);
        }

        for (const ActionLabel_type unlearned_label: nn_interface->get_unlearned_labels()) {
            const auto& action_structure = successorGenerationToZ3->get_action_structure(unlearned_label);
            if (not action_structure.empty()) { choice_point = choice_point || successorGenerationToZ3->generate_action(action_structure, src_vars, target_vars)->move_to_expr(); }
        }

        solver.add(choice_point);

        if constexpr (PLAJA_GLOBAL::enableApplicabilityCache) { unset_self_applicability(); }

    } else {
        successorGenerationToZ3->generate_expansion(get_state_vars(step), get_state_vars(step + 1))->add_to_solver(solver);
    }
}

void ModelZ3::add_loop_exclusion(Z3_IN_PLAJA::SMTSolver& solver, StepIndex_type step_index) const {
    if (step_index < 1) { return; }

    for (StepIndex_type step_index_1 = 0; step_index_1 <= step_index; ++step_index_1) {

        const auto& variables_per_step_1 = get_state_vars(step_index_1);

        for (StepIndex_type step_index_2 = 0; step_index_2 < step_index_1; ++step_index_2) {

            const auto& variables_per_step_2 = get_state_vars(step_index_2);

            std::vector<z3::expr> state_disjunction;
            state_disjunction.reserve(variables_per_step_1.size());

            // locations
            for (auto it_loc = variables_per_step_1.locIterator(); !it_loc.end(); ++it_loc) {
                state_disjunction.push_back(it_loc.loc_z3_expr() != variables_per_step_2.loc_to_z3_expr(it_loc.loc()));
            }

            for (auto it_var = variables_per_step_1.varIterator(); !it_var.end(); ++it_var) {
                state_disjunction.push_back(it_var.var_z3_expr() != variables_per_step_2.to_z3_expr(it_var.var()));
            }

            solver.add(Z3_IN_PLAJA::to_disjunction(state_disjunction));
        }

    }
}

/**********************************************************************************************************************/

std::unique_ptr<PLAJA::SmtSolver> ModelZ3::init_solver(const PLAJA::Configuration& config, StepIndex_type step) const {
    auto solver = std::make_unique<Z3_IN_PLAJA::SMTSolver>(share_context(), config.get_sharable_ptr<PLAJA::StatsBase>(PLAJA::SharableKey::STATS));
    if (step == SEARCH::noneStep) { return solver; }
    for (auto it = iterate_steps(step); !it.end(); ++it) { add_bounds(*solver, it.step(), ignore_locs()); }
    return solver;
}

std::unique_ptr<PLAJA::SmtConstraint> ModelZ3::to_smt(const Expression& expr, StepIndex_type step) const { return std::make_unique<Z3_IN_PLAJA::ConstraintExpr>(to_z3_condition(expr, step)); }

void ModelZ3::add_to_solver(PLAJA::SmtSolver& solver, const Expression& expr, StepIndex_type step) const { add_expression(PLAJA_UTILS::cast_ref<Z3_IN_PLAJA::SMTSolver>(solver), expr, step); }
