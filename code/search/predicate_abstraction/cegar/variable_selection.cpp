//
// This file is part of the PlaJA code base.
// Copyright (c) (2019 - 2023) Marcel Vinzent.
// See README.md in the top-level directory for licensing information.
//

#include "variable_selection.h"
#include "../../../option_parser/option_parser_aux.h"
#include "../../../parser/ast/expression/constant_value_expression.h"
#include "../../../parser/ast/expression/expression_utils.h"
#include "../../../parser/nn_parser/neural_network.h"
#include "../../../parser/ast/expression/lvalue_expression.h"
#include "../../../stats/stats_base.h"
#include "../../../utils/floating_utils.h"
#include "../../factories/configuration.h"
#include "../../information/jani2nnet/jani_2_nnet.h"
#include "../../information/model_information.h"
#include "../../smt_nn/solver/smt_solver_marabou.h"
#include "../../smt_nn/nn_in_marabou.h"
#include "../../states/state_values.h"
#include "../nn/smt/model_marabou_pa.h"

#ifdef USE_VERITAS
#include "../ensemble/smt/model_veritas_pa.h"
#endif

#include "../pa_states/pa_state_base.h"

namespace PLAJA_OPTION_DEFAULTS {

    const std::string var_sel_marabou_mode("relaxed"); // NOLINT(cert-err58-cpp)
    constexpr PLAJA::floating perturbation_bound(-1);
    constexpr PLAJA::floating perturbation_bound_precision(0.05);
    constexpr bool traversing_order_asc(true);
    constexpr bool perturb_witness(true);
    constexpr bool perturb_towards_second(true);
    constexpr bool perturb_1_norm(false);
    constexpr bool global_perturbation(true);
    constexpr bool individual_perturbation(true);
    constexpr bool per_direction_perturbation(true);

}

namespace PLAJA_OPTION {

    extern const std::string perturbation_bound;

    extern const std::string var_sel_marabou_mode("var-sel-marabou-mode"); // NOLINT(cert-err58-cpp)
    const std::string perturbation_bound("perturbation-bound"); // NOLINT(cert-err58-cpp)
    const std::string perturbation_bound_precision("perturbation-bound-precision"); // NOLINT(cert-err58-cpp)
    const std::string traversing_order_asc("traversing-order-asc"); // NOLINT(cert-err58-cpp)
    const std::string perturb_witness("perturb-witness"); // NOLINT(cert-err58-cpp)
    const std::string perturb_towards_second("perturb-towards-second"); // NOLINT(cert-err58-cpp)
    const std::string perturb_1_norm("perturb-1-norm"); // NOLINT(cert-err58-cpp)
    const std::string global_perturbation("global-perturbation"); // NOLINT(cert-err58-cpp)
    const std::string individual_perturbation("individual-perturbation"); // NOLINT(cert-err58-cpp)
    const std::string per_direction_perturbation("per-direction-perturbation"); // NOLINT(cert-err58-cpp)

    namespace VARIABLE_SELECTION {

        void add_options(PLAJA::OptionParser& option_parser) {
            OPTION_PARSER::add_option(option_parser, PLAJA_OPTION::var_sel_marabou_mode, PLAJA_OPTION_DEFAULTS::var_sel_marabou_mode);
            OPTION_PARSER::add_double_option(option_parser, PLAJA_OPTION::perturbation_bound, PLAJA_OPTION_DEFAULTS::perturbation_bound);
            OPTION_PARSER::add_double_option(option_parser, PLAJA_OPTION::perturbation_bound_precision, PLAJA_OPTION_DEFAULTS::perturbation_bound_precision);
            OPTION_PARSER::add_bool_option(option_parser, PLAJA_OPTION::traversing_order_asc, PLAJA_OPTION_DEFAULTS::traversing_order_asc);
            OPTION_PARSER::add_bool_option(option_parser, PLAJA_OPTION::perturb_witness, PLAJA_OPTION_DEFAULTS::perturb_witness);
            OPTION_PARSER::add_bool_option(option_parser, PLAJA_OPTION::perturb_towards_second, PLAJA_OPTION_DEFAULTS::perturb_towards_second);
            OPTION_PARSER::add_bool_option(option_parser, PLAJA_OPTION::perturb_1_norm, PLAJA_OPTION_DEFAULTS::perturb_1_norm);
            OPTION_PARSER::add_bool_option(option_parser, PLAJA_OPTION::global_perturbation, PLAJA_OPTION_DEFAULTS::global_perturbation);
            OPTION_PARSER::add_bool_option(option_parser, PLAJA_OPTION::individual_perturbation, PLAJA_OPTION_DEFAULTS::individual_perturbation);
            OPTION_PARSER::add_bool_option(option_parser, PLAJA_OPTION::per_direction_perturbation, PLAJA_OPTION_DEFAULTS::per_direction_perturbation);
        }

