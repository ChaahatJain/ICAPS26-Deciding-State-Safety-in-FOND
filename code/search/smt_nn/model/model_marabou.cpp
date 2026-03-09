//
// This file is part of the PlaJA code base.
// Copyright (c) (2019 - 2022) Marcel Vinzent.
// See README.md in the top-level directory for licensing information.
//

#include "model_marabou.h"
#include "../../../include/marabou_include/infeasible_query_exception.h"
#include "../../../parser/ast/expression/expression.h"
#include "../../../parser/visitor/extern/to_normalform.h"
#include "../../../utils/utils.h"
#include "../../factories/configuration.h"
#include "../../information/input_feature_to_jani.h"
#include "../../information/jani2nnet/jani_2_nnet.h"
#include "../../information/model_information.h"
#include "../../smt/successor_generation/op_applicability.h"
#include "../solver/smt_solver_marabou.h"
#include "../successor_generation/successor_generator_marabou.h"
#include "../visitor/extern/to_marabou_visitor.h"
#include "../constraints_in_marabou.h"
#include "../nn_in_marabou.h"
#include "../output_constraints_in_marabou.h"

// extern:
namespace PLAJA_OPTION::SMT_SOLVER_MARABOU { extern const std::unordered_map<std::string, MILPSolverBoundTighteningType> stringToBoundTightening; }

/**********************************************************************************************************************/

ModelMarabou::ModelMarabou(const PLAJA::Configuration& config, bool init_suc_gen):
    ModelSmt(std::shared_ptr<PLAJA::SmtContext>(new MARABOU_IN_PLAJA::Context()), config)
    , context(PLAJA_UTILS::cast_ptr<MARABOU_IN_PLAJA::Context>(get_context_ptr().get()))
    , nnBounds(config.get_option(PLAJA_OPTION::SMT_SOLVER_MARABOU::stringToBoundTightening, PLAJA_OPTION::nn_tightening))
    , nnBoundsPerLabel(config.get_option(PLAJA_OPTION::SMT_SOLVER_MARABOU::stringToBoundTightening, PLAJA_OPTION::nn_tightening_per_label)) {

    // PLAJA_ASSERT_EXPECT(nnInterface and has_nn()) // Seems to work without NN, though not exhaustively tested. To be used for sanity checks at development time anyway ...

    auto start_tmp = std::move(start); // Cache start.
    set_start(nullptr);
    set_reach(std::move(reach));
    STMT_IF_TERMINAL_STATE_SUPPORT(set_non_terminal(std::move(nonTerminal), false);)

    generate_steps(1); // Generate up to start.

    set_start(std::move(start_tmp)); // Reset start.

    if (start) { // Optimize start.
        std::unique_ptr<MARABOU_IN_PLAJA::SMTSolver> solver = MARABOU_IN_PLAJA::SMT_SOLVER::construct(config, make_query(false, false, 0));
        start = optimize_expression(*start, *solver, true, false, false, false);
        startMarabou = nullptr;
    }

    generate_steps(1);

    // bound tightening

    if (nnBounds != MILPSolverBoundTighteningType::NONE) {
        auto query = make_query(true, false, 0);

        switch (nnBounds) {

            case MILPSolverBoundTighteningType::LP_RELAXATION: {
                lp_tightening(*query, MILPSolverBoundTighteningType::LP_RELAXATION);
                break;
            }
            case MILPSolverBoundTighteningType::LP_RELAXATION_INCREMENTAL: {
                lp_tightening(*query, MILPSolverBoundTighteningType::LP_RELAXATION_INCREMENTAL);
                break;
            }
            case MILPSolverBoundTighteningType::MILP_ENCODING: {
                lp_tightening(*query, MILPSolverBoundTighteningType::LP_RELAXATION);
                lp_tightening(*query, MILPSolverBoundTighteningType::MILP_ENCODING);
                break;
            }
            case MILPSolverBoundTighteningType::MILP_ENCODING_INCREMENTAL: {
                lp_tightening(*query, MILPSolverBoundTighteningType::LP_RELAXATION_INCREMENTAL);
                lp_tightening(*query, MILPSolverBoundTighteningType::MILP_ENCODING_INCREMENTAL);
                break;
            }
            default: { PLAJA_ABORT }
        }

        // store bounds in context
        for (auto it = query->variableIterator(); !it.end(); ++it) { context->tighten_bounds(it.var(), it.lower_bound(), it.upper_bound()); }
        // context->dump();

        // layer.dumpSymbolics();

    }

    if (init_suc_gen) {
        successorGenerator = std::make_unique<MARABOU_IN_PLAJA::SuccessorGenerator>(get_successor_generator(), *this, true);
        init_output_constraints();
    }

}

