//
// This file is part of the PlaJA code base.
// Copyright (c) (2023 - 2024) Chaahat Jain.
//

#include "model_veritas.h"
#include "../../../parser/ast/expression/expression.h"
#include "../../../parser/visitor/extern/to_normalform.h"
#include "../../../utils/utils.h"
#include "../../factories/configuration.h"
#include "../../information/input_feature_to_jani.h"
#include "../../information/jani2ensemble/jani_2_ensemble.h"
#include "../../information/model_information.h"
#include "../../smt/successor_generation/op_applicability.h"
#include "../solver/solver.h"
#include "../successor_generation/successor_generator_veritas.h"
#include "../visitor/extern/to_veritas_visitor.h"
#include "../constraints_in_veritas.h"
#include "addtree.hpp"
#include <iostream>
#include <fstream>
#include <sstream>

// extern:
namespace PLAJA_OPTION {
    extern const std::string app_filter_ensemble;
}
/**********************************************************************************************************************/

ModelVeritas::ModelVeritas(const PLAJA::Configuration& config, bool init_suc_gen):
    ModelSmt(std::shared_ptr<PLAJA::SmtContext>(new VERITAS_IN_PLAJA::Context()), config)
    , context(PLAJA_UTILS::cast_ptr<VERITAS_IN_PLAJA::Context>(get_context_ptr().get())) {

    // PLAJA_ASSERT_EXPECT(nnInterface and has_nn()) // Seems to work without NN, though not exhaustively tested. To be used for sanity checks at development time anyway ...

    if (interface->_do_applicability_filtering()) {
        if (PLAJA_GLOBAL::optionParser->has_value_option(PLAJA_OPTION::app_filter_ensemble)) {
            set_filter(FilterType::Naive);
            auto path = PLAJA_GLOBAL::optionParser->get_value_option_string(PLAJA_OPTION::app_filter_ensemble);
            std::ifstream file(path);
            PLAJA_ASSERT(file.is_open());
            std::string filepath{std::istreambuf_iterator<char>(file), std::istreambuf_iterator<char>()};
            std::istringstream iss(filepath);
            auto appFilter = veritas::addtree_from_json<veritas::AddTree>(iss);
            applicabilityTrees = { appFilter };
        } else {
            set_filter(FilterType::Indicator);
        }
    } else {
        set_filter(FilterType::None);
    }
    
    auto start_tmp = std::move(start); // Cache start.
    set_start(nullptr);
    set_reach(std::move(reach));
    STMT_IF_TERMINAL_STATE_SUPPORT(set_non_terminal(std::move(nonTerminal), false);)

    generate_steps(1);
    set_start(std::move(start_tmp)); // Reset start.

    if (start) {
        std::unique_ptr<VERITAS_IN_PLAJA::Solver> solver = VERITAS_IN_PLAJA::ENSEMBLE_SOLVER::construct(config, *make_query(false, false, 0));
        startVeritas = nullptr;
    }

    generate_steps(1);

    if (init_suc_gen) {
        successorGenerator = std::make_unique<VERITAS_IN_PLAJA::SuccessorGenerator>(get_successor_generator(), *this, true);
        init_output_constraints();
    }

}

ModelVeritas::~ModelVeritas() = default;

/**********************************************************************************************************************/
void ModelVeritas::init_output_constraints() {
    if (interface->_do_applicability_filtering() and filter == FilterType::Indicator) {
        applicabilityTrees = successorGenerator->get_applicability_filter();
        indicatorTrees = successorGenerator->get_indicator_trees();
        std::cout << "Added app trees: " << applicabilityTrees.size() << std::endl;
        std::cout << "Auxialliary Variables added: " << context->num_aux_vars() << std::endl;
    }
}

void ModelVeritas::set_start(std::unique_ptr<Expression>&& start_ptr) {
    start = start_ptr ? TO_NORMALFORM::normalize(std::move(start_ptr)) : nullptr;
    startVeritas = nullptr;
}

void ModelVeritas::set_reach(std::unique_ptr<Expression>&& reach_ptr) {
    reach = reach_ptr ? TO_NORMALFORM::normalize(std::move(reach_ptr)) : nullptr;
    reachVeritas.clear();
}

