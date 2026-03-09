//
// This file is part of the PlaJA code base.
// Copyright (c) (2019 - 2019) Marcel Vinzent and (2023 - 2024) Chaahat Jain.
// See README.md in the top-level directory for licensing information.
//


#include "property_information.h"
#include "../../exception/property_analysis_exception.h"
#include "../../include/ct_config_const.h"
#include "../../option_parser/plaja_options.h"
#include "../../parser/ast/expression/non_standard/objective_expression.h"
#include "../../parser/ast/expression/non_standard/predicate_abstraction_expression.h"
#include "../../parser/ast/expression/non_standard/problem_instance_expression.h"
#include "../../parser/ast/expression/bool_value_expression.h"
#include "../../parser/ast/expression/integer_value_expression.h"
#include "../../parser/ast/expression/filter_expression.h"
#include "../../parser/ast/expression/state_predicate_expression.h"
#include "../../parser/ast/expression/path_expression.h"
#include "../../parser/ast/expression/expectation_expression.h"
#include "../../parser/ast/expression/qfied_expression.h"
#include "../../parser/ast/non_standard/reward_accumulation.h"
#include "../../parser/ast/property.h"
#include "../../parser/visitor/to_normalform.h"
#include "../../parser/jani_words.h"
#include "../factories/configuration.h"
#include "../factories/search_engine_factory.h"
#include "jani2nnet/jani_2_nnet.h"
#ifdef USE_VERITAS
#include "jani2ensemble/jani_2_ensemble.h"
#endif

#include "../../parser/ast/expression/non_standard/predicates_expression.h"
#include "../../parser/ast/expression/binary_op_expression.h"


namespace PROPERTY_INFORMATION {
    const std::unique_ptr<Expression> zeroCostExp(new IntegerValueExpression(0)); // NOLINT(cert-err58-cpp)
    const std::unique_ptr<Expression> unitCostExp(new IntegerValueExpression(1)); // NOLINT(cert-err58-cpp)
}

# if 0
PropertyInformation::PropertyInformation(PropertyType property_type, const Property* property_, const Model* model, std::unique_ptr<PropertyExpression>&& property_own):
    propertyType(property_type),
    property(property_),
    model(model),
    propertyOwn(std::move(property_own)),
    reachExp(nullptr), costExp(nullptr)
    { load_nn_interface(); }
#endif

PropertyInformation::PropertyInformation(const PLAJA::Configuration& config, PropertyType property_type, const Property* property_, const Model* model):
    propertyType(property_type)
    , property(property_)
    , model(model)
    , propertyOwn(nullptr)
    , startExp(nullptr)
    , reachExp(nullptr)
#ifdef ENABLE_TERMINAL_STATE_SUPPORT
    , terminalExp(nullptr)
#endif
    , costExp(nullptr)
    , predicates(nullptr)
    , learningObjective(nullptr) { load_interface(config); }

PropertyInformation::~PropertyInformation() = default;

void PropertyInformation::load_interface(const PLAJA::Configuration& config) {
    if (config.has_value_option(PLAJA_OPTION::nn_interface)) {
        interface = Jani2NNet::load_interface(*model);
    }
#ifdef USE_VERITAS
    else if (config.has_value_option(PLAJA_OPTION::ensemble_interface)) {
        interface = Jani2Ensemble::load_interface(*model);
    }
#endif
    else if (not config.is_flag_set(PLAJA_OPTION::ignore_nn) and property and propertyType == PA_PROPERTY) {
        const auto* pa_exp = PLAJA_UTILS::cast_ptr<PAExpression>(get_property_expression());
        PLAJA_ASSERT(pa_exp)
        if (pa_exp->nn_file_exists()) { interface = Jani2NNet::load_interface(*model, pa_exp->get_nnFile()); }
    }
}

const std::string& PropertyInformation::get_property_name() const {
    if (property) { return property->get_name(); }
    else { return PLAJA_UTILS::emptyString; }
}

const PropertyExpression* PropertyInformation::get_property_expression() const {
    if (propertyOwn) { return propertyOwn.get(); }
    if (property) { return property->get_propertyExpression(); }
    else { return nullptr; }
}

std::unique_ptr<Expression> PropertyInformation::get_non_terminal() const {
    PLAJA_ASSERT(get_terminal())
    auto non_terminal = get_terminal()->deepCopy_Exp();
    TO_NORMALFORM::negate(non_terminal);
    return non_terminal;
}

std::unique_ptr<Expression> PropertyInformation::get_non_terminal_reach() const {
    std::list<std::unique_ptr<Expression>> conjuncts;
    conjuncts.push_back(get_non_terminal());
    PLAJA_ASSERT(reachExp)
    conjuncts.push_back(get_reach()->deepCopy_Exp());
    return TO_NORMALFORM::construct_conjunction(std::move(conjuncts));
}