ModelMarabou::~ModelMarabou() = default;

void ModelMarabou::init_output_constraints() {

    if (not has_nn()) { return; }

    PLAJA_ASSERT_EXPECT(successorGenerator) // Needed for applicability filtering.
    outputConstraints = MARABOU_IN_PLAJA::OutputConstraints::construct(*this); // Need successor generator.

    if (nnBoundsPerLabel == MILPSolverBoundTighteningType::NONE) { return; }

    // Compute bounds per label.

    for (auto it_output = interface->init_output_feature_iterator(); !it_output.end(); ++it_output) {

        auto query = make_query(true, false, 0);

        PLAJA_ASSERT(outputConstraints) // Need successor generator.
        outputConstraints->add(*query, it_output._output_index(), 0);

        lp_tightening_output(*query, nnBoundsPerLabel);

        tighteningsPerLabel.emplace_back();
        auto& tightenings = tighteningsPerLabel.back();

        for (auto it = query->variableIterator(); !it.end(); ++it) {

            if (it.lower_bound() > context->get_lower_bound(it.var())) {
                tightenings.push_back(std::make_unique<Tightening>(it.var(), it.lower_bound(), Tightening::LB));
            }

            if (it.upper_bound() > context->get_upper_bound(it.var())) {
                tightenings.push_back(std::make_unique<Tightening>(it.var(), it.upper_bound(), Tightening::UB));
            }

        }

    }

}

/**********************************************************************************************************************/

std::shared_ptr<MARABOU_IN_PLAJA::Context> ModelMarabou::share_context() const { return PLAJA_UTILS::cast_shared<MARABOU_IN_PLAJA::Context>(get_context_ptr()); }

/**********************************************************************************************************************/

void ModelMarabou::set_start(std::unique_ptr<Expression>&& start_ptr) {
    start = start_ptr ? TO_NORMALFORM::normalize(std::move(start_ptr)) : nullptr;
    startMarabou = nullptr;
    // startMarabouWithInit = nullptr;
}

void ModelMarabou::set_reach(std::unique_ptr<Expression>&& reach_ptr) {
    reach = reach_ptr ? TO_NORMALFORM::normalize(std::move(reach_ptr)) : nullptr;
    reachMarabou.clear();
}

void ModelMarabou::set_non_terminal(std::unique_ptr<Expression>&& terminal, bool negate) {
    PLAJA_ASSERT(PLAJA_GLOBAL::enableTerminalStateSupport)
    ModelSmt::set_non_terminal(std::move(terminal), negate);
    STMT_IF_TERMINAL_STATE_SUPPORT(if (nonTerminal) { nonTerminal = TO_NORMALFORM::normalize(std::move(nonTerminal)); })
}

/**********************************************************************************************************************/

std::vector<MarabouVarIndex_type> ModelMarabou::extract_nn_inputs(const StateIndexesInMarabou& state_indexes) const {
    std::vector<MarabouVarIndex_type> nn_inputs;
    nn_inputs.reserve(interface->get_num_input_features());
    for (auto it = interface->init_input_feature_iterator(); !it.end(); ++it) { nn_inputs.push_back(state_indexes.to_marabou(it.get_input_feature_index())); }
    return nn_inputs;
}