/**********************************************************************************************************************/

std::vector<VeritasVarIndex_type> ModelVeritas::extract_ensemble_inputs(const StateIndexesInVeritas& state_indexes) const {
    std::vector<VeritasVarIndex_type> ensemble_inputs;
    ensemble_inputs.reserve(interface->get_num_input_features());
    for (auto it = interface->init_input_feature_iterator(); !it.end(); ++it) { ensemble_inputs.push_back(state_indexes.to_veritas(it.get_input_feature_index())); }
    return ensemble_inputs;
}

void ModelVeritas::match_state_vars_to_ensemble(const VERITAS_IN_PLAJA::VeritasQuery& query_in_veritas, StateIndexesInVeritas& state_indexes) const {
    PLAJA_ASSERT(query_in_veritas._context() == state_indexes._context())
    // set tightest possible bounds:
    bool bounds_differ = false;
    for (InputIndex_type input_index = 0; input_index < interface->get_num_input_features(); ++input_index) {
        auto state_index = interface->get_input_feature_index(input_index);
        PLAJA_ASSERT(not state_indexes.exists(state_index)) // not expected
        state_indexes.set(state_index, input_index);
        if (modelInfo->is_integer(state_index)) { context->mark_integer(input_index); }
        bounds_differ = context->tighten_bounds(input_index, modelInfo->get_lower_bound(state_index), modelInfo->get_upper_bound(state_index)) or bounds_differ; // use the tightest possible domain
    }

    PLAJA_LOG_IF(bounds_differ, "Warning: bounds of network and model differ! Using tightest combination in Veritas context...")
}

void ModelVeritas::generate_steps(StepIndex_type max_step) {

    PLAJA_ASSERT(stateIndexesInVeritas.empty() or stateIndexesInVeritas.size() == get_max_num_step())
    PLAJA_ASSERT(stateIndexesInVeritas.size() == maxVarPerStep.size())
    PLAJA_ASSERT(ensembleInVeritasPerStep.empty() or ensembleInVeritasPerStep.size() == get_max_path_len())

    maxStep = std::max(max_step, maxStep);

    stateIndexesInVeritas.reserve(get_max_num_step());
    maxVarPerStep.reserve(get_max_num_step());
    ensembleInVeritasPerStep.reserve(get_max_path_len());

    for (auto it = iterate_path(); !it.end(); ++it) {

        if (it.step() >= stateIndexesInVeritas.size()) {
            stateIndexesInVeritas.push_back(std::make_unique<StateIndexesInVeritas>(*modelInfo, *context));
            generate_state_indexes(*stateIndexesInVeritas.back(), not ignore_locs(), true);
            if (not maxVarPerStep.empty()) { maxVarPerStep.back().second = context->max_var(); }
            maxVarPerStep.emplace_back(context->max_var(), context->max_var());
        }

        if (has_ensemble() and it.step() >= ensembleInVeritasPerStep.size()) {

            const auto ensemble_inputs = extract_ensemble_inputs(get_state_indexes(it.step()));
            auto ensembleInterface = dynamic_cast<const Jani2Ensemble*>(interface);
            ensembleInVeritasPerStep.push_back(std::make_unique<VERITAS_IN_PLAJA::VeritasQuery>(ensembleInterface->load_ensemble(), share_context(), ensemble_inputs));
            PLAJA_ASSERT(it.step() < maxVarPerStep.size())
            maxVarPerStep[it.step()] = { context->max_var(), context->max_var() };

        }

#if 0 // see above
        if (it.step() >= nnInVeritasPerStep.size()) {
            nnInVeritasPerStep.emplace_back(new NNInVeritas(nnInterface.load_network(), *context));
            outputConstraintsPerStep.push_back(VERITAS_IN_PLAJA::OutputConstraints::construct(*nnInVeritasPerStep.back()));
            stateIndexesInVeritas.emplace_back(new StateIndexesInVeritas(modelInfo, *context));
            match_state_vars_to_nn(*nnInVeritasPerStep.back(), *stateIndexesInVeritas.back());
            generate_state_indexes(*stateIndexesInVeritas.back(), doLocations, true);
        }
#endif

    }
    PLAJA_ASSERT(not has_ensemble() or ensembleInVeritasPerStep.size() == get_max_path_len())

    // there is one additional stateIndexesInVeritas step
    if (stateIndexesInVeritas.size() < get_max_num_step()) {
        stateIndexesInVeritas.push_back(std::make_unique<StateIndexesInVeritas>(*modelInfo, *context));
        generate_state_indexes(*stateIndexesInVeritas.back(), not ignore_locs(), true); // last variable set is not set as input
        maxVarPerStep.back().second = context->max_var();
        maxVarPerStep.emplace_back(context->max_var(), context->max_var());
    }
    PLAJA_ASSERT(stateIndexesInVeritas.size() == get_max_num_step())

    // start and reach:

    if (not startVeritas) {

        if (start) {

            // PLAJA_ASSERT(not startVeritasWithInit)
            startVeritas = VERITAS_IN_PLAJA::to_veritas_constraint(*start, get_state_indexes(0));
        } 

    }

    if (reach) {
        reachVeritas.reserve(get_max_num_step());
        for (auto it = iterate_steps(reachVeritas.size()); !it.end(); ++it) { reachVeritas.push_back(VERITAS_IN_PLAJA::to_veritas_constraint(*reach, get_state_indexes(it.step()))); }
    }
    PLAJA_ASSERT(reachVeritas.empty() or reachVeritas.size() == get_max_num_step())

    // generate steps for action ops if already present -> *not* during construction
    if (successorGenerator) {
        successorGenerator->generate_steps();
        // outputConstraints->generate_steps(); // Needs successor generator.
    }
}