        void print_options() {
            OPTION_PARSER::print_option(PLAJA_OPTION::var_sel_marabou_mode, OPTION_PARSER::valueStr, PLAJA_OPTION_DEFAULTS::var_sel_marabou_mode, "Mode to check Marabou queries during variable selection.", false);
            OPTION_PARSER::print_double_option(PLAJA_OPTION::perturbation_bound, PLAJA_OPTION_DEFAULTS::perturbation_bound, "Perturbation bound during CEGAR variable selection. Increased using binary search if all variables are irrelevant.");
            OPTION_PARSER::print_double_option(PLAJA_OPTION::perturbation_bound_precision, PLAJA_OPTION_DEFAULTS::perturbation_bound_precision, "Perturbation bound precision during CEGAR variable selection if binary search is used.");
            OPTION_PARSER::print_bool_option(PLAJA_OPTION::traversing_order_asc, PLAJA_OPTION_DEFAULTS::traversing_order_asc, "Traverse variables with ASC sensitivity during CEGAR variable selection.");
            OPTION_PARSER::print_bool_option(PLAJA_OPTION::perturb_witness, PLAJA_OPTION_DEFAULTS::perturb_witness, "Add perturbation to witness during CEGAR variable selection (alternative: concretization state).");
            OPTION_PARSER::print_bool_option(PLAJA_OPTION::perturb_towards_second, PLAJA_OPTION_DEFAULTS::perturb_towards_second, "Restrict perturbation towards second state during CEGAR variable selection.");
            OPTION_PARSER::print_bool_option(PLAJA_OPTION::perturb_1_norm, PLAJA_OPTION_DEFAULTS::perturb_1_norm, "Use 1-norm-perturbation during CEGAR variable selection (default: infinity).");
            OPTION_PARSER::print_bool_option(PLAJA_OPTION::global_perturbation, PLAJA_OPTION_DEFAULTS::global_perturbation, "Perform global perturbation when searching decision boundary (not variable selection).");
            OPTION_PARSER::print_bool_option(PLAJA_OPTION::individual_perturbation, PLAJA_OPTION_DEFAULTS::individual_perturbation, "Perform individual per-variable perturbation when searching decision boundary (not variable selection).");
            OPTION_PARSER::print_bool_option(PLAJA_OPTION::per_direction_perturbation, PLAJA_OPTION_DEFAULTS::per_direction_perturbation, "Perform per-direction per-variable perturbation when searching decision boundary (not variable selection).");
        }

    }

}

/**********************************************************************************************************************/

#include "variable_selection_aux.h"

/**********************************************************************************************************************/

VariableSelection::VariableSelection(const PLAJA::Configuration& config):
    model(config.has_sharable(PLAJA::SharableKey::MODEL_MARABOU) ? config.get_sharable_cast<ModelMarabou, ModelMarabouPA>(PLAJA::SharableKey::MODEL_MARABOU) : PLAJA_UTILS::cast_shared<ModelMarabouPA>(config.set_sharable<ModelMarabou>(PLAJA::SharableKey::MODEL_MARABOU, std::make_shared<ModelMarabouPA>(config))))
    , solver(nullptr)
    , varInfo(new VARIABLE_SELECTION::VarInfoVec(model->get_model_info()))
    , lastPertBound(new VARIABLE_SELECTION::PerturbationBoundMap(-1))
    // configuration:
    , perturbationBoundUser(config.get_double_option(PLAJA_OPTION::perturbation_bound))
    , perturbationBoundPrecision(config.get_double_option(PLAJA_OPTION::perturbation_bound_precision))
    , ascTravOrd(config.get_bool_option(PLAJA_OPTION::traversing_order_asc))
    , perturbWitness(config.get_bool_option(PLAJA_OPTION::perturb_witness))
    , perturbTowardsRef(config.get_bool_option(PLAJA_OPTION::perturb_towards_second))
    , perturb1Norm(config.get_bool_option(PLAJA_OPTION::perturb_1_norm))
    , globalPerturbation(config.get_bool_option(PLAJA_OPTION::global_perturbation))
    , individualPerturbation(config.get_bool_option(PLAJA_OPTION::individual_perturbation))
    , perDirectionPerturbation(config.get_bool_option(PLAJA_OPTION::per_direction_perturbation))
    , stats(config.has_sharable_ptr(PLAJA::SharableKey::STATS) ? config.get_sharable_ptr<PLAJA::StatsBase>(PLAJA::SharableKey::STATS) : nullptr)
    , pertBoundSum(0.0)
    , pertBoundCounter(0)
    , relevantVars(0)
    , irrelevantVars(0) {
    PLAJA_ASSERT(globalPerturbation or individualPerturbation)

    PLAJA::Configuration config_solver(config);
    config_solver.set_value_option(PLAJA_OPTION::marabou_mode, config.get_option(PLAJA_OPTION::var_sel_marabou_mode));
    solver = MARABOU_IN_PLAJA::SMT_SOLVER::construct(config_solver, model->make_query(true, false, 0));

    PLAJA_ASSERT((PLAJA_FLOATS::lte(0.0, perturbationBoundUser) and PLAJA_FLOATS::gte(perturbationBoundUser, 1.0)) or perturbationBoundUser == -1.0)

    if (stats) {
        stats->add_double_stats(PLAJA::StatsDouble::AvPertBound, "AvPerturbationBound", 0);
        stats->add_double_stats(PLAJA::StatsDouble::RelevantVars, "RelevantVars", 0);
        stats->add_double_stats(PLAJA::StatsDouble::IrrelevantVars, "IrrelevantVars", 0);
    }

}