void ModelMarabou::match_state_vars_to_nn(const NNInMarabou& nn_in_marabou, StateIndexesInMarabou& state_indexes) const {
    PLAJA_ASSERT(nn_in_marabou._context() == state_indexes.get_context())
    // set tightest possible bounds:
    bool bounds_differ = false;
    for (InputIndex_type input_index = 0; input_index < interface->get_num_input_features(); ++input_index) {
        auto state_index = interface->get_input_feature_index(input_index);
        PLAJA_ASSERT(not state_indexes.exists(state_index)) // not expected
        auto marabou_var = nn_in_marabou.get_var(0, input_index);
        PLAJA_ASSERT(nn_in_marabou._context().is_marked_input(marabou_var));
        state_indexes.set(state_index, marabou_var);
        if (modelInfo->is_integer(state_index)) { context->mark_integer(marabou_var); }
        bounds_differ = context->tighten_bounds(marabou_var, modelInfo->get_lower_bound(state_index), modelInfo->get_upper_bound(state_index)) or bounds_differ; // use the tightest possible domain
    }

    PLAJA_LOG_IF(bounds_differ, "Warning: bounds of network and model differ! Using tightest combination in Marabou context...")
}

void ModelMarabou::generate_steps(StepIndex_type max_step) {

    PLAJA_ASSERT(stateIndexesInMarabou.empty() or stateIndexesInMarabou.size() == get_max_num_step())
    PLAJA_ASSERT(stateIndexesInMarabou.size() == maxVarPerStep.size())
    PLAJA_ASSERT(nnInMarabouPerStep.empty() or nnInMarabouPerStep.size() == get_max_path_len())

    maxStep = std::max(max_step, maxStep);

    stateIndexesInMarabou.reserve(get_max_num_step());
    maxVarPerStep.reserve(get_max_num_step());
    nnInMarabouPerStep.reserve(get_max_path_len());

    for (auto it = iterate_path(); !it.end(); ++it) {

        if (it.step() >= stateIndexesInMarabou.size()) {
            stateIndexesInMarabou.push_back(std::make_unique<StateIndexesInMarabou>(*modelInfo, *context));
            generate_state_indexes(*stateIndexesInMarabou.back(), not ignore_locs(), true);
            if (not maxVarPerStep.empty()) { maxVarPerStep.back().second = context->max_var(); }
            maxVarPerStep.emplace_back(context->max_var(), context->max_var());
        }

        if (has_nn() and it.step() >= nnInMarabouPerStep.size()) {
            const auto nnInterface = dynamic_cast<const Jani2NNet*>(interface);
            const auto nn_inputs = extract_nn_inputs(get_state_indexes(it.step()));
            nnInMarabouPerStep.push_back(std::make_unique<NNInMarabou>(nnInterface->load_network(), *context, &nn_inputs));
            PLAJA_ASSERT(it.step() < maxVarPerStep.size())
            maxVarPerStep[it.step()] = { context->max_var(), context->max_var() };

            // load preprocessed nn bounds?

            if (nnBounds != MILPSolverBoundTighteningType::NONE and it.step() > 0) {
                const auto& nn_in_marabou = nnInMarabouPerStep[0];
                const auto& nn_in_marabou_new = nnInMarabouPerStep.back();
                for (auto nn_it = nn_in_marabou->neuronIterator(); !nn_it.end(); ++nn_it) {
                    const auto var = nn_it.get_var();
                    context->tighten_bounds(nn_in_marabou_new->get_var(nn_it._layer(), nn_it._neuron()), context->get_lower_bound(var), context->get_upper_bound(var));
                    if (nn_it.has_activation_var()) {
                        const auto var_a = nn_it.get_activation_var();
                        context->tighten_bounds(nn_in_marabou_new->get_activation_var(nn_it._layer(), nn_it._neuron()), context->get_lower_bound(var_a), context->get_upper_bound(var_a));
                    }
                }
            }

        }

#if 0 // see above
        if (it.step() >= nnInMarabouPerStep.size()) {
            nnInMarabouPerStep.emplace_back(new NNInMarabou(nnInterface.load_network(), *context));
            outputConstraintsPerStep.push_back(MARABOU_IN_PLAJA::OutputConstraints::construct(*nnInMarabouPerStep.back()));
            stateIndexesInMarabou.emplace_back(new StateIndexesInMarabou(modelInfo, *context));
            match_state_vars_to_nn(*nnInMarabouPerStep.back(), *stateIndexesInMarabou.back());
            generate_state_indexes(*stateIndexesInMarabou.back(), doLocations, true);
        }
#endif

    }
    PLAJA_ASSERT(not has_nn() or nnInMarabouPerStep.size() == get_max_path_len())

    // there is one additional stateIndexesInMarabou step
    if (stateIndexesInMarabou.size() < get_max_num_step()) {
        stateIndexesInMarabou.push_back(std::make_unique<StateIndexesInMarabou>(*modelInfo, *context));
        generate_state_indexes(*stateIndexesInMarabou.back(), not ignore_locs(), true); // last variable set is not set as input
        maxVarPerStep.back().second = context->max_var();
        maxVarPerStep.emplace_back(context->max_var(), context->max_var());
    }
    PLAJA_ASSERT(stateIndexesInMarabou.size() == get_max_num_step())

    // start and reach:

    if (not startMarabou) {

        if (start) {

            // PLAJA_ASSERT(not startMarabouWithInit)
            startMarabou = MARABOU_IN_PLAJA::to_marabou_constraint(*start, get_state_indexes(0));
            /* startMarabouWithInit = std::make_unique<DisjunctionInMarabou>(*context);
            auto& disjunction = PLAJA_UTILS::cast<MarabouConstraint, DisjunctionInMarabou>(*startMarabouWithInit);
            startMarabou->add_to_disjunction(disjunction);
            get_state_indexes(0).encode_state(modelInfo->get_initial_values())->add_to_disjunction(disjunction); */

        } /* else if (not startMarabouWithInit) {
            startMarabouWithInit = get_state_indexes(0).encode_state(modelInfo->get_initial_values());
        } */

    }

    if (reach) {
        reachMarabou.reserve(get_max_num_step());
        for (auto it = iterate_steps(reachMarabou.size()); !it.end(); ++it) { reachMarabou.push_back(MARABOU_IN_PLAJA::to_marabou_constraint(*reach, get_state_indexes(it.step()))); }
    }
    PLAJA_ASSERT(reachMarabou.empty() or reachMarabou.size() == get_max_num_step())

#ifdef ENABLE_TERMINAL_STATE_SUPPORT
    const auto* non_terminal = get_non_terminal();
    if (non_terminal) {
        nonTerminalSmt.reserve(get_max_num_step());
        for (auto it = iterate_steps(nonTerminalSmt.size()); !it.end(); ++it) { nonTerminalSmt.push_back(to_marabou(*non_terminal, it.step())); }
    }
    PLAJA_ASSERT((not non_terminal and nonTerminalSmt.empty()) or nonTerminalSmt.size() == get_max_num_step())
#endif

    // generate steps for action ops if already present -> *not* during construction
    if (successorGenerator) {
        successorGenerator->generate_steps();
        outputConstraints->generate_steps(); // Needs successor generator.
    }
}