// const std::string* PropertyInformation::get_nn_file() const { return nnInterface ? &nnInterface->get_interface_file() : nullptr; }

std::unique_ptr<PropertyInformation> PropertyInformation::construct_property_information(PropertyInformation::PropertyType property_type, const Property* property, const Model* model) {
    const PLAJA::Configuration config(*PLAJA_GLOBAL::optionParser);
    return std::make_unique<PropertyInformation>(config, property_type, property, model);
}

/**********************************************************************************************************************/

std::unique_ptr<PropertyInformation> PropertyInformation::analyse_property(const Property& property, const Model& model) {
    const PLAJA::Configuration config(*PLAJA_GLOBAL::optionParser);
    return analyse_property(config, property, model);
}

std::unique_ptr<PropertyInformation> PropertyInformation::analyse_property(const PLAJA::Configuration& config, const Property& property, const Model& model) {
    const PropertyExpression* property_exp = property.get_propertyExpression();
    JANI_ASSERT(property_exp)

    const auto* filter_exp = PLAJA_UTILS::cast_ptr_if<FilterExpression>(property_exp);
    if (filter_exp) {

        /* Check "states". */
        const auto* states = PLAJA_UTILS::cast_ptr_if<StatePredicateExpression>(filter_exp->get_states());
        if (not states or StatePredicateExpression::INITIAL != states->get_op()) { throw PropertyAnalysisException(property.get_name(), JANI::STATES); }

        /* Check "fun" and "values". */

        // Reachability & BFS
        const auto* path_exp = PLAJA_UTILS::cast_ptr_if<PathExpression>(filter_exp->get_values());
        if (path_exp) { // currently this may only be plain reachability checked by BFS
            if ((FilterExpression::EXISTS == filter_exp->get_fun() or FilterExpression::VALUES == filter_exp->get_fun())) {
                auto prop_info = std::make_unique<PropertyInformation>(config, PropertyInformation::GOAL, &property, &model);
                prop_info->set_reach(extract_goal_exp(path_exp, property));
                prop_info->set_cost(PROPERTY_INFORMATION::zeroCostExp.get());
                return prop_info;
            } else { throw PropertyAnalysisException(property.get_name(), JANI::FUN); }
        }

        /* Probability property. */
        const auto* qfied_exp = PLAJA_UTILS::cast_ptr_if<QfiedExpression>(filter_exp->get_values());
        if (qfied_exp) {
            if (FilterExpression::MIN == filter_exp->get_fun() or FilterExpression::MAX == filter_exp->get_fun() or FilterExpression::VALUES == filter_exp->get_fun()) {
                return analyse_prob_prop(config, qfied_exp, property, model);
            } else { throw PropertyAnalysisException(property.get_name(), JANI::FUN); }
        }

        /* Expected cost. */
        const auto* exp_exp = PLAJA_UTILS::cast_ptr_if<ExpectationExpression>(filter_exp->get_values());
        if (exp_exp) {
            if (FilterExpression::AVG == filter_exp->get_fun() or FilterExpression::MIN == filter_exp->get_fun() or FilterExpression::MAX == filter_exp->get_fun()) {
                return analyse_cost_prop(config, exp_exp, property, model);
            } else { throw PropertyAnalysisException(property.get_name(), JANI::FUN); }
        }

    } else {

        const auto* pa_exp = PLAJA_UTILS::cast_ptr_if<PAExpression>(property_exp);
        if (pa_exp) {

            auto prop_info = std::make_unique<PropertyInformation>(config, PropertyInformation::PA_PROPERTY, &property, &model);
            prop_info->set_start(pa_exp->get_start());
            prop_info->set_predicates(pa_exp->get_predicates());

            if (config.is_flag_set(PLAJA_OPTION::pa_objective) and pa_exp->get_objective() and pa_exp->get_objective_goal()) {
                PLAJA_ASSERT(not PLAJA_GLOBAL::enableTerminalStateSupport or not config.get_bool_option(PLAJA_OPTION::set_pa_goal_objective_terminal))
                prop_info->set_reach(pa_exp->get_objective()->get_goal());
            } else {
                prop_info->set_reach(pa_exp->get_reach());
                prop_info->set_learning_objective(pa_exp->get_objective());

                if (pa_exp->get_objective() and PLAJA_GLOBAL::enableTerminalStateSupport and config.get_bool_option(PLAJA_OPTION::set_pa_goal_objective_terminal)) {
                    prop_info->set_terminal(pa_exp->get_objective_goal());
                }

            }

            return prop_info;
        }

        const auto* problem_instance_exp = PLAJA_UTILS::cast_ptr_if<ProblemInstanceExpression>(property_exp);
        if (problem_instance_exp) {
            auto prop_info = std::make_unique<PropertyInformation>(config, PropertyInformation::ProblemInstanceProperty, &property, &model);
            prop_info->set_start(problem_instance_exp->get_start());
            prop_info->set_reach(problem_instance_exp->get_reach());
            prop_info->set_predicates(problem_instance_exp->get_predicates());
            return prop_info;
        }

    }

    throw PropertyAnalysisException(property.get_name(), PLAJA_UTILS::emptyString);
}