VariableSelection::~VariableSelection() = default;

/**********************************************************************************************************************/

std::list<const LValueExpression*> VariableSelection::compute_traversing_order() {
    const auto interface = model->get_interface();
    const auto& nn_interface = *(dynamic_cast<const Jani2NNet*>(interface));
    const auto& nn = nn_interface.load_network();
    auto num_outputs = nn_interface.get_num_output_features();

    // input state to evaluate over
    std::vector<double> input_state;
    input_state.reserve(nn_interface.get_num_input_features());
    for (auto it = nn_interface.init_input_feature_iterator(false); !it.end(); ++it) {
        input_state.push_back(varInfo->get_info(it.get_input_feature_index()).get_perturbation_point());
    }
    PLAJA_ASSERT(input_state.size() <= nn_interface.get_num_input_features())

    std::vector<double> outputs_base;
    nn.evaluate(input_state, outputs_base, num_outputs); //TODO: This is the only place where NN is required. Can make a generic evaluate for models?

    std::vector<std::pair<const LValueExpression*, double>> sensitivity_per_feature;
    sensitivity_per_feature.reserve(nn_interface.get_num_input_features());
    for (auto it = nn_interface.init_input_feature_iterator(false); !it.end(); ++it) {

        auto state_index = it.get_input_feature_index();
        const auto& var_info = varInfo->get_info(state_index);

        const auto perturbation_interval = var_info.get_perturbation_interval();

        if (PLAJA_FLOATS::is_zero(perturbation_interval, PLAJA::floatingPrecision)) { continue; } // skip for equal-valued variables

        // sensitivity(v) = max(|NN(t[v -> up_v]) − NN(t[v -> v])|, |NN(t[v -> up_l]) − NN(t[v -> v])|)
        sensitivity_per_feature.emplace_back(it.get_input_feature_expression(), 0);

        switch (var_info.get_perturbation_point_flag()) {

            case VARIABLE_SELECTION::VarInfo::Lower: {
                input_state[it._input_index()] = var_info.get_current_ub();
                sensitivity_per_feature.back().second = nn.computeDiffSum(input_state, outputs_base);
                break;
            }

            case VARIABLE_SELECTION::VarInfo::Upper: {
                input_state[it._input_index()] = var_info.get_current_lb();
                sensitivity_per_feature.back().second = nn.computeDiffSum(input_state, outputs_base);
                break;
            }

            case VARIABLE_SELECTION::VarInfo::Special: {
                input_state[it._input_index()] = var_info.get_current_lb();
                const auto sensitivity_lb = nn.computeDiffSum(input_state, outputs_base);

                input_state[it._input_index()] = var_info.get_current_ub();
                const auto sensitivity_ub = nn.computeDiffSum(input_state, outputs_base);

                sensitivity_per_feature.back().second = std::max(sensitivity_lb, sensitivity_ub);
                break;

            }

        }

        // reset
        input_state[it._input_index()] = var_info.get_perturbation_point();

    }

    PLAJA_ASSERT(sensitivity_per_feature.size() <= nn_interface.get_num_input_features())

    // sort (ASC)
    std::sort(sensitivity_per_feature.begin(), sensitivity_per_feature.end(), [this](const std::pair<const LValueExpression*, double>& left, const std::pair<const LValueExpression*, double>& right) { if (ascTravOrd) { return left.second < right.second; } else { return left.second > right.second; } });

    STMT_IF_DEBUG(auto last_value = std::numeric_limits<double>::lowest(); for (auto& var_sensitivity: sensitivity_per_feature) {
        PLAJA_ASSERT(ascTravOrd ? var_sensitivity.second > last_value : var_sensitivity.second < last_value)
        last_value = var_sensitivity.second;
    })

    std::list<const LValueExpression*> traversing_order;
    for (auto& var_sensitivity: sensitivity_per_feature) { traversing_order.push_back(var_sensitivity.first); }

    std::list<const LValueExpression*> traversing_order_extended;
    for (auto it = nn_interface.init_input_feature_iterator_extended(); !it.end(); ++it) {
        if (not PLAJA_FLOATS::is_zero(varInfo->get_info(it.get_input_feature_index()).get_perturbation_interval(), PLAJA::floatingPrecision)) {
            traversing_order_extended.push_back(it.get_input_feature_expression());
        }
    }

    if (ascTravOrd) {
        traversing_order_extended.splice(traversing_order_extended.cend(), traversing_order);
        std::swap(traversing_order, traversing_order_extended);
    } else {
        traversing_order.splice(traversing_order.end(), traversing_order_extended);
    }

    PLAJA_ASSERT(not traversing_order.empty())
    return traversing_order;
}