void ModelMarabou::generate_state_indexes(StateIndexesInMarabou& state_indexes, bool do_locs, bool mark_input) const {

#if 0
    // set actual inputs first
    if (mark_input) {
        for (auto it = nnInterface.inputFeatureIterator(); !it.end(); ++it) {
            auto state_index = it.get_input_feature_index();
            PLAJA_ASSERT(not state_indexes.exists(state_index))
            // if (state_indexes.exists(state_index)) { continue; }
            state_indexes.add(state_index, modelInfo.is_location(state_index) or modelInfo.is_integer(state_index), true);
        }
    }
#endif

    // also set other variable as inputs
    for (auto it = modelInfo->stateIndexIterator(true); !it.end(); ++it) {
        if (it.is_location() and not do_locs and
            // always add locations that serve as input -- even if they are not used as input now (mark_input), they may be used later on
            (not std::any_of(inputLocations.cbegin(), inputLocations.cend(), [&it](VariableIndex_type loc_input) { return it.state_index() == loc_input; }))) {
            continue;
        }
        if (state_indexes.exists(it.state_index())) { continue; }
        state_indexes.add(it.state_index(), it.is_location() or it.is_integer(), mark_input);
    }
}

/**********************************************************************************************************************/

void ModelMarabou::add_start(MARABOU_IN_PLAJA::QueryConstructable& query) const { startMarabou->add_to_query(query); }