void ModelVeritas::generate_state_indexes(StateIndexesInVeritas& state_indexes, bool do_locs, bool mark_input) const {
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
        state_indexes.add(it.state_index(), it.is_location() or it.is_integer());
    }
}

/**********************************************************************************************************************/

void ModelVeritas::add_start(VERITAS_IN_PLAJA::QueryConstructable& query) const { startVeritas->add_to_query(query); }

void ModelVeritas::add_init(VERITAS_IN_PLAJA::QueryConstructable& query, StepIndex_type step) const {
    get_state_indexes(step).encode_state(modelInfo->get_initial_values())->move_to_query(query);
}

void ModelVeritas::exclude_start(VERITAS_IN_PLAJA::QueryConstructable& query, bool include_init, StepIndex_type step) const {
    VERITAS_IN_PLAJA::to_veritas_constraint(*start, get_state_indexes(step))->move_to_negation()->move_to_query(query);
    if (include_init) { get_state_indexes(step).encode_state(modelInfo->get_initial_values())->move_to_negation()->move_to_query(query); }
}

void ModelVeritas::add_reach(VERITAS_IN_PLAJA::QueryConstructable& query, StepIndex_type step) const {
    PLAJA_ASSERT(step < reachVeritas.size())
    reachVeritas[step]->add_to_query(query);
}

void ModelVeritas::exclude_reach(VERITAS_IN_PLAJA::QueryConstructable& query, StepIndex_type step) const {
    PLAJA_ASSERT(step < reachVeritas.size())
    reachVeritas[step]->compute_negation()->move_to_query(query);
}

/**********************************************************************************************************************/

std::unique_ptr<VERITAS_IN_PLAJA::VeritasQuery> ModelVeritas::make_query(bool add_ensemble, bool add_target_vars, StepIndex_type step) const {
    VERITAS_IN_PLAJA::ContextView context_view(share_context());
    PLAJA_ASSERT(step < maxVarPerStep.size())
    const auto max_vars = add_target_vars ? maxVarPerStep[step].second : maxVarPerStep[step].first;
    /*
     * Target vars marked as input to extract solution in the presence of non-deterministic assignments (and enforce integer solution in branch and bound).
     * In the absence of such we still save the AST-evaluation inside PlaJA.
     * Moreover, in branch and bound (since mapping in VERITAS_IN_PLAJA::Solution is ordered)
     * integer deterministically set target variables are always be checked last -- then enforced to be integer via source assignment.
    */
    context_view.set_upper_bound_input(max_vars);
    context_view.set_upper_bound_var(max_vars);
    auto query = std::make_unique<VERITAS_IN_PLAJA::VeritasQuery>(context_view);
    if (add_ensemble and has_ensemble()) {
        auto ensembleInterface = dynamic_cast<const Jani2Ensemble*>(interface); 
        query->set_input_query(ensembleInterface->load_ensemble());
    }
    return query;
}