/**********************************************************************************************************************/

void VariableSelection::add_perturbation_constraint(VariableIndex_type state_index, PLAJA::floating perturbation_bound) { // NOLINT(*-easily-swappable-parameters)

    const auto& var_info = varInfo->get_info(state_index);
    const auto marabou_var = model->get_state_indexes(0).to_marabou(state_index);
    solver->tighten_lower_bound(marabou_var, var_info.compute_lower_perturbation_bound(perturbation_bound));
    solver->tighten_upper_bound(marabou_var, var_info.compute_upper_perturbation_bound(perturbation_bound));

}

void VariableSelection::add_perturbation_constraint_0_norm(PLAJA::floating perturbation_bound, const std::unordered_set<VariableIndex_type>& perturbed_vars) {

    const auto& marabou_interface = model->get_state_indexes(0);

    // Iterate over all inputs to obtain fixed order equation constraint.
    for (auto input_it = model->get_interface()->init_input_feature_iterator(true); !input_it.end(); ++input_it) {

        const auto state_index = input_it.get_input_feature_index();

        if (not perturbed_vars.count(state_index)) { continue; }

        const auto& var_info = varInfo->get_info(state_index);
        const auto marabou_var = marabou_interface.to_marabou(state_index);
        solver->tighten_lower_bound(marabou_var, var_info.compute_lower_perturbation_bound(perturbation_bound));
        solver->tighten_upper_bound(marabou_var, var_info.compute_upper_perturbation_bound(perturbation_bound));

    }

}

/**********************************************************************************************************************/

void VariableSelection::add_perturbation_constraint_1_norm(PLAJA::floating perturbation_bound, const std::unordered_set<VariableIndex_type>& perturbed_vars) {

    const auto& marabou_interface = model->get_state_indexes(0);

    Equation perturbation_bound_constraint(Equation::LE);
    perturbation_bound_constraint.setScalar(perturbation_bound);

    // Iterate over all inputs to obtain fixed order equation constraint.
    for (auto input_it = model->get_interface()->init_input_feature_iterator(true); !input_it.end(); ++input_it) {

        const auto state_index = input_it.get_input_feature_index();

        if (not perturbed_vars.count(state_index)) { continue; }

        const auto marabou_var = marabou_interface.to_marabou(state_index);
        const auto& var_info = varInfo->get_info(state_index);

        const auto perturbation_interval = var_info.get_perturbation_interval();

        switch (var_info.get_perturbation_point_flag()) {

            case VARIABLE_SELECTION::VarInfo::Lower: {
                // 1-NORM: add *normalized* contribution to perturbation bound: || s'(v) - s(v) || --> (v - s(v)) / normalization
                perturbation_bound_constraint.addAddend(1.0 / perturbation_interval, marabou_var);
                perturbation_bound_constraint._scalar += var_info.get_perturbation_point() / perturbation_interval; // ...
                break;
            }

            case VARIABLE_SELECTION::VarInfo::Upper: {
                // 1-NORM -> || s'(v) - s(v) || --> (s(v) - v) / normalization
                perturbation_bound_constraint.addAddend(-1.0 / perturbation_interval, marabou_var);
                perturbation_bound_constraint._scalar -= var_info.get_perturbation_point() / perturbation_interval; // ...
                break;
            }

            case VARIABLE_SELECTION::VarInfo::Special: { // NOLINT(*-branch-clone)
                PLAJA_ABORT // TODO aux vars and abs
            }

            default: { PLAJA_ABORT }

        }

        // Tighten bounds.
        solver->tighten_lower_bound(marabou_var, var_info.get_current_lb());
        solver->tighten_upper_bound(marabou_var, var_info.get_current_ub());

    }

    solver->add_equation(perturbation_bound_constraint);

}