void ModelMarabou::add_init(MARABOU_IN_PLAJA::QueryConstructable& query, StepIndex_type step) const {
    get_state_indexes(step).encode_state(modelInfo->get_initial_values())->move_to_query(query);
}

void ModelMarabou::exclude_start(MARABOU_IN_PLAJA::QueryConstructable& query, bool include_init, StepIndex_type step) const {
    MARABOU_IN_PLAJA::to_marabou_constraint(*start, get_state_indexes(step))->move_to_negation()->move_to_query(query);
    if (include_init) { get_state_indexes(step).encode_state(modelInfo->get_initial_values())->move_to_negation()->move_to_query(query); }
}

void ModelMarabou::add_reach(MARABOU_IN_PLAJA::QueryConstructable& query, StepIndex_type step) const {
    PLAJA_ASSERT(step < reachMarabou.size())
    reachMarabou[step]->add_to_query(query);
}

void ModelMarabou::exclude_reach(MARABOU_IN_PLAJA::QueryConstructable& query, StepIndex_type step) const {
    PLAJA_ASSERT(step < reachMarabou.size())
    reachMarabou[step]->compute_negation()->move_to_query(query);
}

/**********************************************************************************************************************/

std::unique_ptr<MARABOU_IN_PLAJA::MarabouQuery> ModelMarabou::make_query(bool add_nn, bool add_target_vars, StepIndex_type step) const {
    MARABOU_IN_PLAJA::ContextView context_view(share_context());
    PLAJA_ASSERT(step < maxVarPerStep.size())
    const auto max_vars = add_target_vars ? maxVarPerStep[step].second : maxVarPerStep[step].first;
    /*
     * Target vars marked as input to extract solution in the presence of non-deterministic assignments (and enforce integer solution in branch and bound).
     * In the absence of such we still save the AST-evaluation inside PlaJA.
     * Moreover, in branch and bound (since mapping in MARABOU_IN_PLAJA::Solution is ordered)
     * integer deterministically set target variables are always be checked last -- then enforced to be integer via source assignment.
    */
    context_view.set_upper_bound_input(max_vars);
    context_view.set_upper_bound_var(max_vars);
    auto query = std::make_unique<MARABOU_IN_PLAJA::MarabouQuery>(context_view);
    if (add_nn and has_nn()) { get_nn_in_marabou(step).add_to_query(*query); }
    return query;
}

void ModelMarabou::add_nn_to_query(MARABOU_IN_PLAJA::MarabouQuery& query, StepIndex_type step) const { get_nn_in_marabou(step).add_to_query(query); }

void ModelMarabou::add_output_interface(MARABOU_IN_PLAJA::QueryConstructable& query, ActionLabel_type action_label, StepIndex_type step) const {
    PLAJA_ASSERT(outputConstraints)

    outputConstraints->add(query, interface->get_output_index(action_label), step);

    if (nnBoundsPerLabel != MILPSolverBoundTighteningType::NONE) {
        const auto output_index = interface->get_output_index(action_label);
        PLAJA_ASSERT(output_index < tighteningsPerLabel.size())
        for (const auto& tightening: tighteningsPerLabel[output_index]) { query.set_tightening<false>(*tightening); }
    }

}