void ModelVeritas::add_policy_to_query(VERITAS_IN_PLAJA::VeritasQuery& query, StepIndex_type step) const { query.set_input_query(get_query_in_veritas(step).get_input_query()); }

veritas::AddTree ModelVeritas::get_applicability_filter(ActionLabel_type action_label) {
    if (filter == FilterType::Naive) {
        return applicabilityTrees[0];
    }
    int num_actions = applicabilityTrees.size();
    veritas::AddTree trees(num_actions, veritas::AddTreeType::REGR);
    for (int i = 0; i < num_actions; ++i) {
        if (action_label == i) {
            auto test = indicatorTrees.make_multiclass(action_label, num_actions);
            trees.add_trees(test);
            continue;
        }
        auto ts = applicabilityTrees[i];
        trees.add_trees(ts);
    }
    return trees;
}


/**********************************************************************************************************************/

const std::vector<const ActionOpVeritas*>& ModelVeritas::get_action_structure(ActionLabel_type label) const { return successorGenerator->get_action_structure(label); }

const ActionOpVeritas& ModelVeritas::get_action_op(ActionOpID_type op_id) const { return successorGenerator->get_action_op(op_id); }

void ModelVeritas::add_guard(VERITAS_IN_PLAJA::QueryConstructable& query, ActionOpID_type op_id, bool do_locs, StepIndex_type step) const { successorGenerator->add_guard(query, op_id, do_locs, step); }

void ModelVeritas::add_update(VERITAS_IN_PLAJA::QueryConstructable& query, ActionOpID_type op_id, UpdateIndex_type update, bool do_locs, bool do_copy, StepIndex_type step) const { successorGenerator->add_update(query, op_id, update, do_locs, do_copy, step); }

void ModelVeritas::add_action_op(VERITAS_IN_PLAJA::QueryConstructable& query, ActionOpID_type op_id, UpdateIndex_type update, bool do_locs, bool do_copy, StepIndex_type step) const { successorGenerator->add_action_op(query, op_id, update, do_locs, do_copy, step); }

/**********************************************************************************************************************/

std::unique_ptr<VeritasConstraint> ModelVeritas::to_veritas(const Expression& expr, StepIndex_type step_index) const {
    return VERITAS_IN_PLAJA::to_veritas_constraint(expr, get_state_indexes(step_index));
}

void ModelVeritas::add_to_query(VERITAS_IN_PLAJA::QueryConstructable& query, const Expression& expr, StepIndex_type step_index) const {
    to_veritas(expr, step_index)->move_to_query(query);
}

/**********************************************************************************************************************/
std::unique_ptr<PLAJA::SmtSolver> ModelVeritas::init_solver(const PLAJA::Configuration& config, StepIndex_type step) const {
    if (step == SEARCH::noneStep) { return VERITAS_IN_PLAJA::ENSEMBLE_SOLVER::construct(config, std::move(share_context())); }
    else { return VERITAS_IN_PLAJA::ENSEMBLE_SOLVER::construct(config, *make_query(false, false, step)); }
}

std::unique_ptr<PLAJA::SmtConstraint> ModelVeritas::to_smt(const Expression& expr, StepIndex_type step) const { return to_veritas(expr, step); }

void ModelVeritas::add_to_solver(PLAJA::SmtSolver& solver, const Expression& expr, StepIndex_type step) const { add_to_query(PLAJA_UTILS::cast_ref<VERITAS_IN_PLAJA::Solver>(solver), expr, step); }