/**********************************************************************************************************************/

void VariableSelection::constraint_non_irrelevant_vars(const StateBase& state, const std::unordered_set<VariableIndex_type>& irrelevant_vars) {
    const auto& marabou_interface = model->get_state_indexes(0);

    for (auto input_it = model->get_interface()->init_input_feature_iterator(true); !input_it.end(); ++input_it) {

        const auto state_index = input_it.get_input_feature_index();

        if (irrelevant_vars.count(state_index)) { continue; }

        const auto marabou_var = marabou_interface.to_marabou(state_index);

        // fix non-irrelevant variables, i.e., s'(v) == s(v)
        solver->tighten_lower_bound(marabou_var, state[state_index]);
        solver->tighten_upper_bound(marabou_var, state[state_index]);

    }

}

void VariableSelection::compute_relevant_vars(const StateBase& state, ActionLabel_type action_label, PLAJA::floating perturbation_bound, const std::list<const LValueExpression*>& traversing_order, std::list<const LValueExpression*>& relevant_features, std::unordered_set<VariableIndex_type>& irrelevant_features) { // NOLINT(*-easily-swappable-parameters)
    PLAJA_ASSERT(relevant_features.empty())
    PLAJA_ASSERT(irrelevant_features.empty())

    for (auto var: traversing_order) {
        auto state_index = var->get_variable_index();

        irrelevant_features.insert(state_index);
        solver->push();

        // Constraint perturbation of irrelevant variables (including the currently added candidate variable) // =: phi
        if (perturb1Norm) { add_perturbation_constraint_1_norm(perturbation_bound, irrelevant_features); }
        else { add_perturbation_constraint_0_norm(perturbation_bound, irrelevant_features); }
        // Fix non-irrelevant vars.
        constraint_non_irrelevant_vars(state, irrelevant_features);

        bool hold; // NOLINT(*-init-variables)
        if (perturbWitness) {
            // CheckValid(phi -> pi(s') = l) <--> CheckValid(not phi or pi(s') = l) <--> CheckUnsat(not (not phi or pi(s') = l)) <--> CheckUnsat(phi and pi(s') != l)
            if constexpr (false) {
                for (auto output_it = model->get_interface()->init_output_feature_iterator(); !output_it.end(); ++output_it) {
                    if (output_it.get_output_label() == action_label) { continue; }
                    solver->push();
                    model->add_output_interface(*solver, output_it.get_output_label());
                    hold = not solver->check();
                    solver->pop();
                    if (not hold) { break; }
                }
            } else {
                model->add_output_interface_negation(*solver, action_label);
                hold = not solver->check(); // hold <- CheckValid(phi -> pi(s') == l) <--> CheckUnsat(phi && pi(s') != l)
            }
        } else {
            model->add_output_interface(*solver, action_label);
            hold = not solver->check(); // hold <- CheckValid(phi -> pi(s') != l) <--> CheckUnsat(phi && pi(s') == l)
        }

        if (not hold) {
            irrelevant_features.erase(state_index);
            relevant_features.push_back(var);
        }

        solver->pop();
    }

}

/**********************************************************************************************************************/

bool VariableSelection::check_decision_boundary(ActionLabel_type action_label, const VARIABLE_SELECTION::PerturbationBoundMap& relative_perturbation) {

    solver->push();

    const auto& marabou_interface = model->get_state_indexes(0);
    for (auto it = model->get_interface()->init_input_feature_iterator(true); !it.end(); ++it) {

        const auto state_index = it.get_input_feature_index();
        const auto& var_info = varInfo->get_info(state_index);
        const auto& perturbation_bound = relative_perturbation.at(state_index);
        const auto marabou_var = marabou_interface.to_marabou(state_index);

        solver->tighten_lower_bound(marabou_var, var_info.compute_lower_perturbation_bound(perturbation_bound.first));
        solver->tighten_upper_bound(marabou_var, var_info.compute_upper_perturbation_bound(perturbation_bound.second));

    }

    bool hold; // NOLINT(*-init-variables)
    if (perturbWitness) {
        model->add_output_interface_negation(*solver, action_label);
        hold = not solver->check();
    } else {
        model->add_output_interface(*solver, action_label);
        hold = not solver->check();
    }

    solver->pop();

    return hold;

}