void ModelMarabou::add_output_interface_negation(MARABOU_IN_PLAJA::QueryConstructable& query, ActionLabel_type action_label, StepIndex_type step) const {
    PLAJA_ASSERT(outputConstraints)
    outputConstraints->add_negation(query, interface->get_output_index(action_label), step);
}

void ModelMarabou::add_output_interface_other_label(MARABOU_IN_PLAJA::MarabouQuery& query, ActionLabel_type action_label, StepIndex_type step) const {
    PLAJA_ASSERT(outputConstraints)
    outputConstraints->add_negation_other_label(query, interface->get_output_index(action_label), step);
}

void ModelMarabou::add_output_interface_for_op(MARABOU_IN_PLAJA::QueryConstructable& query, ActionOpID_type op_id, StepIndex_type step) const {
    OpApplicability* op_app; // NOLINT(*-init-variables)
    if constexpr (PLAJA_GLOBAL::enableApplicabilityCache) { op_app = get_op_applicability(); }
    if constexpr (PLAJA_GLOBAL::enableApplicabilityCache) { if (op_app) { op_app->set_self_applicability(true); } } // Consider this an optimization specific to cached op applicability.
    add_output_interface(query, successorGenerator->get_action_label(op_id), step);
    if constexpr (PLAJA_GLOBAL::enableApplicabilityCache) { if (op_app) { op_app->unset_self_applicability(); } }
}

/**********************************************************************************************************************/

const std::vector<const ActionOpMarabou*>& ModelMarabou::get_action_structure(ActionLabel_type label) const { return successorGenerator->get_action_structure(label); }

const ActionOpMarabou& ModelMarabou::get_action_op(ActionOpID_type op_id) const { return successorGenerator->get_action_op(op_id); }

void ModelMarabou::add_guard(MARABOU_IN_PLAJA::QueryConstructable& query, ActionOpID_type op_id, bool do_locs, StepIndex_type step) const { successorGenerator->add_guard(query, op_id, do_locs, step); }

void ModelMarabou::add_update(MARABOU_IN_PLAJA::QueryConstructable& query, ActionOpID_type op_id, UpdateIndex_type update, bool do_locs, bool do_copy, StepIndex_type step) const { successorGenerator->add_update(query, op_id, update, do_locs, do_copy, step); }

void ModelMarabou::add_action_op(MARABOU_IN_PLAJA::QueryConstructable& query, ActionOpID_type op_id, UpdateIndex_type update, bool do_locs, bool do_copy, StepIndex_type step) const { successorGenerator->add_action_op(query, op_id, update, do_locs, do_copy, step); }

std::unique_ptr<MarabouConstraint> ModelMarabou::guards_to_marabou(ActionLabel_type action_label, StepIndex_type step) const { return successorGenerator->guards_to_marabou(action_label, step); }

void ModelMarabou::add_guard_disjunction(MARABOU_IN_PLAJA::QueryConstructable& query, ActionLabel_type action_label, StepIndex_type step) const { successorGenerator->add_guard_disjunction(query, action_label, step); } // NOLINT(bugprone-easily-swappable-parameters)