const Expression* PropertyInformation::extract_goal_exp(const PathExpression* values, const Property& property) {

    /* Check that "bounds" fields are not specified, we do not handle them ... */
    if (values->get_step_bounds()) { throw PropertyAnalysisException(property.get_name(), JANI::STEP_BOUNDS); }
    if (values->get_time_bounds()) { throw PropertyAnalysisException(property.get_name(), JANI::TIME_BOUNDS); }
    if (values->get_number_reward_bounds() != 0) { throw PropertyAnalysisException(property.get_name(), JANI::REWARD_BOUNDS); }

    /* Only "F" or "U" ... */
    if (values->get_op() != PathExpression::EVENTUALLY and values->get_op() != PathExpression::UNTIL) { throw PropertyAnalysisException(property.get_name(), JANI::OP); }
    if (values->get_op() == PathExpression::UNTIL) {
        if (not PLAJA_UTILS::is_derived_ptr_type<BoolValueExpression>(values->get_left()) or not PLAJA_UTILS::cast_ptr<BoolValueExpression>(values->get_left())->evaluate_integer_const()) { throw PropertyAnalysisException(property.get_name(), JANI::LEFT); }
    }

    const auto* goal = PLAJA_UTILS::cast_ptr_if<Expression>(values->get_right());
    if (not values->get_right()->is_proposition() or not goal) { throw PropertyAnalysisException(property.get_name(), JANI::RIGHT); }

    PLAJA_ASSERT(goal)
    return goal;
}

std::unique_ptr<PropertyInformation> PropertyInformation::analyse_prob_prop(const PLAJA::Configuration& config, const QfiedExpression* values, const Property& property, const Model& model) {

    if (values->get_op() != QfiedExpression::PMIN and values->get_op() != QfiedExpression::PMAX) { throw PropertyAnalysisException(property.get_name(), JANI::OP); }

    const auto* path_exp = PLAJA_UTILS::cast_ptr_if<PathExpression>(values->get_path_formula());
    if (not path_exp) { throw PropertyAnalysisException(property.get_name(), JANI::EXP); }

    /* We only have to extract the goal ... */
    auto prop_info = std::make_unique<PropertyInformation>(config, values->get_op() == QfiedExpression::PMIN ? PropertyInformation::PROBMIN : PropertyInformation::PROBMAX, &property, &model);
    prop_info->set_reach(extract_goal_exp(path_exp, property));
    prop_info->set_cost(PROPERTY_INFORMATION::zeroCostExp.get());
    return prop_info;

}

std::unique_ptr<PropertyInformation> PropertyInformation::analyse_cost_prop(const PLAJA::Configuration& config, const ExpectationExpression* values, const Property& property, const Model& model) {

    /* Check that "instant" fields are not specified, we do not handle them ... */
    if (values->get_step_instant()) { throw PropertyAnalysisException(property.get_name(), JANI::STEP_INSTANT); }
    if (values->get_time_instant()) { throw PropertyAnalysisException(property.get_name(), JANI::TIME_INSTANT); }
    if (values->get_number_reward_instants() != 0) { throw PropertyAnalysisException(property.get_name(), JANI::REWARD_INSTANTS); }

    /* Only minimal expected cost ... */
    if (values->get_op() != ExpectationExpression::EMIN) { throw PropertyAnalysisException(property.get_name(), JANI::OP); }

    /* Check reward acc, currently only on exit (otherwise we would have to store accumulate-vector) ... */
    if (values->get_reward_accumulation()->size() != 1 or not values->get_reward_accumulation()->accumulate_exit()) { throw PropertyAnalysisException(property.get_name(), JANI::ACCUMULATE); }

    /* (value) exp must be a proposition ... */
    JANI_ASSERT(values->get_value())
    if (!values->get_value()->is_proposition()) { throw PropertyAnalysisException(property.get_name(), values->get_value()->to_string()); }

    /* ... finally, reach must be the goal expression: */
    const auto* goal = PLAJA_UTILS::cast_ptr_if<Expression>(values->get_reach());
    if (not values->get_reach()->is_proposition() or not goal) { throw PropertyAnalysisException(property.get_name(), JANI::REACH); }

    auto prop_info = std::make_unique<PropertyInformation>(config, PropertyInformation::MINCOST, &property, &model);
    prop_info->set_reach(goal);
    prop_info->set_cost(values->get_value());
    return prop_info;
}