PLAJA::floating VariableSelection::compute_decision_boundary_greedy(ActionLabel_type action_label) {

    double bs_lower = 0;
    double bs_upper = 1.0;
    const auto perturbation_bound_precision = varInfo->propose_perturbation_bound_precision(perturbationBoundPrecision, true);
    VARIABLE_SELECTION::PerturbationBoundMap perturbation_bound_current((bs_lower + bs_upper) / 2);

    while (PLAJA_FLOATS::non_equal(bs_lower, bs_upper, perturbation_bound_precision)) {

        PLAJA_ASSERT(check_decision_boundary(action_label, VARIABLE_SELECTION::PerturbationBoundMap(bs_lower)))
        PLAJA_ASSERT(not check_decision_boundary(action_label, VARIABLE_SELECTION::PerturbationBoundMap(bs_upper)))

        const bool hold = check_decision_boundary(action_label, perturbation_bound_current);

        if (hold) { bs_lower = perturbation_bound_current.get_default(); }
        else { bs_upper = perturbation_bound_current.get_default(); }
        perturbation_bound_current.set_default((bs_lower + bs_upper) / 2);

        PLAJA_ASSERT(bs_lower < bs_upper)
    }

    PLAJA_ASSERT(check_decision_boundary(action_label, VARIABLE_SELECTION::PerturbationBoundMap(bs_lower)))

    PLAJA_LOG_DEBUG(PLAJA_UTILS::to_red_string(PLAJA_UTILS::string_f("Global bound: %f with precision %f", bs_lower, perturbation_bound_precision)))

    return bs_lower;

}

bool VariableSelection::compute_decision_boundary_greedy(VariableIndex_type state_index, ActionLabel_type action_label, VARIABLE_SELECTION::PerturbationBoundMap& perturbation_bound, VARIABLE_SELECTION::PerturbationBoundType type) { // NOLINT(*-easily-swappable-parameters)

    PLAJA_ASSERT(check_decision_boundary(action_label, perturbation_bound))

    double bs_lower = perturbation_bound.at(state_index, type);
    double bs_upper = 1.0;

    // Maybe the variable is not important.
    perturbation_bound.set(state_index, 1.0, type);
    if (check_decision_boundary(action_label, perturbation_bound)) { return false; }

    // Is individual perturbation refinement enabled?
    if (not individualPerturbation) {
        perturbation_bound.unset(state_index);
        return true;
    }

    perturbation_bound.set(state_index, (bs_lower + bs_upper) / 2, type);
    const auto perturbation_bound_precision = varInfo->get_info(state_index).propose_perturbation_precision(perturbationBoundPrecision);

    while (PLAJA_FLOATS::non_equal(bs_lower, bs_upper, perturbation_bound_precision)) {

        const bool hold = check_decision_boundary(action_label, perturbation_bound);

        if (hold) { bs_lower = perturbation_bound.at(state_index, type); }
        else { bs_upper = perturbation_bound.at(state_index, type); }
        perturbation_bound.set(state_index, (bs_lower + bs_upper) / 2, type);

        PLAJA_ASSERT(bs_lower < bs_upper)
    }

    PLAJA_LOG_DEBUG(PLAJA_UTILS::to_red_string(PLAJA_UTILS::string_f("Optimizing variable: %i to %.10f with precision %f towards %s", state_index, bs_lower, perturbation_bound_precision, VARIABLE_SELECTION::PerturbationBoundMap::type_to_str(type).c_str())))

    perturbation_bound.set(state_index, bs_lower, type);

    PLAJA_ASSERT(check_decision_boundary(action_label, perturbation_bound))

    return true;
}

/**********************************************************************************************************************/

const LValueExpression* VariableSelection::compute_most_sensitive_variable(const StateBase& witness, const StateBase& non_witness) {
    // Reset cache.
    lastPertBound->clear();

    const auto& state = perturbWitness ? witness : non_witness;
    const auto* ref_state = perturbTowardsRef ? (perturbWitness ? &non_witness : &witness) : nullptr; // the "other" state
    varInfo->load_var_info(state, ref_state);

    auto traversing_order = compute_traversing_order();
    return ascTravOrd ? traversing_order.back() : traversing_order.front();
}