void ModelMarabou::add_choice_point(MARABOU_IN_PLAJA::QueryConstructable& query, bool nn_aware, StepIndex_type step) const {
    if (nn_aware) {

        PLAJA_ASSERT(has_nn())
        PLAJA_ASSERT(outputConstraints)
        // const auto& src_vars = get_state_vars(step);
        // const auto& target_vars = get_state_vars(step + 1);

        const auto* nn_interface = get_interface();
        PLAJA_ASSERT(nn_interface)
        PLAJA_ASSERT(nn_interface->get_num_output_features() > 0)

        auto choice_point = std::make_unique<DisjunctionInMarabou>(*context);

        for (auto it = nn_interface->init_output_feature_iterator(); !it.end(); ++it) {

            auto conjunction = std::make_unique<ConjunctionInMarabou>(*context);

            // nn selection
            outputConstraints->compute(it._output_index(), step)->move_to_conjunction(*conjunction);

            // transition relation
            auto action_in_marabou = successorGenerator->action_to_marabou(it.get_output_label(), step);
            PLAJA_ASSERT_EXPECT(not action_in_marabou->is_trivially_false())

            if (action_in_marabou->is_trivially_false()) { continue; } // Special case: No "applicable" operator.

            if (not action_in_marabou->is_trivially_true()) { action_in_marabou->move_to_conjunction(*conjunction); }

            conjunction->move_to_disjunction(*choice_point);
        }

        for (const ActionLabel_type unlearned_label: nn_interface->get_unlearned_labels()) {
            auto action_in_marabou = successorGenerator->action_to_marabou(unlearned_label, step);
            PLAJA_ASSERT_EXPECT(not action_in_marabou->is_trivially_false() or unlearned_label == ACTION::silentLabel)
            PLAJA_ASSERT_EXPECT(not action_in_marabou->is_trivially_true())

            if (action_in_marabou->is_trivially_false()) { continue; }

            if (action_in_marabou->is_trivially_true()) { return; }

            action_in_marabou->move_to_disjunction(*choice_point);
        }

        choice_point->move_to_query(query);

    } else {

        successorGenerator->expansion_to_marabou(step)->move_to_query(query);

    }

}

void ModelMarabou::add_loop_exclusion(MARABOU_IN_PLAJA::QueryConstructable& query, StepIndex_type step_index) const {
    if (step_index < 1) { return; }

    for (StepIndex_type step_index_1 = 0; step_index_1 <= step_index; ++step_index_1) {

        const auto& variables_per_step_1 = get_state_indexes(step_index_1);

        for (StepIndex_type step_index_2 = 0; step_index_2 < step_index_1; ++step_index_2) {

            const auto& variables_per_step_2 = get_state_indexes(step_index_2);

            DisjunctionInMarabou state_disjunction(get_context());

            for (auto it = variables_per_step_1.iterator(); !it.end(); ++it) {
                Equation eq(Equation::EQ);
                eq.setScalar(0);
                eq.addAddend(-1, it.marabou_var());
                eq.addAddend(1, variables_per_step_2.to_marabou(it.state_index()));
                EquationConstraint(get_context(), std::move(eq)).move_to_negation()->move_to_disjunction(state_disjunction);
            }

            state_disjunction.move_to_query(query);
        }

    }
}

/**********************************************************************************************************************/

std::unique_ptr<MarabouConstraint> ModelMarabou::to_marabou(const Expression& expr, StepIndex_type step_index) const {
    return MARABOU_IN_PLAJA::to_marabou_constraint(expr, get_state_indexes(step_index));
}

void ModelMarabou::add_to_query(MARABOU_IN_PLAJA::QueryConstructable& query, const Expression& expr, StepIndex_type step_index) const {
    to_marabou(expr, step_index)->move_to_query(query);
}

/**********************************************************************************************************************/

std::unique_ptr<PLAJA::SmtSolver> ModelMarabou::init_solver(const PLAJA::Configuration& config, StepIndex_type step) const {
    if (step == SEARCH::noneStep) { return MARABOU_IN_PLAJA::SMT_SOLVER::construct(config, share_context()); }
    else { return MARABOU_IN_PLAJA::SMT_SOLVER::construct(config, make_query(false, false, step)); }
}

std::unique_ptr<PLAJA::SmtConstraint> ModelMarabou::to_smt(const Expression& expr, StepIndex_type step) const { return to_marabou(expr, step); }