std::list<const LValueExpression*> VariableSelection::compute_relevant_vars(const StateBase& witness, const StateBase& non_witness, ActionLabel_type action_label) {
    PLAJA_ASSERT(witness != non_witness)
    PLAJA_ASSERT(model->get_interface()->is_chosen(witness, action_label))
    PLAJA_ASSERT(not model->get_interface()->is_chosen(non_witness, action_label))

    // Reset cache.
    lastPertBound->clear();

    const auto& state = perturbWitness ? witness : non_witness;
    const auto* ref_state = perturbTowardsRef ? (perturbWitness ? &non_witness : &witness) : nullptr; // the "other" state
    varInfo->load_var_info(state, ref_state);

    const auto traversing_order = compute_traversing_order();

    std::unordered_set<VariableIndex_type> irrelevant_features;
    std::list<const LValueExpression*> relevant_features;

    // If user-provided perturbation bound results in empty set of relevant features,
    // perform binary search for smallest perturbation bound such that relevant_features is non-empty
    // (same if no user-provided bound).
    double bs_lower = perturbationBoundUser == -1.0 ? 0 : perturbationBoundUser; // if user-provided we only increase
    double bs_upper = 1.0;
    double perturbation_bound_current = perturbationBoundUser == -1.0 ? (bs_lower + bs_upper) / 2 : perturbationBoundUser;
    const auto perturbation_bound_precision = varInfo->propose_perturbation_bound_precision(perturbationBoundPrecision, false);
    PLAJA_ASSERT(perturbationBoundUser == 1 or bs_lower < bs_upper)
    //
    while (PLAJA_FLOATS::non_equal(bs_lower, bs_upper, perturbation_bound_precision)) {

        std::unordered_set<VariableIndex_type> irrelevant_features_current;
        std::list<const LValueExpression*> relevant_features_current;

        compute_relevant_vars(state, action_label, perturbation_bound_current, traversing_order, relevant_features_current, irrelevant_features_current);
        PLAJA_ASSERT(PLAJA_FLOATS::lt(perturbation_bound_current, 1.0) or not relevant_features_current.empty())

        if (relevant_features_current.empty()) {

            bs_lower = perturbation_bound_current;
            perturbation_bound_current = (bs_lower + bs_upper) / 2;

        } else {

            relevant_features = std::move(relevant_features_current);
            irrelevant_features = std::move(irrelevant_features_current);

            // if (relevant_features.size() == 1) { break; } // not valid, we search minimum-epsilon not |Var_sel|-minimum epsilon

            bs_upper = perturbation_bound_current;
            perturbation_bound_current = (bs_lower + bs_upper) / 2;

        }

        PLAJA_ASSERT(bs_lower < bs_upper)
    }

    // fall-back
    if (relevant_features.empty()) {

        PLAJA_ASSERT(irrelevant_features.empty())

        perturbation_bound_current = 1;

        compute_relevant_vars(state, action_label, perturbation_bound_current, traversing_order, relevant_features, irrelevant_features);

    }

    PLAJA_ASSERT(not relevant_features.empty())

    if (stats) {
        relevantVars += relevant_features.size();
        irrelevantVars += irrelevant_features.size();
        stats->set_attr_double(PLAJA::StatsDouble::RelevantVars, static_cast<double>(relevantVars) / (irrelevantVars + relevantVars));
        stats->set_attr_double(PLAJA::StatsDouble::IrrelevantVars, static_cast<double>(irrelevantVars) / (irrelevantVars + relevantVars));
        stats->set_attr_double(PLAJA::StatsDouble::AvPertBound, (pertBoundSum += perturbation_bound_current) / ++pertBoundCounter);
    }

    lastPertBound->set_default(perturbation_bound_current);
    return relevant_features;
}

// Notes concerning soundness & completeness
// sound: true only if actually valid <--> unsat only if actually unsat --> over-approxmation is ok
// complete: true always if valid <--> unsat always if valid

std::list<const LValueExpression*> VariableSelection::compute_decision_boundary(const StateBase& witness, const StateBase& non_witness, ActionLabel_type action_label) {

    PLAJA_ASSERT(not perturb1Norm) // Not supported.

    PLAJA_ASSERT(witness != non_witness)
    PLAJA_ASSERT(model->get_interface()->is_chosen(witness, action_label))
    PLAJA_ASSERT(not model->get_interface()->is_chosen(non_witness, action_label))

    // Reset cache.
    lastPertBound->clear();

    const auto& state = perturbWitness ? witness : non_witness;
    const auto* ref_state = perturbTowardsRef ? (perturbWitness ? &non_witness : &witness) : nullptr; // the "other" state
    varInfo->load_var_info(state, ref_state);

    // Compute default perturbation bound.
    if (globalPerturbation) {
        lastPertBound->set_default(compute_decision_boundary_greedy(action_label));
    } else {
        lastPertBound->set_default(0);
    }

    const auto traversing_order = compute_traversing_order();

    std::list<const LValueExpression*> split_vars;

    double perturbation_bound_sum = 0;

    // Try to increase perturbation bound for each variable individually.
    for (const auto& var: traversing_order) {

        if (compute_decision_boundary_greedy(var->get_variable_index(), action_label, *lastPertBound, VARIABLE_SELECTION::PerturbationBoundType::Both)) { split_vars.push_back(var); }

        perturbation_bound_sum += lastPertBound->at(var->get_variable_index(), VARIABLE_SELECTION::PerturbationBoundType::Both);

    }

    if (stats) {
        const double perturbation_bound_average = perturbation_bound_sum / PLAJA_UTILS::cast_numeric<double>(traversing_order.size());
        stats->set_attr_double(PLAJA::StatsDouble::AvPertBound, (pertBoundSum += perturbation_bound_average) / ++pertBoundCounter);
    }

    return split_vars;
}

std::list<const LValueExpression*> VariableSelection::compute_decision_boundary(const StateBase& witness, const PaStateBase* pa_state, ActionLabel_type action_label) {

    PLAJA_ASSERT(not perturb1Norm) // Not supported.
    PLAJA_ASSERT(perturbWitness)
    // PLAJA_ASSERT(not perturbTowardsRef)

    PLAJA_ASSERT(not pa_state or pa_state->is_abstraction(witness));
    PLAJA_ASSERT(model->get_interface()->is_chosen(witness, action_label))

    // Reset cache.
    lastPertBound->clear();

    solver->push();

    if (pa_state) {

        model->add_to(*pa_state, *solver); // TODO having only bound predicates this is redundancy with respect to perturbation constraints.
        const auto bounds = pa_state->compute_bounds(model->get_model_info());
        varInfo->load_var_info(witness, bounds.first, bounds.second);

    } else { varInfo->load_var_info(witness, nullptr); }

    // Compute default perturbation bound.
    if (globalPerturbation) {
        lastPertBound->set_default(compute_decision_boundary_greedy(action_label));
    } else {
        lastPertBound->set_default(0);
    }

    const auto traversing_order = compute_traversing_order();

    std::list<const LValueExpression*> split_vars;

    double perturbation_bound_sum = 0;

    // Try to increase perturbation bound for each variable individually.
    for (const auto& var: traversing_order) {
        if (compute_decision_boundary_greedy(var->get_variable_index(), action_label, *lastPertBound, VARIABLE_SELECTION::PerturbationBoundType::Both)) {

            const auto& var_info = varInfo->get_info(var->get_variable_index());

            split_vars.push_back(var);

            if (perDirectionPerturbation and var_info.perturb_towards() == VARIABLE_SELECTION::PerturbationBoundType::Both) {
                compute_decision_boundary_greedy(var->get_variable_index(), action_label, *lastPertBound, VARIABLE_SELECTION::PerturbationBoundType::Lower);
                compute_decision_boundary_greedy(var->get_variable_index(), action_label, *lastPertBound, VARIABLE_SELECTION::PerturbationBoundType::Upper);
            }

        }

        const auto& perturbation_bound = lastPertBound->at(var->get_variable_index());
        perturbation_bound_sum += std::min(perturbation_bound.first, perturbation_bound.second); // For stats, we always use the minimum.

    }

    if (stats) {
        const double perturbation_bound_average = perturbation_bound_sum / PLAJA_UTILS::cast_numeric<double>(traversing_order.size());
        stats->set_attr_double(PLAJA::StatsDouble::AvPertBound, (pertBoundSum += perturbation_bound_average) / ++pertBoundCounter);
    }

    solver->pop();

    return split_vars;

}

/**********************************************************************************************************************/

std::unique_ptr<Expression> VariableSelection::get_last_lower_perturbation_bound(VariableIndex_type state_index) const {
    return varInfo->get_info(state_index).compute_lower_perturbation_bound_absolute(lastPertBound->at(state_index, VARIABLE_SELECTION::PerturbationBoundType::Lower));
}

std::unique_ptr<Expression> VariableSelection::get_last_upper_perturbation_bound(VariableIndex_type state_index) const {
    return varInfo->get_info(state_index).compute_upper_perturbation_bound_absolute(lastPertBound->at(state_index, VARIABLE_SELECTION::PerturbationBoundType::Upper));
}