void ModelMarabou::add_to_solver(PLAJA::SmtSolver& solver, const Expression& expr, StepIndex_type step) const { add_to_query(PLAJA_UTILS::cast_ref<MARABOU_IN_PLAJA::SMTSolver>(solver), expr, step); }


bool ModelMarabou::lp_tightening(MARABOU_IN_PLAJA::QueryConstructable& query, MILPSolverBoundTighteningType type, LayerIndex_type layer_index) {
    GlobalConfiguration::ENABLE_SBT_ANYWAY = true;

    // should be present
    auto* nlr = query.provide_nlr();
    PLAJA_ASSERT(nlr)

    // nlr->intervalArithmeticBoundPropagation();
    //
    nlr->symbolicBoundPropagation();
    for (const auto& tightening: nlr->viewConstraintTightenings()) { query.set_tightening<true>(tightening); }
    nlr->clearConstraintTightenings();
    //
    query.update_nlr();
    //
    nlr->deepPolyPropagation();
    for (const auto& tightening: nlr->viewConstraintTightenings()) { query.set_tightening<true>(tightening); }
    nlr->clearConstraintTightenings();
    //
    query.update_nlr();

    bool rlt = true;
    try {
        switch (type) {
            case MILPSolverBoundTighteningType::LP_RELAXATION: {
                if (layer_index == LAYER_INDEX::none) { nlr->lpRelaxationPropagation(MILPSolverBoundTighteningType::LP_RELAXATION); }
                else { nlr->LPTighteningForOneLayer(layer_index, MILPSolverBoundTighteningType::LP_RELAXATION); }
                break;
            }
            case MILPSolverBoundTighteningType::LP_RELAXATION_INCREMENTAL:
                if (layer_index == LAYER_INDEX::none) { nlr->lpRelaxationPropagation(MILPSolverBoundTighteningType::LP_RELAXATION_INCREMENTAL); }
                else {
                    PLAJA_LOG(PLAJA_UTILS::to_red_string("Incremental computation not supported for single layer tightening."))
                    nlr->LPTighteningForOneLayer(layer_index, MILPSolverBoundTighteningType::LP_RELAXATION);
                }
                break;
            case MILPSolverBoundTighteningType::MILP_ENCODING: {
                if (layer_index == LAYER_INDEX::none) { nlr->MILPPropagation(MILPSolverBoundTighteningType::MILP_ENCODING); }
                else { nlr->MILPTighteningForOneLayer(layer_index, MILPSolverBoundTighteningType::MILP_ENCODING); }
                break;
            }
            case MILPSolverBoundTighteningType::MILP_ENCODING_INCREMENTAL: {
                if (layer_index == LAYER_INDEX::none) { nlr->MILPPropagation(MILPSolverBoundTighteningType::MILP_ENCODING_INCREMENTAL); }
                else {
                    PLAJA_LOG(PLAJA_UTILS::to_red_string("Incremental computation not supported for single layer tightening."))
                    nlr->MILPTighteningForOneLayer(layer_index, MILPSolverBoundTighteningType::MILP_ENCODING);
                }
                break;
            }
                // case MILPSolverBoundTighteningType::ITERATIVE_PROPAGATION: { nlr->iterativePropagation(); break; }
            default: { PLAJA_ABORT }
        }

        for (const auto& tightening: nlr->viewConstraintTightenings()) { query.set_tightening<true>(tightening); }

    } catch (InfeasibleQueryException& e) { rlt = false; }

    query.update_nlr();

    GlobalConfiguration::ENABLE_SBT_ANYWAY = false;
    return rlt;
}

bool ModelMarabou::lp_tightening_output(MARABOU_IN_PLAJA::QueryConstructable& query, MILPSolverBoundTighteningType type) const {
    const auto nnInterface = dynamic_cast<const Jani2NNet*>(interface);
    return lp_tightening(query, type, 1 + nnInterface->get_num_hidden_layers() * 2);
}
